/*
 * LoRaRadioAdapter.cpp
 *
 * Created on: Mar 15, 2026
 * Author: Pranav
 */

#include <Sx1280Radio/ComAdapter/LoRaRadioAdapter.hpp>

#include <Fw/Buffer/Buffer.hpp>
#include <cstring>

namespace Sx1280Radio {

namespace {

// Hardcoded YAML path for now; deployments can override by editing this file
// in their vendored copy of the library.
constexpr const char* kDefaultYamlPath = "Sx1280Radio/Config/sx1280.yaml";

}

LoRaRadioAdapter::LoRaRadioAdapter(const char* compName)
    : LoRaRadioAdapterComponentBase(compName) {}

LoRaRadioAdapter::~LoRaRadioAdapter() = default;

bool LoRaRadioAdapter::ensureNodeStarted() {
    if (m_started) {
        return true;
    }

    std::string error;
    const auto err = loadNodeConfigFromYaml(kDefaultYamlPath, m_nodeConfig, &error);
    if (err != NodeConfigError::None) {
        // Failed to load configuration; adapter cannot operate.
        return false;
    }

    static LoRaNode node(m_nodeConfig);
    m_node = &node;

    // Install RX callback to enqueue received packets for later forwarding.
    m_node->setRxCallback([this](const LoRaNodeRxPacket& pkt) {
        this->m_rxQueue.push_back(pkt);
    });

    if (!m_node->start()) {
        return false;
    }

    m_started = true;
    return true;
}

void LoRaRadioAdapter::dataIn_handler(const FwIndexType portNum,
                                      Fw::Buffer& sendBuffer,
                                      const ComCfg::FrameContext& context) {
    if (!this->ensureNodeStarted()) {
        // Cannot transmit; return buffer and signal failure.
        this->dataReturnOut_out(portNum, sendBuffer, context);
        Fw::Success status = Fw::Success::FAILURE;
        this->comStatusOut_out(portNum, status);
        return;
    }

    const std::uint8_t* data = sendBuffer.getData();
    const FwSizeType size = sendBuffer.getSize();

    std::vector<std::uint8_t> payload;
    payload.assign(data, data + size);

    const bool ok = m_node->send(payload);

    // Return buffer ownership first, per Svc.Com protocol.
    this->dataReturnOut_out(portNum, sendBuffer, context);

    Fw::Success status = ok ? Fw::Success::SUCCESS : Fw::Success::FAILURE;
    this->comStatusOut_out(portNum, status);
}

void LoRaRadioAdapter::dataReturnIn_handler(const FwIndexType portNum,
                                            Fw::Buffer& buffer,
                                            const ComCfg::FrameContext& context) {
    (void)portNum;
    (void)context;

    // Upstream components are done with this buffer; return it to BufferManager.
    this->deallocate_out(0, buffer);
}

void LoRaRadioAdapter::comStatusInit() {
    if (m_initialStatusSent) {
        return;
    }

    if (!this->ensureNodeStarted()) {
        return;
    }

    Fw::Success status = Fw::Success::SUCCESS;
    this->comStatusOut_out(0, status);
    m_initialStatusSent = true;
}

void LoRaRadioAdapter::run_handler(const FwIndexType portNum, const Svc::SchedContext& context) {
    (void)portNum;
    (void)context;

    if (!this->ensureNodeStarted()) {
        return;
    }

    // Ensure we have signaled initial readiness once the node is up.
    this->comStatusInit();

    // Service radio IRQs and invoke RX callbacks.
    m_node->poll();

    // Forward any queued packets into the Com stack.
    while (!m_rxQueue.empty()) {
        const LoRaNodeRxPacket pkt = m_rxQueue.front();
        m_rxQueue.pop_front();

        const FwSizeType needed = static_cast<FwSizeType>(pkt.payload.size());

        Fw::Buffer buffer;
        this->allocate_out(0, needed, buffer);

        if (!buffer.isValid() || buffer.getSize() < needed) {
            // Drop packet if we cannot get a suitable buffer.
            if (buffer.isValid()) {
                this->deallocate_out(0, buffer);
            }
            continue;
        }

        std::uint8_t* data = buffer.getData();
        std::memcpy(data, pkt.payload.data(), needed);
        buffer.setSize(needed);

        ComCfg::FrameContext ctx;
        this->dataOut_out(0, buffer, ctx);
    }
}

}  // namespace Sx1280Radio
