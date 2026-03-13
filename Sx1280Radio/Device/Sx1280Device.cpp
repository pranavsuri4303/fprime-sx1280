// Sx1280Radio/Device/include/Sx1280Device.cpp
#include "Sx1280Device.hpp"

#include <utility>

namespace Sx1280Radio {

    Sx1280Device::Sx1280Device(std::unique_ptr<LinuxSx1280Radio> radio)
        : m_radio(std::move(radio)) {}

    Sx1280Device::~Sx1280Device() = default;

    Sx1280Device::Sx1280Device(Sx1280Device&& other) noexcept
        : m_radio(std::move(other.m_radio))
        , m_config(other.m_config)
        , m_listener(other.m_listener)
        , m_state(other.m_state)
        , m_initialized(other.m_initialized) {
        if (m_radio) {
            bindVendorCallbacks();
        }

        other.m_listener = nullptr;
        other.m_state = Sx1280DeviceState::Uninitialized;
        other.m_initialized = false;
    }

    Sx1280Device& Sx1280Device::operator=(Sx1280Device&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        m_radio = std::move(other.m_radio);
        m_config = other.m_config;
        m_listener = other.m_listener;
        m_state = other.m_state;
        m_initialized = other.m_initialized;

        if (m_radio) {
            bindVendorCallbacks();
        }

        other.m_listener = nullptr;
        other.m_state = Sx1280DeviceState::Uninitialized;
        other.m_initialized = false;

        return *this;
    }

    Sx1280DeviceError Sx1280Device::init() {
        if (!m_radio) {
            return Sx1280DeviceError::InvalidConfiguration;
        }

        if (m_initialized) {
            return Sx1280DeviceError::None;
        }

        bindVendorCallbacks();
        m_radio->Init();

        m_initialized = true;
        m_state = Sx1280DeviceState::Idle;

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::configure(const Sx1280Config& config) {
        if (!m_initialized) {
            return Sx1280DeviceError::NotInitialized;
        }

        if (config.lora.payload_length == 0) {
            return Sx1280DeviceError::InvalidConfiguration;
        }

        auto result = applyRfConfig(config.rf);
        if (result != Sx1280DeviceError::None) {
            m_state = Sx1280DeviceState::Error;
            return result;
        }

        result = applyLoRaConfig(config.lora);
        if (result != Sx1280DeviceError::None) {
            m_state = Sx1280DeviceState::Error;
            return result;
        }

        result = applyRuntimeConfig(config.runtime);
        if (result != Sx1280DeviceError::None) {
            m_state = Sx1280DeviceState::Error;
            return result;
        }

        m_config = config;
        m_state = Sx1280DeviceState::Idle;

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::startRx(Sx1280ReceiveMode mode) {
        if (!m_initialized) {
            return Sx1280DeviceError::NotInitialized;
        }

        if (m_radio->config().pins.role == RadioRole::TxOnly) {
            return Sx1280DeviceError::RadioOperationFailed;
        }

        const auto timeout = (mode == Sx1280ReceiveMode::Continuous)
            ? m_radio->RX_TX_CONTINUOUS
            : currentRxTimeout();

        m_radio->SetRx(timeout);
        m_state = Sx1280DeviceState::Receiving;

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::stop() {
        if (!m_initialized) {
            return Sx1280DeviceError::NotInitialized;
        }

        m_radio->SetStandby(SX128x::STDBY_RC);
        m_state = Sx1280DeviceState::Idle;

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::send(std::span<const std::uint8_t> payload) {
        if (!m_initialized) {
            return Sx1280DeviceError::NotInitialized;
        }

        if (m_radio->config().pins.role == RadioRole::RxOnly) {
            return Sx1280DeviceError::RadioOperationFailed;
        }

        if (!canSend(payload.size())) {
            return Sx1280DeviceError::PayloadTooLarge;
        }

        if (m_state == Sx1280DeviceState::Transmitting) {
            return Sx1280DeviceError::Busy;
        }

        std::vector<std::uint8_t> tx_buffer(payload.begin(), payload.end());

        auto tx_packet_params = m_config.lora.to_packet_params();
        tx_packet_params.Params.LoRa.PayloadLength =
            static_cast<std::uint8_t>(tx_buffer.size());

        m_radio->SetPacketParams(tx_packet_params);

        m_radio->SendPayload(
            tx_buffer.data(),
            static_cast<std::uint8_t>(tx_buffer.size()),
            m_config.timeouts.tx_timeout
        );

        m_state = Sx1280DeviceState::Transmitting;
        return Sx1280DeviceError::None;
    }

    void Sx1280Device::processIrqs() {
        if (!m_initialized || !m_radio) {
            return;
        }

        m_radio->ProcessIrqs();
    }

    void Sx1280Device::setListener(Sx1280DeviceListener* listener) noexcept {
        m_listener = listener;
    }

    bool Sx1280Device::isInitialized() const noexcept {
        return m_initialized;
    }

    Sx1280DeviceState Sx1280Device::state() const noexcept {
        return m_state;
    }

    const Sx1280Config& Sx1280Device::config() const noexcept {
        return m_config;
    }

    LinuxSx1280Radio& Sx1280Device::radio() noexcept {
        return *m_radio;
    }

    const LinuxSx1280Radio& Sx1280Device::radio() const noexcept {
        return *m_radio;
    }

    void Sx1280Device::bindVendorCallbacks() {
        if (!m_radio) {
            return;
        }

        m_radio->callbacks.txDone = [this]() {
            handleTxDone();
        };

        m_radio->callbacks.rxDone = [this]() {
            handleRxDone();
        };

        m_radio->callbacks.rxTimeout = [this]() {
            handleRxTimeout();
        };

        m_radio->callbacks.txTimeout = [this]() {
            handleTxTimeout();
        };

        m_radio->callbacks.rxError = [this](SX128x::IrqErrorCode_t error_code) {
            handleRxError(error_code);
        };
    }

    void Sx1280Device::handleTxDone() {
        m_state = Sx1280DeviceState::Idle;

        if (m_listener) {
            m_listener->onTxDone();
        }
    }

    void Sx1280Device::handleRxDone() {
        if (!m_radio) {
            m_state = Sx1280DeviceState::Error;
            return;
        }

        SX128x::PacketStatus_t packet_status{};
        m_radio->GetPacketStatus(&packet_status);

        auto packet = Sx1280RxPacket{};
        packet.payload.resize(m_config.lora.payload_length);

        std::uint8_t payload_size = 0;
        const auto get_payload_result = m_radio->GetPayload(
            packet.payload.data(),
            &payload_size,
            static_cast<std::uint8_t>(packet.payload.size())
        );

        if (get_payload_result != 0) {
            m_state = Sx1280DeviceState::Error;
            return;
        }

        packet.payload.resize(payload_size);
        packet.crc_ok = true;

        switch (packet_status.packetType) {
            case SX128x::PACKET_TYPE_LORA:
            case SX128x::PACKET_TYPE_RANGING:
                packet.rssi_dbm = packet_status.LoRa.RssiPkt;
                packet.snr_db = packet_status.LoRa.SnrPkt;
                break;

            case SX128x::PACKET_TYPE_GFSK:
                packet.rssi_dbm = packet_status.Gfsk.RssiSync;
                packet.snr_db = 0;
                break;

            case SX128x::PACKET_TYPE_FLRC:
                packet.rssi_dbm = packet_status.Flrc.RssiSync;
                packet.snr_db = 0;
                break;

            case SX128x::PACKET_TYPE_BLE:
                packet.rssi_dbm = packet_status.Ble.RssiSync;
                packet.snr_db = 0;
                break;

            case SX128x::PACKET_TYPE_NONE:
            default:
                packet.rssi_dbm = 0;
                packet.snr_db = 0;
                break;
        }

        m_state = (m_radio->GetOpMode() == SX128x::MODE_RX)
            ? Sx1280DeviceState::Receiving
            : Sx1280DeviceState::Idle;

        if (m_listener) {
            m_listener->onRxDone(packet);
        }
    }

    void Sx1280Device::handleRxTimeout() {
        m_state = Sx1280DeviceState::Idle;

        if (m_listener) {
            m_listener->onRxTimeout();
        }
    }

    void Sx1280Device::handleTxTimeout() {
        m_state = Sx1280DeviceState::Idle;

        if (m_listener) {
            m_listener->onTxTimeout();
        }
    }

    void Sx1280Device::handleRxError(SX128x::IrqErrorCode_t error_code) {
        m_state = Sx1280DeviceState::Error;

        if (m_listener) {
            m_listener->onRxError(error_code);
        }
    }

    Sx1280DeviceError Sx1280Device::applyRfConfig(const Sx1280RfConfig& config) {
        if (!m_radio) {
            return Sx1280DeviceError::InvalidConfiguration;
        }

        m_radio->SetRegulatorMode(config.regulator_mode);
        m_radio->SetRfFrequency(config.frequency_hz);
        m_radio->SetTxParams(config.tx_power_dbm, config.ramp_time);

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::applyLoRaConfig(const Sx1280LoRaConfig& config) {
        if (!m_radio) {
            return Sx1280DeviceError::InvalidConfiguration;
        }

        m_radio->SetPacketType(SX128x::PACKET_TYPE_LORA);
        m_radio->SetBufferBaseAddresses(0x00, 0x00);
        m_radio->SetModulationParams(config.to_modulation_params());
        m_radio->SetPacketParams(config.to_packet_params());

        return Sx1280DeviceError::None;
    }

    Sx1280DeviceError Sx1280Device::applyRuntimeConfig(const Sx1280RuntimeConfig& config) {
        if (!m_radio) {
            return Sx1280DeviceError::InvalidConfiguration;
        }

        m_radio->SetAutoFs(config.auto_fs);
        m_radio->SetLongPreamble(config.long_preamble);
        m_radio->SetLNAGainSetting(config.lna_setting);

        if (config.use_manual_gain) {
            m_radio->EnableManualGain();
            m_radio->SetManualGainValue(config.manual_gain_value);
        } else {
            m_radio->DisableManualGain();
        }

        return Sx1280DeviceError::None;
    }

    SX128x::TickTime_t Sx1280Device::currentRxTimeout() const noexcept {
        return m_config.timeouts.rx_timeout;
    }

    bool Sx1280Device::canSend(std::size_t payload_size) const noexcept {
        return payload_size > 0 &&
               payload_size <= m_config.lora.payload_length;
    }

}  // namespace Sx1280Radio