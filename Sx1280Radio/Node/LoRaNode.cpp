/*
 * LoRaNode.cpp
 *
 * Created on: Mar 14, 2026
 * Author: Pranav
 */

#include "LoRaNode.hpp"

#include <utility>

#include "LinuxSx1280Radio.hpp"

namespace Sx1280Radio {

    struct LoRaNode::Impl {
        NodeConfig config;
        RxCallback rx_callback{};

        std::unique_ptr<LinuxSx1280Radio> rx_radio;
        std::unique_ptr<LinuxSx1280Radio> tx_radio;
        bool started{false};

        explicit Impl(const NodeConfig& cfg)
            : config(cfg)
        {
        }
    };

    LoRaNode::LoRaNode(const NodeConfig& config)
        : m_impl(std::make_unique<Impl>(config))
    {
    }

    LoRaNode::~LoRaNode() = default;

    bool LoRaNode::start() {
        if (m_impl->started) {
            return true;
        }

        try {
            const auto& node_cfg = m_impl->config;
            const auto& radio_cfg = node_cfg.radio;

            auto rx = std::make_unique<LinuxSx1280Radio>(node_cfg.rx.hw);
            auto tx = std::make_unique<LinuxSx1280Radio>(node_cfg.tx.hw);

            auto configure_radio = [&radio_cfg](LinuxSx1280Radio& radio) {
                radio.Init();

                radio.SetStandby(SX128x::STDBY_RC);
                radio.SetRegulatorMode(radio_cfg.rf.regulator_mode);
                radio.SetPacketType(SX128x::PACKET_TYPE_LORA);

                const auto mod_params = radio_cfg.lora.to_modulation_params();
                const auto pkt_params = radio_cfg.lora.to_packet_params();
                radio.SetModulationParams(mod_params);
                radio.SetPacketParams(pkt_params);

                radio.SetRfFrequency(radio_cfg.rf.frequency_hz);
                radio.SetTxParams(radio_cfg.rf.tx_power_dbm, radio_cfg.rf.ramp_time);
                radio.SetBufferBaseAddresses(0x00, 0x00);

                radio.SetAutoFs(radio_cfg.runtime.auto_fs);
                radio.SetLongPreamble(radio_cfg.runtime.long_preamble);

                if (radio_cfg.runtime.use_manual_gain) {
                    radio.EnableManualGain();
                    radio.SetManualGainValue(radio_cfg.runtime.manual_gain_value);
                } else {
                    radio.DisableManualGain();
                }

                radio.SetLNAGainSetting(radio_cfg.runtime.lna_setting);
            };

            configure_radio(*rx);
            configure_radio(*tx);

            // Configure IRQ callbacks for RX radio so poll() can deliver data
            rx->callbacks.rxDone = [this]() {
                if (!this->m_impl->rx_radio) {
                    return;
                }

                SX128x::PacketStatus_t status{};
                this->m_impl->rx_radio->GetPacketStatus(&status);

                std::uint8_t length = 0;
                std::uint8_t offset = 0;
                this->m_impl->rx_radio->GetRxBufferStatus(&length, &offset);

                std::vector<std::uint8_t> buffer(length);
                if (length > 0) {
                    this->m_impl->rx_radio->ReadBuffer(offset, buffer.data(), length);
                }

                LoRaNodeRxPacket pkt{};
                pkt.payload = std::move(buffer);

                if (status.packetType == SX128x::PACKET_TYPE_LORA) {
                    pkt.rssi_dbm = status.LoRa.RssiPkt;
                    pkt.snr_db = status.LoRa.SnrPkt;
                }

                auto cb = this->m_impl->rx_callback;
                if (cb) {
                    cb(pkt);
                }
            };

            m_impl->rx_radio = std::move(rx);
            m_impl->tx_radio = std::move(tx);
            // Configure basic IRQ routing similar to the legacy smoke tests.
            const std::uint16_t irq_mask =
                SX128x::IRQ_TX_DONE |
                SX128x::IRQ_RX_DONE |
                SX128x::IRQ_RX_TX_TIMEOUT |
                SX128x::IRQ_CRC_ERROR |
                SX128x::IRQ_HEADER_ERROR |
                SX128x::IRQ_HEADER_VALID;

            m_impl->rx_radio->SetDioIrqParams(irq_mask, irq_mask, 0, 0);
            m_impl->tx_radio->SetDioIrqParams(irq_mask, 0, 0, 0);

            // Start RX according to the configured timeout mode.
            SX128x::TickTime_t rx_timeout = radio_cfg.timeouts.rx_timeout;
            if (radio_cfg.timeouts.continuous_rx) {
                rx_timeout.PeriodBaseCount = 0xFFFFu;
            }
            m_impl->rx_radio->SetRx(rx_timeout);

            m_impl->started = true;
            return true;
        } catch (...) {
            m_impl->rx_radio.reset();
            m_impl->tx_radio.reset();
            m_impl->started = false;
            return false;
        }
    }

    void LoRaNode::stop() {
        m_impl->rx_radio.reset();
        m_impl->tx_radio.reset();
        m_impl->started = false;
    }

    bool LoRaNode::send(const std::vector<std::uint8_t>& payload) {
        if (!m_impl->started || !m_impl->tx_radio) {
            return false;
        }

        if (payload.empty()) {
            return true;
        }

        if (payload.size() > 0xFFu) {
            return false;
        }

        auto timeout = m_impl->config.radio.timeouts.tx_timeout;
        auto* data = const_cast<std::uint8_t*>(payload.data());
        m_impl->tx_radio->SendPayload(
            data,
            static_cast<std::uint8_t>(payload.size()),
            timeout
        );

        return true;
    }

    void LoRaNode::poll() {
        if (m_impl->rx_radio) {
            m_impl->rx_radio->ProcessIrqs();
        }
        if (m_impl->tx_radio) {
            m_impl->tx_radio->ProcessIrqs();
        }
    }

    void LoRaNode::setRxCallback(RxCallback cb) {
        m_impl->rx_callback = std::move(cb);
    }

} // namespace Sx1280Radio
