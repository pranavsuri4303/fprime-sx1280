// File: build-smoke/lora_node_smoke.cpp

#include "LoRaNode.hpp"
#include "NodeConfig.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace Sx1280Radio;

namespace {

    volatile std::sig_atomic_t g_stop = 0;

    void handle_sigint(int) {
        g_stop = 1;
    }

} // namespace

int main() {
    try {
        std::signal(SIGINT, handle_sigint);

        NodeConfig cfg{};
        std::string error;

        const std::string yaml_path = "Sx1280Radio/Config/sx1280_dual.yaml";
        const auto err = loadNodeConfigFromYaml(yaml_path, cfg, &error);
        if (err != NodeConfigError::None) {
            std::cerr << "Failed to load node config from '" << yaml_path
                      << "': " << error << std::endl;
            return 1;
        }

        LoRaNode node(cfg);

        node.setRxCallback([](const LoRaNodeRxPacket& pkt) {
            const std::size_t msg_len = 11; // "Hello LoRa!"
            const std::size_t n = std::min<std::size_t>(msg_len, pkt.payload.size());
            std::string as_text(pkt.payload.begin(), pkt.payload.begin() + n);
            std::cout << "[rx] payload_len=" << pkt.payload.size()
                      << " text=\"" << as_text << "\""
                      << " rssi=" << static_cast<int>(pkt.rssi_dbm)
                      << " snr=" << static_cast<int>(pkt.snr_db)
                      << std::endl;
        });

        std::cout << "Starting LoRaNode..." << std::endl;
        if (!node.start()) {
            std::cerr << "LoRaNode::start() failed" << std::endl;
            return 2;
        }

        const std::vector<std::uint8_t> payload{
            'H','e','l','l','o',' ','L','o','R','a','!'
        };

        auto next_tx = std::chrono::steady_clock::now();

        std::cout << "Running. Press Ctrl-C to stop." << std::endl;

        while (!g_stop) {
            node.poll();

            const auto now = std::chrono::steady_clock::now();
            if (now >= next_tx) {
                std::cout << "[tx] sending test payload" << std::endl;
                if (!node.send(payload)) {
                    std::cerr << "LoRaNode::send() failed" << std::endl;
                }
                next_tx = now + std::chrono::seconds(2);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "Stopping LoRaNode..." << std::endl;
        node.stop();

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 10;
    }
}
