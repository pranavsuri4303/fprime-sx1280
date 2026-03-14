#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "NodeConfig.hpp"

namespace Sx1280Radio {

    struct LoRaNodeRxPacket {
        std::vector<std::uint8_t> payload;
        std::int8_t rssi_dbm{0};
        std::int8_t snr_db{0};
    };

    class LoRaNode {
    public:
        using RxCallback = std::function<void(const LoRaNodeRxPacket&)>;

        explicit LoRaNode(const NodeConfig& config);
        ~LoRaNode();

        LoRaNode(const LoRaNode&) = delete;
        LoRaNode& operator=(const LoRaNode&) = delete;
        LoRaNode(LoRaNode&&) = delete;
        LoRaNode& operator=(LoRaNode&&) = delete;

        bool start();
        void stop();

        // Send a payload via the TX radio. Returns true on success.
        bool send(const std::vector<std::uint8_t>& payload);

        // Polls underlying devices for IRQs and dispatches callbacks.
        void poll();

        void setRxCallback(RxCallback cb);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace Sx1280Radio
