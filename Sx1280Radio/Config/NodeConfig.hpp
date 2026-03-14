#pragma once

#include <string>

#include "Sx1280Config.hpp"
#include "LinuxSx1280Radio.hpp"

namespace Sx1280Radio {

    enum class NodeConfigError {
        None = 0,
        IoError,
        ParseError,
        InvalidSchema,
    };

    enum class NodeRadioRole {
        Rx,
        Tx,
        TxRx,
    };

    struct RadioHwConfig {
        LinuxSx1280RadioConfig hw;
        NodeRadioRole role{NodeRadioRole::TxRx};
    };

    struct NodeConfig {
        RadioHwConfig rx;
        RadioHwConfig tx;
        Sx1280Config radio;
    };

    // Load a NodeConfig from a YAML file. This expects the schema used in
    // Sx1280Radio/config/sx1280_dual.yaml and fills out the provided NodeConfig.
    // Returns NodeConfigError::None on success; on failure, returns an error and
    // optionally stores a human-readable message in `error_out`.
    NodeConfigError loadNodeConfigFromYaml(
        const std::string& path,
        NodeConfig& out,
        std::string* error_out = nullptr
    );

} // namespace Sx1280Radio
