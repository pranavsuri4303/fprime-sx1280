/*
 * LinuxSx1280Radio.cpp
 *
 * Created on: Mar 14, 2026
 * Author: Pranav
 */

#include "LinuxSx1280Radio.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <gpiod.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Sx1280Radio {

    namespace {

        [[noreturn]] void throwSystemError(const std::string& message) {
            throw std::system_error(errno, std::generic_category(), message);
        }

        [[noreturn]] void throwRuntimeError(const std::string& message) {
            throw std::runtime_error(message);
        }

        gpiod_line_value assertedLineValue(bool active_high, bool asserted) {
            const bool raw_high = asserted ? active_high : !active_high;
            return raw_high ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
        }

        gpiod_line_request* requestInputLine(gpiod_chip* chip,
                                             std::uint32_t offset,
                                             const char* consumer) {
            gpiod_line_settings* settings = gpiod_line_settings_new();
            if (!settings) {
                throwRuntimeError("failed to allocate GPIO line settings");
            }

            if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT) < 0) {
                gpiod_line_settings_free(settings);
                throwSystemError("failed to set GPIO input direction");
            }

            gpiod_line_config* line_config = gpiod_line_config_new();
            if (!line_config) {
                gpiod_line_settings_free(settings);
                throwRuntimeError("failed to allocate GPIO line config");
            }

            if (gpiod_line_config_add_line_settings(line_config, &offset, 1, settings) < 0) {
                gpiod_line_config_free(line_config);
                gpiod_line_settings_free(settings);
                throwSystemError("failed to add GPIO input line settings");
            }

            gpiod_request_config* request_config = gpiod_request_config_new();
            if (!request_config) {
                gpiod_line_config_free(line_config);
                gpiod_line_settings_free(settings);
                throwRuntimeError("failed to allocate GPIO request config");
            }

            gpiod_request_config_set_consumer(request_config, consumer);

            gpiod_line_request* request = gpiod_chip_request_lines(chip, request_config, line_config);

            gpiod_request_config_free(request_config);
            gpiod_line_config_free(line_config);
            gpiod_line_settings_free(settings);

            if (!request) {
                throwSystemError("failed to request GPIO input line");
            }

            return request;
        }

        gpiod_line_request* requestOutputLine(gpiod_chip* chip,
                                              std::uint32_t offset,
                                              gpiod_line_value initial_value,
                                              const char* consumer) {
            gpiod_line_settings* settings = gpiod_line_settings_new();
            if (!settings) {
                throwRuntimeError("failed to allocate GPIO line settings");
            }

            if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0) {
                gpiod_line_settings_free(settings);
                throwSystemError("failed to set GPIO output direction");
            }

            if (gpiod_line_settings_set_output_value(settings, initial_value) < 0) {
                gpiod_line_settings_free(settings);
                throwSystemError("failed to set GPIO output initial value");
            }

            gpiod_line_config* line_config = gpiod_line_config_new();
            if (!line_config) {
                gpiod_line_settings_free(settings);
                throwRuntimeError("failed to allocate GPIO line config");
            }

            if (gpiod_line_config_add_line_settings(line_config, &offset, 1, settings) < 0) {
                gpiod_line_config_free(line_config);
                gpiod_line_settings_free(settings);
                throwSystemError("failed to add GPIO output line settings");
            }

            gpiod_request_config* request_config = gpiod_request_config_new();
            if (!request_config) {
                gpiod_line_config_free(line_config);
                gpiod_line_settings_free(settings);
                throwRuntimeError("failed to allocate GPIO request config");
            }

            gpiod_request_config_set_consumer(request_config, consumer);

            gpiod_line_request* request = gpiod_chip_request_lines(chip, request_config, line_config);

            gpiod_request_config_free(request_config);
            gpiod_line_config_free(line_config);
            gpiod_line_settings_free(settings);

            if (!request) {
                throwSystemError("failed to request GPIO output line");
            }

            return request;
        }

        int getRequestedLineValue(gpiod_line_request* request, std::uint32_t offset) {
            const int value = gpiod_line_request_get_value(request, offset);
            if (value < 0) {
                throwSystemError("failed to read GPIO line value");
            }

            return value;
        }

        void setRequestedLineValue(gpiod_line_request* request,
                                   std::uint32_t offset,
                                   gpiod_line_value value) {
            if (gpiod_line_request_set_value(request, offset, value) < 0) {
                throwSystemError("failed to write GPIO line value");
            }
        }

    }  // namespace

    struct LinuxSx1280Radio::Impl {
        explicit Impl(LinuxSx1280RadioConfig cfg)
            : config(std::move(cfg)) {
            validateConfig();
            openSpi();
            openGpioChip();
            requestGpioLines();
            applyInitialTxEnableState();
        }

        ~Impl() {
            if (reset_request) {
                gpiod_line_request_release(reset_request);
            }

            if (busy_request) {
                gpiod_line_request_release(busy_request);
            }

            if (dio1_request) {
                gpiod_line_request_release(dio1_request);
            }

            if (dio2_request) {
                gpiod_line_request_release(dio2_request);
            }

            if (dio3_request) {
                gpiod_line_request_release(dio3_request);
            }

            if (tx_enable_request) {
                gpiod_line_request_release(tx_enable_request);
            }

            if (gpio_chip) {
                gpiod_chip_close(gpio_chip);
            }

            if (spi_fd >= 0) {
                ::close(spi_fd);
            }
        }

        void openSpi() {
            spi_fd = ::open(config.spi.device_path.c_str(), O_RDWR);
            if (spi_fd < 0) {
                throwSystemError("failed to open SPI device: " + config.spi.device_path);
            }

            std::uint8_t mode = config.spi.mode;
            std::uint8_t bits = config.spi.bits_per_word;
            std::uint32_t speed = config.spi.speed_hz;

            if (::ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
                throwSystemError("failed to set SPI mode");
            }

            if (::ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) < 0) {
                throwSystemError("failed to read SPI mode");
            }

            if (::ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
                throwSystemError("failed to set SPI bits per word");
            }

            if (::ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
                throwSystemError("failed to read SPI bits per word");
            }

            if (::ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
                throwSystemError("failed to set SPI max speed");
            }

            if (::ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
                throwSystemError("failed to read SPI max speed");
            }
        }

        void openGpioChip() {
            gpio_chip = gpiod_chip_open(config.gpio.chip_path.c_str());
            if (!gpio_chip) {
                throwSystemError("failed to open GPIO chip: " + config.gpio.chip_path);
            }
        }

        void requestGpioLines() {
            reset_request = requestOutputLine(
                gpio_chip,
                config.pins.reset_line,
                GPIOD_LINE_VALUE_ACTIVE,
                "sx1280-reset"
            );

            busy_request = requestInputLine(
                gpio_chip,
                config.pins.busy_line,
                "sx1280-busy"
            );

            if (config.pins.has_dio1) {
                dio1_request = requestInputLine(
                    gpio_chip,
                    config.pins.dio1_line,
                    "sx1280-dio1"
                );
            }

            if (config.pins.has_dio2) {
                dio2_request = requestInputLine(
                    gpio_chip,
                    config.pins.dio2_line,
                    "sx1280-dio2"
                );
            }

            if (config.pins.has_dio3) {
                dio3_request = requestInputLine(
                    gpio_chip,
                    config.pins.dio3_line,
                    "sx1280-dio3"
                );
            }

            if (config.pins.has_tx_enable) {
                tx_enable_request = requestOutputLine(
                    gpio_chip,
                    config.pins.tx_enable_line,
                    assertedLineValue(config.pins.tx_enable_active_high, false),
                    "sx1280-tx-enable"
                );
            }
        }

        void validateConfig() const {
            if (config.spi.device_path.empty()) {
                throwRuntimeError("SPI device path cannot be empty");
            }

            if (config.gpio.chip_path.empty()) {
                throwRuntimeError("GPIO chip path cannot be empty");
            }

            if (!config.pins.has_tx_enable &&
                config.pins.tx_enable_behavior != TxEnableBehavior::None) {
                throwRuntimeError("TX enable behavior requires has_tx_enable=true");
            }
        }

        void applyInitialTxEnableState() {
            if (!config.pins.has_tx_enable || !tx_enable_request) {
                return;
            }

            switch (config.pins.tx_enable_behavior) {
                case TxEnableBehavior::None:
                case TxEnableBehavior::AssertDuringTxOnly:
                    setTxEnable(false);
                    break;

                case TxEnableBehavior::StaticAsserted:
                    setTxEnable(true);
                    break;

                case TxEnableBehavior::StaticDeasserted:
                    setTxEnable(false);
                    break;
            }
        }

        void setTxEnable(bool asserted) {
            if (!config.pins.has_tx_enable || !tx_enable_request) {
                return;
            }

            setRequestedLineValue(
                tx_enable_request,
                config.pins.tx_enable_line,
                assertedLineValue(config.pins.tx_enable_active_high, asserted)
            );
        }

        std::uint8_t readInputLine(SX128x::GpioPinFunction_t func) const {
            switch (func) {
                case SX128x::GPIO_PIN_BUSY:
                    return static_cast<std::uint8_t>(
                        getRequestedLineValue(busy_request, config.pins.busy_line) ==
                        GPIOD_LINE_VALUE_ACTIVE
                    );

                case SX128x::GPIO_PIN_DIO1:
                    if (!config.pins.has_dio1 || !dio1_request) {
                        return 0;
                    }
                    return static_cast<std::uint8_t>(
                        getRequestedLineValue(dio1_request, config.pins.dio1_line) ==
                        GPIOD_LINE_VALUE_ACTIVE
                    );

                case SX128x::GPIO_PIN_DIO2:
                    if (!config.pins.has_dio2 || !dio2_request) {
                        return 0;
                    }
                    return static_cast<std::uint8_t>(
                        getRequestedLineValue(dio2_request, config.pins.dio2_line) ==
                        GPIOD_LINE_VALUE_ACTIVE
                    );

                case SX128x::GPIO_PIN_DIO3:
                    if (!config.pins.has_dio3 || !dio3_request) {
                        return 0;
                    }
                    return static_cast<std::uint8_t>(
                        getRequestedLineValue(dio3_request, config.pins.dio3_line) ==
                        GPIOD_LINE_VALUE_ACTIVE
                    );

                case SX128x::GPIO_PIN_RESET:
                    return static_cast<std::uint8_t>(
                        getRequestedLineValue(reset_request, config.pins.reset_line) ==
                        GPIOD_LINE_VALUE_ACTIVE
                    );
            }

            return 0;
        }

        void writeOutputLine(SX128x::GpioPinFunction_t func, std::uint8_t value) {
            switch (func) {
                case SX128x::GPIO_PIN_RESET:
                    setRequestedLineValue(
                        reset_request,
                        config.pins.reset_line,
                        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
                    );
                    return;

                case SX128x::GPIO_PIN_BUSY:
                case SX128x::GPIO_PIN_DIO1:
                case SX128x::GPIO_PIN_DIO2:
                case SX128x::GPIO_PIN_DIO3:
                    throwRuntimeError("attempted to write to an input GPIO line");
            }
        }

        LinuxSx1280RadioConfig config{};

        int spi_fd{-1};
        gpiod_chip* gpio_chip{nullptr};

        gpiod_line_request* reset_request{nullptr};
        gpiod_line_request* busy_request{nullptr};
        gpiod_line_request* dio1_request{nullptr};
        gpiod_line_request* dio2_request{nullptr};
        gpiod_line_request* dio3_request{nullptr};
        gpiod_line_request* tx_enable_request{nullptr};
    };

    LinuxSx1280Radio::LinuxSx1280Radio(const LinuxSx1280RadioConfig& config)
        : m_impl(std::make_unique<Impl>(config)) {}

    LinuxSx1280Radio::~LinuxSx1280Radio() = default;

    const LinuxSx1280RadioConfig& LinuxSx1280Radio::config() const noexcept {
        return m_impl->config;
    }

    std::uint8_t LinuxSx1280Radio::HalGpioRead(GpioPinFunction_t func) {
        return m_impl->readInputLine(func);
    }

    void LinuxSx1280Radio::HalGpioWrite(GpioPinFunction_t func, std::uint8_t value) {
        m_impl->writeOutputLine(func, value);
    }

    void LinuxSx1280Radio::HalSpiTransfer(std::uint8_t* buffer_in,
                                          const std::uint8_t* buffer_out,
                                          std::uint16_t size) {
        spi_ioc_transfer transfer{};
        transfer.tx_buf = reinterpret_cast<unsigned long>(buffer_out);
        transfer.rx_buf = reinterpret_cast<unsigned long>(buffer_in);
        transfer.len = size;
        transfer.speed_hz = m_impl->config.spi.speed_hz;
        transfer.bits_per_word = m_impl->config.spi.bits_per_word;
        transfer.delay_usecs = 0;

        if (::ioctl(m_impl->spi_fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
            throwSystemError("failed to perform SPI transfer");
        }
    }

    void LinuxSx1280Radio::HalPreTx() {
        switch (m_impl->config.pins.tx_enable_behavior) {
            case TxEnableBehavior::None:
            case TxEnableBehavior::StaticAsserted:
            case TxEnableBehavior::StaticDeasserted:
                break;

            case TxEnableBehavior::AssertDuringTxOnly:
                m_impl->setTxEnable(true);
                break;
        }
    }

    void LinuxSx1280Radio::HalPreRx() {
        switch (m_impl->config.pins.tx_enable_behavior) {
            case TxEnableBehavior::None:
            case TxEnableBehavior::StaticAsserted:
            case TxEnableBehavior::StaticDeasserted:
                break;

            case TxEnableBehavior::AssertDuringTxOnly:
                m_impl->setTxEnable(false);
                break;
        }
    }

    void LinuxSx1280Radio::HalPostTx() {
        switch (m_impl->config.pins.tx_enable_behavior) {
            case TxEnableBehavior::None:
                break;

            case TxEnableBehavior::StaticAsserted:
                m_impl->setTxEnable(true);
                break;

            case TxEnableBehavior::StaticDeasserted:
                m_impl->setTxEnable(false);
                break;

            case TxEnableBehavior::AssertDuringTxOnly:
                m_impl->setTxEnable(false);
                break;
        }
    }

    void LinuxSx1280Radio::HalPostRx() {
        switch (m_impl->config.pins.tx_enable_behavior) {
            case TxEnableBehavior::None:
                break;

            case TxEnableBehavior::StaticAsserted:
                m_impl->setTxEnable(true);
                break;

            case TxEnableBehavior::StaticDeasserted:
            case TxEnableBehavior::AssertDuringTxOnly:
                m_impl->setTxEnable(false);
                break;
        }
    }

    std::uint32_t LinuxSx1280Radio::pinFunctionToLine(GpioPinFunction_t func) const {
        switch (func) {
            case SX128x::GPIO_PIN_RESET:
                return m_impl->config.pins.reset_line;

            case SX128x::GPIO_PIN_BUSY:
                return m_impl->config.pins.busy_line;

            case SX128x::GPIO_PIN_DIO1:
                return m_impl->config.pins.dio1_line;

            case SX128x::GPIO_PIN_DIO2:
                return m_impl->config.pins.dio2_line;

            case SX128x::GPIO_PIN_DIO3:
                return m_impl->config.pins.dio3_line;
        }

        return 0;
    }

    bool LinuxSx1280Radio::isPinConfigured(GpioPinFunction_t func) const {
        switch (func) {
            case SX128x::GPIO_PIN_RESET:
            case SX128x::GPIO_PIN_BUSY:
            
            case SX128x::GPIO_PIN_DIO1:
                return m_impl->config.pins.has_dio1;
                
            case SX128x::GPIO_PIN_DIO2:
                return m_impl->config.pins.has_dio2;

            case SX128x::GPIO_PIN_DIO3:
                return m_impl->config.pins.has_dio3;
        }

        return false;
    }

}  // namespace Sx1280Radio