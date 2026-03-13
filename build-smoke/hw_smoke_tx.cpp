// File: build-smoke/hw_smoke_tx.cpp

#include "Sx1280Device.hpp"

#include <exception>
#include <iostream>
#include <memory>

int main() {
    using namespace Sx1280Radio;

    try {
        LinuxSx1280RadioConfig hw{};
        hw.spi.device_path = "/dev/spidev0.1";
        hw.spi.speed_hz = 1'000'000;
        hw.spi.mode = 0;
        hw.spi.bits_per_word = 8;

        hw.gpio.chip_path = "/dev/gpiochip4";

        hw.pins.reset_line = 5;
        hw.pins.busy_line = 6;

        // Set this once TX DIO1 is confirmed.
        // Using 0 as a placeholder will likely be wrong on real hardware.
        hw.pins.dio1_line = 0;

        hw.pins.has_dio2 = false;
        hw.pins.has_dio3 = false;

        hw.pins.has_tx_enable = true;
        hw.pins.tx_enable_line = 12;
        hw.pins.tx_enable_active_high = true;

        hw.pins.role = RadioRole::TxOnly;
        hw.pins.tx_enable_behavior = TxEnableBehavior::StaticDeasserted;

        auto radio = std::make_unique<LinuxSx1280Radio>(hw);
        Sx1280Device device(std::move(radio));

        std::cout << "Initializing TX radio..." << std::endl;
        const auto init_result = device.init();
        if (init_result != Sx1280DeviceError::None) {
            std::cerr << "device.init() failed" << std::endl;
            return 1;
        }

        Sx1280Config config{};
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
        config.timeouts.continuous_rx = false;

        config.runtime.auto_fs = true;
        config.runtime.long_preamble = false;
        config.runtime.use_manual_gain = false;
        config.runtime.lna_setting = SX128x::LNA_HIGH_SENSITIVITY_MODE;

        std::cout << "Configuring TX radio..." << std::endl;
        const auto cfg_result = device.configure(config);
        if (cfg_result != Sx1280DeviceError::None) {
            std::cerr << "device.configure() failed" << std::endl;
            return 2;
        }

        std::cout << "TX smoke test passed (no transmission performed)." << std::endl;
        std::cout << "Final state: ";
        switch (device.state()) {
            case Sx1280DeviceState::Uninitialized:
                std::cout << "Uninitialized";
                break;
            case Sx1280DeviceState::Idle:
                std::cout << "Idle";
                break;
            case Sx1280DeviceState::Receiving:
                std::cout << "Receiving";
                break;
            case Sx1280DeviceState::Transmitting:
                std::cout << "Transmitting";
                break;
            case Sx1280DeviceState::Error:
                std::cout << "Error";
                break;
        }
        std::cout << std::endl;

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 10;
    }
}