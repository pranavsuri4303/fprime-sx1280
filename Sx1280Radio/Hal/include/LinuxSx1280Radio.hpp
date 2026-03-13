// Sx1280Radio/Hal/include/LinuxSx1280Radio.hpp
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "SX128x.hpp"

namespace Sx1280Radio {

    enum class RadioRole {
        TxOnly,
        RxOnly,
        TxRx,
    };

    enum class TxEnableBehavior {
        None,
        StaticAsserted,
        StaticDeasserted,
        AssertDuringTxOnly,
    };

    struct LinuxSpiConfig {
        std::string device_path{"/dev/spidev0.0"};
        std::uint32_t speed_hz{1'000'000};
        std::uint8_t mode{0};
        std::uint8_t bits_per_word{8};
    };

    struct LinuxGpioChipConfig {
        std::string chip_path{"/dev/gpiochip0"};
    };

    struct LinuxSx1280PinConfig {
        std::uint32_t reset_line{0};
        std::uint32_t busy_line{0};
        std::uint32_t dio1_line{0};

        bool has_dio2{false};
        std::uint32_t dio2_line{0};

        bool has_dio3{false};
        std::uint32_t dio3_line{0};

        bool has_tx_enable{false};
        std::uint32_t tx_enable_line{0};
        bool tx_enable_active_high{true};

        RadioRole role{RadioRole::TxRx};
        TxEnableBehavior tx_enable_behavior{TxEnableBehavior::None};
    };

    struct LinuxSx1280RadioConfig {
        LinuxSpiConfig spi{};
        LinuxGpioChipConfig gpio{};
        LinuxSx1280PinConfig pins{};
    };

    class LinuxSx1280Radio final : public SX128x {
    public:
        explicit LinuxSx1280Radio(const LinuxSx1280RadioConfig& config);
        ~LinuxSx1280Radio() override;

        LinuxSx1280Radio(const LinuxSx1280Radio&) = delete;
        LinuxSx1280Radio& operator=(const LinuxSx1280Radio&) = delete;
        LinuxSx1280Radio(LinuxSx1280Radio&&) = delete;
        LinuxSx1280Radio& operator=(LinuxSx1280Radio&&) = delete;

        [[nodiscard]] const LinuxSx1280RadioConfig& config() const noexcept;

        std::uint8_t HalGpioRead(GpioPinFunction_t func) override;
        void HalGpioWrite(GpioPinFunction_t func, std::uint8_t value) override;
        void HalSpiTransfer(std::uint8_t* buffer_in,
                            const std::uint8_t* buffer_out,
                            std::uint16_t size) override;

        void HalPreTx() override;
        void HalPreRx() override;
        void HalPostTx() override;
        void HalPostRx() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

        [[nodiscard]] std::uint32_t pinFunctionToLine(GpioPinFunction_t func) const;
        [[nodiscard]] bool isPinConfigured(GpioPinFunction_t func) const;
    };

}