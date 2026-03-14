#include "NodeConfig.hpp"

#include <fstream>
#include <sstream>

namespace Sx1280Radio {

namespace {

    // Very small, schema-specific parser for the current sx1280_dual.yaml.

    bool read_file_to_string(const std::string& path, std::string& out, std::string* error_out) {
        std::ifstream ifs(path);
        if (!ifs) {
            if (error_out) {
                *error_out = "failed to open config file: " + path;
            }
            return false;
        }
        std::ostringstream ss;
        ss << ifs.rdbuf();
        out = ss.str();
        return true;
    }

} // namespace

NodeConfigError loadNodeConfigFromYaml(
    const std::string& path,
    NodeConfig& out,
    std::string* error_out
) {
    std::string yaml;
    if (!read_file_to_string(path, yaml, error_out)) {
        return NodeConfigError::IoError;
    }

    // TODO: Implement minimal YAML parsing that understands the specific
    // schema used in Sx1280Radio/config/sx1280_dual.yaml and populates `out`.
    // For now, return ParseError to indicate this is not wired up yet.

    if (error_out) {
        *error_out = "loadNodeConfigFromYaml is not yet implemented";
    }
    (void) out; // suppress unused for now
    return NodeConfigError::ParseError;
}

} // namespace Sx1280Radio
