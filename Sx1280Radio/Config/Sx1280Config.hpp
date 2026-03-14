/*
 * Sx1280Config.hpp
 *
 * Created on: Mar 14, 2026
 * Author: Pranav
 */

#pragma once

#include <cstdint>

#include "SX128x.hpp"

namespace Sx1280Radio {

    struct Sx1280RfConfig {
        std::uint32_t frequency_hz{2400000000UL};
        std::int8_t tx_power_dbm{10};
        SX128x::RadioRampTimes_t ramp_time{SX128x::RADIO_RAMP_20_US};
        SX128x::RadioRegulatorModes_t regulator_mode{SX128x::USE_DCDC};
    };

    struct Sx1280LoRaConfig {
        SX128x::RadioLoRaSpreadingFactors_t spreading_factor{SX128x::LORA_SF7};
        SX128x::RadioLoRaBandwidths_t bandwidth{SX128x::LORA_BW_0800};
        SX128x::RadioLoRaCodingRates_t coding_rate{SX128x::LORA_CR_4_5};

        std::uint8_t preamble_length{12};
        SX128x::RadioLoRaPacketLengthsModes_t header_type{SX128x::LORA_PACKET_VARIABLE_LENGTH};
        std::uint8_t payload_length{255};
        SX128x::RadioLoRaCrcModes_t crc{SX128x::LORA_CRC_ON};
        SX128x::RadioLoRaIQModes_t iq_mode{SX128x::LORA_IQ_NORMAL};

        [[nodiscard]] SX128x::ModulationParams_t to_modulation_params() const noexcept {
            SX128x::ModulationParams_t params{};
            params.PacketType = SX128x::PACKET_TYPE_LORA;
            params.Params.LoRa.SpreadingFactor = spreading_factor;
            params.Params.LoRa.Bandwidth = bandwidth;
            params.Params.LoRa.CodingRate = coding_rate;
            return params;
        }

        [[nodiscard]] SX128x::PacketParams_t to_packet_params() const noexcept {
            SX128x::PacketParams_t params{};
            params.PacketType = SX128x::PACKET_TYPE_LORA;
            params.Params.LoRa.PreambleLength = preamble_length;
            params.Params.LoRa.HeaderType = header_type;
            params.Params.LoRa.PayloadLength = payload_length;
            params.Params.LoRa.Crc = crc;
            params.Params.LoRa.InvertIQ = iq_mode;
            return params;
        }
    };

    struct Sx1280TimeoutConfig {
        SX128x::TickTime_t tx_timeout{
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };

        SX128x::TickTime_t rx_timeout{
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };

        bool continuous_rx{true};
    };

    struct Sx1280RuntimeConfig {
        bool auto_fs{true};
        bool long_preamble{false};

        bool use_manual_gain{false};
        std::uint8_t manual_gain_value{0};

        SX128x::RadioLnaSettings_t lna_setting{
            SX128x::LNA_HIGH_SENSITIVITY_MODE
        };
    };

    struct Sx1280Config {
        Sx1280RfConfig rf{};
        Sx1280LoRaConfig lora{};
        Sx1280TimeoutConfig timeouts{};
        Sx1280RuntimeConfig runtime{};
    };

} // namespace Sx1280Radio
