// File: Sx1280Radio/Components/LoraRadioManager/LoraRadioManager.cpp

#include "Sx1280Radio/Components/LoraRadioManager/include/LoraRadioManager.hpp"

#include <memory>
#include <span>
#include <utility>

namespace Sx1280Radio {

    namespace {

        LinuxSx1280RadioConfig makeRxHwConfig() {
            LinuxSx1280RadioConfig config{};

            config.spi.device_path = "/dev/spidev0.0";
            config.spi.speed_hz = 1'000'000;
            config.spi.mode = 0;
            config.spi.bits_per_word = 8;

            config.gpio.chip_path = "/dev/gpiochip4";

            config.pins.reset_line = 22;
            config.pins.busy_line = 23;
            config.pins.has_dio1 = true;
            config.pins.dio1_line = 24;
            config.pins.has_dio2 = false;
            config.pins.has_dio3 = false;

            config.pins.has_tx_enable = false;
            config.pins.role = RadioRole::RxOnly;
            config.pins.tx_enable_behavior = TxEnableBehavior::None;

            return config;
        }

        LinuxSx1280RadioConfig makeTxHwConfig() {
            LinuxSx1280RadioConfig config{};

            config.spi.device_path = "/dev/spidev0.1";
            config.spi.speed_hz = 1'000'000;
            config.spi.mode = 0;
            config.spi.bits_per_word = 8;

            config.gpio.chip_path = "/dev/gpiochip4";

            config.pins.reset_line = 5;
            config.pins.busy_line = 6;
            config.pins.has_dio1 = false;
            config.pins.dio1_line = 0;
            config.pins.has_dio2 = false;
            config.pins.has_dio3 = false;

            config.pins.has_tx_enable = true;
            config.pins.tx_enable_line = 12;
            config.pins.tx_enable_active_high = true;
            config.pins.role = RadioRole::TxOnly;
            config.pins.tx_enable_behavior = TxEnableBehavior::AssertDuringTxOnly;

            return config;
        }

    }  // namespace

    class LoraRadioManager::TxListener final : public Sx1280DeviceListener {
      public:
        explicit TxListener(LoraRadioManager& owner)
        : m_owner(owner) {}

        void onTxDone() override {
            m_owner.handleTxDone();
        }

        void onTxTimeout() override {
            m_owner.handleTxTimeout();
        }

      private:
        LoraRadioManager& m_owner;
    };

    class LoraRadioManager::RxListener final : public Sx1280DeviceListener {
      public:
        explicit RxListener(LoraRadioManager& owner)
        : m_owner(owner) {}

        void onRxDone(const Sx1280RxPacket& packet) override {
            m_owner.handleRxDone(packet);
        }

        void onRxTimeout() override {
            m_owner.handleRxTimeout();
        }

        void onRxError(SX128x::IrqErrorCode_t errorCode) override {
            m_owner.handleRxError(errorCode);
        }

      private:
        LoraRadioManager& m_owner;
    };

    LoraRadioManager::LoraRadioManager(const char* const compName)
    : LoraRadioManagerComponentBase(compName) {}

    LoraRadioManager::~LoraRadioManager() = default;

    void LoraRadioManager::init(NATIVE_INT_TYPE instance) {
        LoraRadioManagerComponentBase::init(instance);

        this->configureDefaultRadioConfig();

        this->m_txListener = std::make_unique<TxListener>(*this);
        this->m_rxListener = std::make_unique<RxListener>(*this);

        this->updateLinkRunningTlm();
        this->tlmWrite_TxFrames(this->m_txFrames);
        this->tlmWrite_RxFrames(this->m_rxFrames);
        this->tlmWrite_TxFailures(this->m_txFailures);
        this->tlmWrite_RxFailures(this->m_rxFailures);
        this->tlmWrite_LastTxBytes(this->m_lastTxBytes);
        this->tlmWrite_LastRxBytes(this->m_lastRxBytes);
    }

    void LoraRadioManager::configureDefaultRadioConfig() {
        this->m_radioConfig.rf.frequency_hz = 2400000000U;
        this->m_radioConfig.rf.tx_power_dbm = 10;
        this->m_radioConfig.rf.ramp_time = SX128x::RADIO_RAMP_20_US;
        this->m_radioConfig.rf.regulator_mode = SX128x::USE_DCDC;

        this->m_radioConfig.lora.spreading_factor = SX128x::LORA_SF7;
        this->m_radioConfig.lora.bandwidth = SX128x::LORA_BW_0800;
        this->m_radioConfig.lora.coding_rate = SX128x::LORA_CR_4_5;
        this->m_radioConfig.lora.preamble_length = 12;
        this->m_radioConfig.lora.header_type = SX128x::LORA_PACKET_VARIABLE_LENGTH;
        this->m_radioConfig.lora.payload_length = 255;
        this->m_radioConfig.lora.crc = SX128x::LORA_CRC_ON;
        this->m_radioConfig.lora.iq_mode = SX128x::LORA_IQ_NORMAL;

        this->m_radioConfig.timeouts.tx_timeout = {
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };

        this->m_radioConfig.timeouts.rx_timeout = {
            SX128x::RADIO_TICK_SIZE_1000_US,
            2000
        };

        this->m_radioConfig.timeouts.continuous_rx = true;

        this->m_radioConfig.runtime.auto_fs = true;
        this->m_radioConfig.runtime.long_preamble = false;
        this->m_radioConfig.runtime.use_manual_gain = false;
        this->m_radioConfig.runtime.manual_gain_value = 0;
        this->m_radioConfig.runtime.lna_setting = SX128x::LNA_HIGH_SENSITIVITY_MODE;
    }

    bool LoraRadioManager::createDevices() {
        if (this->m_txDevice && this->m_rxDevice) {
            return true;
        }

        try {
            auto txRadio = std::make_unique<LinuxSx1280Radio>(makeTxHwConfig());
            auto rxRadio = std::make_unique<LinuxSx1280Radio>(makeRxHwConfig());

            this->m_txDevice = std::make_unique<Sx1280Device>(std::move(txRadio));
            this->m_rxDevice = std::make_unique<Sx1280Device>(std::move(rxRadio));

            this->m_txDevice->setListener(this->m_txListener.get());
            this->m_rxDevice->setListener(this->m_rxListener.get());

            return true;
        } catch (...) {
            this->m_txDevice.reset();
            this->m_rxDevice.reset();
            return false;
        }
    }

    bool LoraRadioManager::startDevices() {
        if (!this->m_txDevice || !this->m_rxDevice) {
            return false;
        }

        if (this->m_txDevice->init() != Sx1280DeviceError::None) {
            return false;
        }

        if (this->m_rxDevice->init() != Sx1280DeviceError::None) {
            return false;
        }

        if (this->m_txDevice->configure(this->m_radioConfig) != Sx1280DeviceError::None) {
            return false;
        }

        if (this->m_rxDevice->configure(this->m_radioConfig) != Sx1280DeviceError::None) {
            return false;
        }

        if (this->m_rxDevice->startRx(Sx1280ReceiveMode::Continuous) != Sx1280DeviceError::None) {
            return false;
        }

        return true;
    }

    void LoraRadioManager::stopDevices() {
        if (this->m_rxDevice) {
            static_cast<void>(this->m_rxDevice->stop());
        }

        if (this->m_txDevice) {
            static_cast<void>(this->m_txDevice->stop());
        }

        this->m_started = false;
        this->updateLinkRunningTlm();
    }

    void LoraRadioManager::emitInitialReadyIfNeeded() {
        if (this->m_initialReadySent) {
            return;
        }

        if (this->isConnected_comStatusOut_OutputPort(0)) {
            this->comStatusOut_out(0, Fw::Success::SUCCESS);
        }

        this->m_initialReadySent = true;
    }

    void LoraRadioManager::emitRecoveredReadyIfNeeded() {
        if (!this->m_recoveryReadyPending) {
            return;
        }

        this->emitTransmitStatus(Fw::Success::SUCCESS);
        this->m_recoveryReadyPending = false;
    }

    void LoraRadioManager::emitTransmitStatus(Fw::Success::T status) {
        if (this->isConnected_comStatusOut_OutputPort(0)) {
            this->comStatusOut_out(0, status);
        }
    }

    void LoraRadioManager::handleTxDone() {
        ++this->m_txFrames;
        this->tlmWrite_TxFrames(this->m_txFrames);

        this->emitTransmitStatus(Fw::Success::SUCCESS);
    }

    void LoraRadioManager::handleTxTimeout() {
        ++this->m_txFailures;
        this->tlmWrite_TxFailures(this->m_txFailures);

        this->log_WARNING_LO_TxFailed();
        this->emitTransmitStatus(Fw::Success::FAILURE);
        this->m_recoveryReadyPending = true;
    }

    void LoraRadioManager::handleRxDone(const Sx1280RxPacket& packet) {
        ++this->m_rxFrames;
        this->m_lastRxBytes = static_cast<U32>(packet.payload.size());

        this->tlmWrite_RxFrames(this->m_rxFrames);
        this->tlmWrite_LastRxBytes(this->m_lastRxBytes);

        // TODO:
        // Allocate or source an Fw::Buffer for the received payload,
        // populate it from packet.payload, emit it on dataOut, and
        // reclaim ownership later through dataReturnIn.
        //
        // This is the one major remaining piece for full Communication
        // Adapter behavior on the RX path.
    }

    void LoraRadioManager::handleRxTimeout() {
        // Continuous RX does not treat timeouts as a hard failure in this first version.
    }

    void LoraRadioManager::handleRxError(SX128x::IrqErrorCode_t errorCode) {
        static_cast<void>(errorCode);

        ++this->m_rxFailures;
        this->tlmWrite_RxFailures(this->m_rxFailures);

        this->log_WARNING_LO_RxFailed();
    }

    void LoraRadioManager::returnDataInBuffer(Fw::Buffer& data, const ComCfg::FrameContext& context) {
        if (this->isConnected_dataReturnOut_OutputPort(0)) {
            this->dataReturnOut_out(0, data, context);
        }
    }

    void LoraRadioManager::updateLinkRunningTlm() {
        this->tlmWrite_LinkRunning(this->m_started ? 1U : 0U);
    }

    void LoraRadioManager::dataIn_handler(
        FwIndexType portNum,
        Fw::Buffer& data,
        const ComCfg::FrameContext& context
    ) {
        static_cast<void>(portNum);

        if (!this->m_started || !this->m_txDevice) {
            this->returnDataInBuffer(data, context);

            ++this->m_txFailures;
            this->tlmWrite_TxFailures(this->m_txFailures);

            this->log_WARNING_LO_TxFailed();
            this->emitTransmitStatus(Fw::Success::FAILURE);
            this->m_recoveryReadyPending = true;
            return;
        }

        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data.getData());
        const auto size = static_cast<std::size_t>(data.getSize());

        this->m_lastTxBytes = static_cast<U32>(size);
        this->tlmWrite_LastTxBytes(this->m_lastTxBytes);

        const auto result = this->m_txDevice->send(
            std::span<const std::uint8_t>(bytes, size)
        );

        this->returnDataInBuffer(data, context);

        if (result != Sx1280DeviceError::None) {
            ++this->m_txFailures;
            this->tlmWrite_TxFailures(this->m_txFailures);

            this->log_WARNING_LO_TxFailed();
            this->emitTransmitStatus(Fw::Success::FAILURE);
            this->m_recoveryReadyPending = true;
        }
    }

    void LoraRadioManager::dataReturnIn_handler(
        FwIndexType portNum,
        Fw::Buffer& data,
        const ComCfg::FrameContext& context
    ) {
        static_cast<void>(portNum);
        static_cast<void>(data);
        static_cast<void>(context);

        // TODO:
        // Reclaim ownership of buffers emitted on dataOut once the RX path
        // is implemented with explicit buffer allocation / sourcing.
    }

    void LoraRadioManager::schedIn_handler(
        FwIndexType portNum,
        U32 context
    ) {
        static_cast<void>(portNum);
        static_cast<void>(context);

        if (!this->m_started) {
            return;
        }

        if (this->m_rxDevice) {
            this->m_rxDevice->processIrqs();
        }

        if (this->m_txDevice) {
            this->m_txDevice->processIrqs();
        }

        this->emitRecoveredReadyIfNeeded();
    }

    void LoraRadioManager::START_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) {
        if (this->m_started) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
            return;
        }

        if (!this->createDevices() || !this->startDevices()) {
            this->log_WARNING_HI_LinkStartFailed();
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        this->m_started = true;
        this->updateLinkRunningTlm();

        this->log_ACTIVITY_HI_LinkStarted();
        this->emitInitialReadyIfNeeded();

        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void LoraRadioManager::STOP_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) {
        if (this->m_started) {
            this->stopDevices();
            this->log_ACTIVITY_HI_LinkStopped();
        }

        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

}  // namespace Sx1280Radio