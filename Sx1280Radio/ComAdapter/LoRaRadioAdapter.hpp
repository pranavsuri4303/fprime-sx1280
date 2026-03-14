/*
 * LoRaRadioAdapter.hpp
 *
 * Created on: Mar 15, 2026
 * Author: Pranav
 */

#ifndef SX1280RADIO_COMADAPTER_LORARADIOADAPTER_HPP
#define SX1280RADIO_COMADAPTER_LORARADIOADAPTER_HPP

#include <Sx1280Radio/ComAdapter/LoRaRadioAdapterComponentAc.hpp>
#include <Sx1280Radio/Config/NodeConfig.hpp>
#include <Sx1280Radio/Node/LoRaNode.hpp>

#include <deque>

namespace Sx1280Radio {

class LoRaRadioAdapter final : public LoRaRadioAdapterComponentBase {
  public:
    explicit LoRaRadioAdapter(const char* compName);

    ~LoRaRadioAdapter() override;

  private:
    // Svc.Com handlers
    void dataIn_handler(const FwIndexType portNum,
                        Fw::Buffer& sendBuffer,
                        const ComCfg::FrameContext& context) override;

    void dataReturnIn_handler(const FwIndexType portNum,
                              Fw::Buffer& buffer,
                              const ComCfg::FrameContext& context) override;

    // Com status: initial READY and per-transmission status
    void comStatusInit();

    // Scheduler handler to poll the underlying LoRa node and forward
    // received frames into the Com stack.
    void run_handler(const FwIndexType portNum, const Svc::SchedContext& context) override;

  private:
    // Internal helpers
    bool ensureNodeStarted();

    // Queue of received frames waiting to be forwarded into F´
    std::deque<LoRaNodeRxPacket> m_rxQueue;

    // Underlying radio node and configuration
    NodeConfig m_nodeConfig{};
    LoRaNode* m_node{nullptr};

    bool m_started{false};
    bool m_initialStatusSent{false};
};

}  // namespace Sx1280Radio

#endif  // SX1280RADIO_COMADAPTER_LORARADIOADAPTER_HPP
