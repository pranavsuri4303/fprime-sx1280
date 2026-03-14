/*
 * LoRaNode.cpp
 *
 * Created on: Mar 14, 2026
 * Author: Pranav
 */

#include "LoRaNode.hpp"

#include <utility>

namespace Sx1280Radio {

    struct LoRaNode::Impl {
        NodeConfig config;
        RxCallback rx_callback{};

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
        // TODO: Construct LinuxSx1280Radio instances using m_impl->config and
        // wire them up for RX/TX. For now this is a stub.
        return true;
    }

    void LoRaNode::stop() {
        // TODO: Stop and tear down any underlying radio objects.
    }

    bool LoRaNode::send(const std::vector<std::uint8_t>& payload) {
        // TODO: Forward to TX radio once implemented.
        (void) payload;
        return false;
    }

    void LoRaNode::poll() {
        // TODO: Pump IRQ handling on underlying radios once implemented.
    }

    void LoRaNode::setRxCallback(RxCallback cb) {
        m_impl->rx_callback = std::move(cb);
    }

} // namespace Sx1280Radio
