// File: Sx1280Radio/Components/LoraRadioManager/include/LoraRadioManager.hpp
#pragma once

#include <memory>

#include "Sx1280Radio/Components/LoraRadioManager/LoraRadioManagerComponentAc.hpp"
#include "Sx1280Radio/Device/include/Sx1280Config.hpp"
#include "Sx1280Radio/Device/include/Sx1280Device.hpp"

namespace Sx1280Radio {

    class LoraRadioManager final : public LoraRadioManagerComponentBase {
      public:
        explicit LoraRadioManager(const char* const compName);
        ~LoraRadioManager() override;

        void init(NATIVE_INT_TYPE instance = 0);

      private:
        class TxListener;
        class RxListener;

      private:
        void configureDefaultRadioConfig();
        bool createDevices();
        bool startDevices();
        void stopDevices();

        void emitInitialReadyIfNeeded();
        void emitRecoveredReadyIfNeeded();
        void emitTransmitStatus(Fw::Success::T status);

        void handleTxDone();
        void handleTxTimeout();
        void handleRxDone(const Sx1280RxPacket& packet);
        void handleRxTimeout();
        void handleRxError(SX128x::IrqErrorCode_t errorCode);

        void returnDataInBuffer(Fw::Buffer& data, const ComCfg::FrameContext& context);
        void updateLinkRunningTlm();

      private:
        void dataIn_handler(
            FwIndexType portNum,
            Fw::Buffer& data,
            const ComCfg::FrameContext& context
        ) override;

        void dataReturnIn_handler(
            FwIndexType portNum,
            Fw::Buffer& data,
            const ComCfg::FrameContext& context
        ) override;

        void schedIn_handler(
            FwIndexType portNum,
            U32 context
        ) override;

        void START_cmdHandler(
            FwOpcodeType opCode,
            U32 cmdSeq
        ) override;

        void STOP_cmdHandler(
            FwOpcodeType opCode,
            U32 cmdSeq
        ) override;

      private:
        Sx1280Config m_radioConfig{};

        std::unique_ptr<Sx1280Device> m_txDevice;
        std::unique_ptr<Sx1280Device> m_rxDevice;

        std::unique_ptr<TxListener> m_txListener;
        std::unique_ptr<RxListener> m_rxListener;

        bool m_started{false};
        bool m_initialReadySent{false};
        bool m_recoveryReadyPending{false};

        U32 m_txFrames{0};
        U32 m_rxFrames{0};
        U32 m_txFailures{0};
        U32 m_rxFailures{0};
        U32 m_lastTxBytes{0};
        U32 m_lastRxBytes{0};
    };

}  // namespace Sx1280Radio