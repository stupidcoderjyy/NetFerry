#include "core/common/config.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace net_ferry::common {

Config Config::LoadFromFile(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filepath);
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse config JSON: " + std::string(e.what()));
    }

    Config config;

    if (j.contains("role")) {
        config.role_ = j["role"].get<std::string>();
    } else {
        throw std::runtime_error("Config missing required field: role");
    }

    if (j.contains("listen_port")) {
        config.listen_port_ = j["listen_port"].get<int>();
    }

    if (j.contains("inbox_dir")) {
        config.inbox_dir_ = j["inbox_dir"].get<std::string>();
    } else {
        throw std::runtime_error("Config missing required field: inbox_dir");
    }

    if (j.contains("outbox_dir")) {
        config.outbox_dir_ = j["outbox_dir"].get<std::string>();
    } else {
        throw std::runtime_error("Config missing required field: outbox_dir");
    }

    if (j.contains("target_server")) {
        config.target_server_ = j["target_server"].get<std::string>();
    }

    if (j.contains("file_ext_req")) {
        config.file_ext_req_ = j["file_ext_req"].get<std::string>();
    }

    if (j.contains("file_ext_resp")) {
        config.file_ext_resp_ = j["file_ext_resp"].get<std::string>();
    }

    return config;
}

}  // namespace net_ferry::common
