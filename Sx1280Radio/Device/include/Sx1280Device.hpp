// Sx1280Radio/Device/include/Sx1280Device.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Sx1280Config.hpp"
#include "LinuxSx1280Radio.hpp"

namespace Sx1280Radio {

    enum class Sx1280DeviceState {
        Uninitialized,
        Idle,
        Receiving,
        Transmitting,
        Error,
    };

    enum class Sx1280ReceiveMode {
        Single,
        Continuous,
    };

    enum class Sx1280DeviceError {
        None,
        NotInitialized,
        InvalidConfiguration,
        PayloadTooLarge,
        RadioOperationFailed,
        ReceiveFailed,
        Timeout,
        Busy,
    };

    struct Sx1280RxPacket {
        std::vector<std::uint8_t> payload{};
        std::int16_t rssi_dbm{0};
        std::int16_t snr_db{0};
        bool crc_ok{true};
    };

    class Sx1280DeviceListener {
    public:
        virtual ~Sx1280DeviceListener() = default;

        virtual void onTxDone() {}
        virtual void onRxDone(const Sx1280RxPacket&) {}
        virtual void onRxTimeout() {}
        virtual void onTxTimeout() {}
        virtual void onRxError(SX128x::IrqErrorCode_t) {}
    };

    class Sx1280Device {
    public:
        explicit Sx1280Device(std::unique_ptr<LinuxSx1280Radio> radio);
        ~Sx1280Device();

        Sx1280Device(const Sx1280Device&) = delete;
        Sx1280Device& operator=(const Sx1280Device&) = delete;
        Sx1280Device(Sx1280Device&&) noexcept;
        Sx1280Device& operator=(Sx1280Device&&) noexcept;

        Sx1280DeviceError init();
        Sx1280DeviceError configure(const Sx1280Config& config);

        Sx1280DeviceError startRx(Sx1280ReceiveMode mode);
        Sx1280DeviceError stop();

        Sx1280DeviceError send(const std::uint8_t* payload, std::size_t size);
        
        void processIrqs();

        void setListener(Sx1280DeviceListener* listener) noexcept;

        [[nodiscard]] bool isInitialized() const noexcept;
        [[nodiscard]] Sx1280DeviceState state() const noexcept;
        [[nodiscard]] const Sx1280Config& config() const noexcept;
        [[nodiscard]] LinuxSx1280Radio& radio() noexcept;
        [[nodiscard]] const LinuxSx1280Radio& radio() const noexcept;

    private:
        void bindVendorCallbacks();
        void handleTxDone();
        void handleRxDone();
        void handleRxTimeout();
        void handleTxTimeout();
        void handleRxError(SX128x::IrqErrorCode_t error_code);

        Sx1280DeviceError applyRfConfig(const Sx1280RfConfig& config);
        Sx1280DeviceError applyLoRaConfig(const Sx1280LoRaConfig& config);
        Sx1280DeviceError applyRuntimeConfig(const Sx1280RuntimeConfig& config);

        [[nodiscard]] SX128x::TickTime_t currentRxTimeout() const noexcept;
        [[nodiscard]] bool canSend(std::size_t payload_size) const noexcept;

    private:
        std::unique_ptr<LinuxSx1280Radio> m_radio;
        Sx1280Config m_config{};
        Sx1280DeviceListener* m_listener{nullptr};
        Sx1280DeviceState m_state{Sx1280DeviceState::Uninitialized};
        bool m_initialized{false};
    };

}  // namespace Sx1280Radio