// File: build-smoke/hello_world_dual.cpp

#include "Sx1280Device.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

    class TxListener final : public Sx1280Radio::Sx1280DeviceListener {
    public:
        void onTxDone() override {
            tx_done = true;
            std::cout << "[tx] tx done" << std::endl;
        }

        void onTxTimeout() override {
            tx_timeout = true;
            std::cout << "[tx] tx timeout" << std::endl;
        }

        void onRxError(SX128x::IrqErrorCode_t error_code) override {
            std::cout << "[tx] error code=" << static_cast<int>(error_code) << std::endl;
        }

        bool tx_done{false};
        bool tx_timeout{false};
    };

    class RxListener final : public Sx1280Radio::Sx1280DeviceListener {
    public:
        void onRxDone(const Sx1280Radio::Sx1280RxPacket& packet) override {
            last_payload.assign(packet.payload.begin(), packet.payload.end());
            rx_done = true;

            std::cout << "[rx] got packet: \"" << last_payload << "\""
                      << " rssi=" << packet.rssi_dbm
                      << " snr=" << packet.snr_db
                      << std::endl;
        }

        void onRxTimeout() override {
            rx_timeout = true;
            std::cout << "[rx] rx timeout" << std::endl;
        }

        void onRxError(SX128x::IrqErrorCode_t error_code) override {
            rx_error = true;
            std::cout << "[rx] error code=" << static_cast<int>(error_code) << std::endl;
        }

        bool rx_done{false};
        bool rx_timeout{false};
        bool rx_error{false};
        std::string last_payload{};
    };

    Sx1280Radio::Sx1280Config makeCommonConfig() {
        Sx1280Radio::Sx1280Config config{};

        config.rf.frequency_hz = 2400000000U;
        config.rf.tx_power_dbm = 10;
        config.rf.ramp_time = SX128x::RADIO_RAMP_20_US;
        config.rf.regulator_mode = SX128x::USE_DCDC;

        config.lora.spreading_factor = SX128x::LORA_SF7;
        config.lora.bandwidth = SX128x::LORA_BW_0800;
        config.lora.coding_rate = SX128x::LORA_CR_4_5;
        config.lora.preamble_length = 12;
        config.lora.header_type = SX128x::LORA_PACKET_VARIABLE_LENGTH;
        config.lora.payload_length = 64;
        config.lora.crc = SX128x::LORA_CRC_ON;
        config.lora.iq_mode = SX128x::LORA_IQ_NORMAL;

        config.timeouts.rx_timeout = {
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };
        config.timeouts.tx_timeout = {
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };
        config.timeouts.continuous_rx = true;

        config.runtime.auto_fs = true;
        config.runtime.long_preamble = false;
        config.runtime.use_manual_gain = false;
        config.runtime.lna_setting = SX128x::LNA_HIGH_SENSITIVITY_MODE;

        return config;
    }

    std::uint16_t makeIrqMask() {
        return SX128x::IRQ_TX_DONE
             | SX128x::IRQ_RX_DONE
             | SX128x::IRQ_RX_TX_TIMEOUT
             | SX128x::IRQ_CRC_ERROR
             | SX128x::IRQ_HEADER_ERROR
             | SX128x::IRQ_HEADER_VALID;
    }

}  // namespace

int main() {
    using namespace Sx1280Radio;

    try {
        const auto common_config = makeCommonConfig();

        LinuxSx1280RadioConfig rx_hw{};
        rx_hw.spi.device_path = "/dev/spidev0.0";
        rx_hw.spi.speed_hz = 1'000'000;
        rx_hw.spi.mode = 0;
        rx_hw.spi.bits_per_word = 8;
        rx_hw.gpio.chip_path = "/dev/gpiochip4";
        rx_hw.pins.reset_line = 22;
        rx_hw.pins.busy_line = 23;
        rx_hw.pins.has_dio1 = true;
        rx_hw.pins.dio1_line = 24;
        rx_hw.pins.has_dio2 = false;
        rx_hw.pins.has_dio3 = false;
        rx_hw.pins.has_tx_enable = false;
        rx_hw.pins.role = RadioRole::RxOnly;
        rx_hw.pins.tx_enable_behavior = TxEnableBehavior::None;

        LinuxSx1280RadioConfig tx_hw{};
        tx_hw.spi.device_path = "/dev/spidev0.1";
        tx_hw.spi.speed_hz = 1'000'000;
        tx_hw.spi.mode = 0;
        tx_hw.spi.bits_per_word = 8;
        tx_hw.gpio.chip_path = "/dev/gpiochip4";
        tx_hw.pins.reset_line = 5;
        tx_hw.pins.busy_line = 6;
        tx_hw.pins.has_dio1 = false;
        tx_hw.pins.dio1_line = 0;
        tx_hw.pins.has_dio2 = false;
        tx_hw.pins.has_dio3 = false;
        tx_hw.pins.has_tx_enable = true;
        tx_hw.pins.tx_enable_line = 12;
        tx_hw.pins.tx_enable_active_high = true;
        tx_hw.pins.role = RadioRole::TxOnly;
        tx_hw.pins.tx_enable_behavior = TxEnableBehavior::AssertDuringTxOnly;

        auto rx_radio = std::make_unique<LinuxSx1280Radio>(rx_hw);
        auto tx_radio = std::make_unique<LinuxSx1280Radio>(tx_hw);

        Sx1280Device rx_device(std::move(rx_radio));
        Sx1280Device tx_device(std::move(tx_radio));

        RxListener rx_listener{};
        TxListener tx_listener{};

        rx_device.setListener(&rx_listener);
        tx_device.setListener(&tx_listener);

        std::cout << "init rx..." << std::endl;
        if (rx_device.init() != Sx1280DeviceError::None) {
            std::cerr << "rx init failed" << std::endl;
            return 1;
        }

        std::cout << "init tx..." << std::endl;
        if (tx_device.init() != Sx1280DeviceError::None) {
            std::cerr << "tx init failed" << std::endl;
            return 2;
        }

        std::cout << "configure rx..." << std::endl;
        if (rx_device.configure(common_config) != Sx1280DeviceError::None) {
            std::cerr << "rx configure failed" << std::endl;
            return 3;
        }

        std::cout << "configure tx..." << std::endl;
        if (tx_device.configure(common_config) != Sx1280DeviceError::None) {
            std::cerr << "tx configure failed" << std::endl;
            return 4;
        }

        const auto irq_mask = makeIrqMask();

        rx_device.radio().SetDioIrqParams(irq_mask, irq_mask, 0, 0);
        tx_device.radio().SetDioIrqParams(irq_mask, 0, 0, 0);

        std::cout << "start rx..." << std::endl;
        if (rx_device.startRx(Sx1280ReceiveMode::Continuous) != Sx1280DeviceError::None) {
            std::cerr << "rx start failed" << std::endl;
            return 5;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        const std::string message = "hello world";
        const std::vector<std::uint8_t> payload(message.begin(), message.end());

        std::cout << "send tx..." << std::endl;
        if (tx_device.send(payload) != Sx1280DeviceError::None) {
            std::cerr << "tx send failed" << std::endl;
            return 6;
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

        while (std::chrono::steady_clock::now() < deadline) {
            rx_device.processIrqs();
            tx_device.processIrqs();

            if (rx_listener.rx_done) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!tx_listener.tx_done) {
            std::cerr << "tx never reported done" << std::endl;
        }

        if (!rx_listener.rx_done) {
            std::cerr << "rx never received payload" << std::endl;
            return 7;
        }

        if (rx_listener.last_payload != "hello world") {
            std::cerr << "payload mismatch: \"" << rx_listener.last_payload << "\"" << std::endl;
            return 8;
        }

        std::cout << "hello world demo passed" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 10;
    }
}