/*
 * NodeConfigYaml.cpp
 *
 * Created on: Mar 14, 2026
 * Author: Pranav
 */

#include "NodeConfig.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Sx1280Radio {

    namespace {

        bool read_file_to_string(const std::string& path, std::string& out, std::string* error_out) {
            std::ifstream ifs(path);
            if (!ifs) {
                if (error_out) {
                    *error_out = "failed to open config file: " + path;
                }
                return false;
            }
            std::ostringstream ss;
            ss << ifs.rdbuf();
            out = ss.str();
            return true;
        }

        std::string trim(const std::string& s) {
            std::size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
                ++start;
            }
            std::size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
                --end;
            }
            return s.substr(start, end - start);
        }

        bool starts_with(const std::string& s, const std::string& prefix) {
            return s.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
        }

        bool parse_bool(const std::string& v, bool& out) {
            if (v == "true") {
                out = true;
                return true;
            }
            if (v == "false") {
                out = false;
                return true;
            }
            return false;
        }

        bool parse_uint32(const std::string& v, std::uint32_t& out) {
            try {
                out = static_cast<std::uint32_t>(std::stoul(v));
                return true;
            } catch (...) {
                return false;
            }
        }

        bool parse_uint8(const std::string& v, std::uint8_t& out) {
            try {
                auto tmp = static_cast<unsigned long>(std::stoul(v));
                if (tmp > 0xFFUL) {
                    return false;
                }
                out = static_cast<std::uint8_t>(tmp);
                return true;
            } catch (...) {
                return false;
            }
        }

        bool parse_int8(const std::string& v, std::int8_t& out) {
            try {
                auto tmp = static_cast<long>(std::stol(v));
                if (tmp < -128L || tmp > 127L) {
                    return false;
                }
                out = static_cast<std::int8_t>(tmp);
                return true;
            } catch (...) {
                return false;
            }
        }

        std::string strip_quotes(const std::string& v) {
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
                return v.substr(1, v.size() - 2);
            }
            return v;
        }

        NodeRadioRole parse_node_role(const std::string& v) {
            if (v == "rx") {
                return NodeRadioRole::Rx;
            }
            if (v == "tx") {
                return NodeRadioRole::Tx;
            }
            return NodeRadioRole::TxRx;
        }

        TxEnableBehavior parse_tx_behavior(const std::string& v) {
            if (v == "none") {
                return TxEnableBehavior::None;
            }
            if (v == "static_asserted") {
                return TxEnableBehavior::StaticAsserted;
            }
            if (v == "static_deasserted") {
                return TxEnableBehavior::StaticDeasserted;
            }
            if (v == "assert_during_tx") {
                return TxEnableBehavior::AssertDuringTxOnly;
            }
            return TxEnableBehavior::None;
        }

        SX128x::RadioLoRaSpreadingFactors_t parse_spreading_factor(const std::string& v) {
            if (v == "sf7") {
                return SX128x::LORA_SF7;
            }
            return SX128x::LORA_SF7;
        }

        SX128x::RadioLoRaBandwidths_t parse_bandwidth(const std::string& v) {
            if (v == "bw_0800") {
                return SX128x::LORA_BW_0800;
            }
            return SX128x::LORA_BW_0800;
        }

        SX128x::RadioLoRaCodingRates_t parse_coding_rate(const std::string& v) {
            if (v == "cr_4_5") {
                return SX128x::LORA_CR_4_5;
            }
            return SX128x::LORA_CR_4_5;
        }

        SX128x::RadioLoRaPacketLengthsModes_t parse_header_type(const std::string& v) {
            if (v == "variable_length") {
                return SX128x::LORA_PACKET_VARIABLE_LENGTH;
            }
            return SX128x::LORA_PACKET_VARIABLE_LENGTH;
        }

        SX128x::RadioLoRaCrcModes_t parse_crc_mode(const std::string& v) {
            if (v == "on") {
                return SX128x::LORA_CRC_ON;
            }
            if (v == "off") {
                return SX128x::LORA_CRC_OFF;
            }
            return SX128x::LORA_CRC_ON;
        }

        SX128x::RadioLoRaIQModes_t parse_iq_mode(const std::string& v) {
            if (v == "normal") {
                return SX128x::LORA_IQ_NORMAL;
            }
            return SX128x::LORA_IQ_NORMAL;
        }

        SX128x::RadioRampTimes_t parse_ramp_time(const std::string& v) {
            if (v == "20us") {
                return SX128x::RADIO_RAMP_20_US;
            }
            return SX128x::RADIO_RAMP_20_US;
        }

        SX128x::RadioRegulatorModes_t parse_regulator_mode(const std::string& v) {
            if (v == "dcdc") {
                return SX128x::USE_DCDC;
            }
            return SX128x::USE_DCDC;
        }

        SX128x::RadioLnaSettings_t parse_lna_setting(const std::string& v) {
            if (v == "high_sensitivity") {
                return SX128x::LNA_HIGH_SENSITIVITY_MODE;
            }
            return SX128x::LNA_HIGH_SENSITIVITY_MODE;
        }

    } // namespace

    NodeConfigError loadNodeConfigFromYaml(
        const std::string& path,
        NodeConfig& out,
        std::string* error_out
    ) {
        std::string yaml;
        if (!read_file_to_string(path, yaml, error_out)) {
            return NodeConfigError::IoError;
        }

        enum class Section {
            None,
            RadiosRx,
            RadiosTx,
            Lora,
            Timeouts,
            Runtime,
        };

        enum class SubSection {
            None,
            Spi,
            Gpio,
            Pins,
        };

        Section section = Section::None;
        SubSection sub = SubSection::None;

        NodeConfig cfg{};

        std::istringstream iss(yaml);
        std::string line;
        while (std::getline(iss, line)) {
            const std::string t = trim(line);
            if (t.empty() || t[0] == '#') {
                continue;
            }

            if (t == "radios:") {
                section = Section::None;
                sub = SubSection::None;
                continue;
            }
            if (t == "rx:") {
                section = Section::RadiosRx;
                sub = SubSection::None;
                continue;
            }
            if (t == "tx:") {
                section = Section::RadiosTx;
                sub = SubSection::None;
                continue;
            }
            if (t == "spi:") {
                sub = SubSection::Spi;
                continue;
            }
            if (t == "gpio:") {
                sub = SubSection::Gpio;
                continue;
            }
            if (t == "pins:") {
                sub = SubSection::Pins;
                continue;
            }
            if (t == "lora:") {
                section = Section::Lora;
                sub = SubSection::None;
                continue;
            }
            if (t == "timeouts:") {
                section = Section::Timeouts;
                sub = SubSection::None;
                continue;
            }
            if (t == "runtime:") {
                section = Section::Runtime;
                sub = SubSection::None;
                continue;
            }

            const auto colon_pos = t.find(':');
            if (colon_pos == std::string::npos) {
                continue;
            }

            const std::string key = trim(t.substr(0, colon_pos));
            std::string value = trim(t.substr(colon_pos + 1));

            const auto hash_pos = value.find('#');
            if (hash_pos != std::string::npos) {
                value = trim(value.substr(0, hash_pos));
            }

            if (!value.empty() && value[0] == '"') {
                value = strip_quotes(value);
            }

            auto& rx = cfg.rx;
            auto& tx = cfg.tx;

            switch (section) {
                case Section::RadiosRx:
                case Section::RadiosTx: {
                    RadioHwConfig& rhw = (section == Section::RadiosRx) ? rx : tx;

                    if (sub == SubSection::None) {
                        if (key == "role") {
                            const auto role = parse_node_role(value);
                            rhw.role = role;
                        }
                    } else if (sub == SubSection::Spi) {
                        if (key == "device") {
                            rhw.hw.spi.device_path = value;
                        } else if (key == "speed_hz") {
                            parse_uint32(value, rhw.hw.spi.speed_hz);
                        } else if (key == "mode") {
                            parse_uint8(value, rhw.hw.spi.mode);
                        } else if (key == "bits_per_word") {
                            parse_uint8(value, rhw.hw.spi.bits_per_word);
                        }
                    } else if (sub == SubSection::Gpio) {
                        if (key == "chip") {
                            rhw.hw.gpio.chip_path = value;
                        }
                    } else if (sub == SubSection::Pins) {
                        auto& pins = rhw.hw.pins;
                        if (key == "reset") {
                            parse_uint32(value, pins.reset_line);
                        } else if (key == "busy") {
                            parse_uint32(value, pins.busy_line);
                        } else if (key == "has_dio1") {
                            parse_bool(value, pins.has_dio1);
                        } else if (key == "dio1") {
                            parse_uint32(value, pins.dio1_line);
                        } else if (key == "has_dio2") {
                            parse_bool(value, pins.has_dio2);
                        } else if (key == "dio2") {
                            parse_uint32(value, pins.dio2_line);
                        } else if (key == "has_dio3") {
                            parse_bool(value, pins.has_dio3);
                        } else if (key == "dio3") {
                            parse_uint32(value, pins.dio3_line);
                        } else if (key == "has_tx_enable") {
                            parse_bool(value, pins.has_tx_enable);
                        } else if (key == "tx_enable") {
                            parse_uint32(value, pins.tx_enable_line);
                        } else if (key == "tx_enable_active_high") {
                            parse_bool(value, pins.tx_enable_active_high);
                        } else if (key == "tx_enable_behavior") {
                            pins.tx_enable_behavior = parse_tx_behavior(value);
                        }
                    }
                } break;

                case Section::Lora: {
                    auto& rf = cfg.radio.rf;
                    auto& lora = cfg.radio.lora;
                    if (key == "frequency_hz") {
                        parse_uint32(value, rf.frequency_hz);
                    } else if (key == "tx_power_dbm") {
                        parse_int8(value, rf.tx_power_dbm);
                    } else if (key == "ramp_time") {
                        rf.ramp_time = parse_ramp_time(value);
                    } else if (key == "regulator_mode") {
                        rf.regulator_mode = parse_regulator_mode(value);
                    } else if (key == "spreading_factor") {
                        lora.spreading_factor = parse_spreading_factor(value);
                    } else if (key == "bandwidth") {
                        lora.bandwidth = parse_bandwidth(value);
                    } else if (key == "coding_rate") {
                        lora.coding_rate = parse_coding_rate(value);
                    } else if (key == "preamble_length") {
                        parse_uint8(value, lora.preamble_length);
                    } else if (key == "header_type") {
                        lora.header_type = parse_header_type(value);
                    } else if (key == "payload_length") {
                        parse_uint8(value, lora.payload_length);
                    } else if (key == "crc") {
                        lora.crc = parse_crc_mode(value);
                    } else if (key == "iq_mode") {
                        lora.iq_mode = parse_iq_mode(value);
                    }
                } break;

                case Section::Timeouts: {
                    auto& timeouts = cfg.radio.timeouts;
                    if (key == "tx_ms") {
                        std::uint32_t v32{};
                        if (parse_uint32(value, v32)) {
                            std::uint16_t clamped = 0;
                            if (v32 >= 0xFFFFu) {
                                clamped = 0xFFFFu;
                            } else {
                                clamped = static_cast<std::uint16_t>(v32);
                            }
                            timeouts.tx_timeout = {SX128x::RADIO_TICK_SIZE_1000_US, clamped};
                        }
                    } else if (key == "rx_ms") {
                        std::uint32_t v32{};
                        if (parse_uint32(value, v32)) {
                            std::uint16_t clamped = 0;
                            if (v32 >= 0xFFFFu) {
                                clamped = 0xFFFFu;
                            } else {
                                clamped = static_cast<std::uint16_t>(v32);
                            }
                            timeouts.rx_timeout = {SX128x::RADIO_TICK_SIZE_1000_US, clamped};
                        }
                    } else if (key == "continuous_rx") {
                        parse_bool(value, timeouts.continuous_rx);
                    }
                } break;

                case Section::Runtime: {
                    auto& runtime = cfg.radio.runtime;
                    if (key == "auto_fs") {
                        parse_bool(value, runtime.auto_fs);
                    } else if (key == "long_preamble") {
                        parse_bool(value, runtime.long_preamble);
                    } else if (key == "use_manual_gain") {
                        parse_bool(value, runtime.use_manual_gain);
                    } else if (key == "manual_gain_value") {
                        parse_uint8(value, runtime.manual_gain_value);
                    } else if (key == "lna_setting") {
                        runtime.lna_setting = parse_lna_setting(value);
                    }
                } break;

                case Section::None:
                default:
                    break;
            }
        }

        out = cfg;
        if (error_out) {
            *error_out = {};
        }
        return NodeConfigError::None;
    }

} // namespace Sx1280Radio
