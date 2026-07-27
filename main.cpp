#include <asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "core/common/config.h"
#include "core/common/logger.h"
#include "core/network/tcp_server.h"
#include "core/processor/inside_processor.h"
#include "core/processor/outside_processor.h"
#include "core/watcher/directory_watcher.h"

namespace {

void PrintUsage() {
    std::cerr << "Usage: NetFerry --config=<path>\n";
}

std::string ParseConfigPath(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string const arg(argv[i]);
        if (arg.starts_with("--config=")) {
            return arg.substr(9);
        }
    }
    return "";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string const config_path = ParseConfigPath(argc, argv);
    if (config_path.empty()) {
        PrintUsage();
        return 1;
    }

    net_ferry::common::Config config;
    try {
        config = net_ferry::common::Config::LoadFromFile(config_path);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to load config: " << e.what();
        return 1;
    }

    net_ferry::common::InitLogger();
    LOG_INFO << "NetFerry starting as '" << config.role() << "' role";

    asio::io_context io_context;
    auto work_guard = asio::make_work_guard(io_context);

    if (config.role() == "inside") {
        auto server = std::make_shared<net_ferry::network::TcpServer>(
                io_context, config.listen_port(), config.outbox_dir(), config.file_ext_req());
        server->Start();

        auto processor = std::make_shared<net_ferry::processor::InsideProcessor>(
                io_context, server, config.inbox_dir());

        auto watcher = std::make_shared<net_ferry::watcher::DirectoryWatcher>(io_context,
                config.inbox_dir(), config.file_ext_resp(),
                [processor](const std::string& path) { processor->OnFileCreated(path); });
        watcher->Start();

        LOG_INFO << "Inside gateway initialized. Press Ctrl+C to stop.";
        io_context.run();

    } else if (config.role() == "outside") {
        std::string host;
        std::string port = "80";
        std::string target = config.target_server();

        if (size_t const proto_end = target.find("://"); proto_end != std::string::npos) {
            target = target.substr(proto_end + 3);
        }

        size_t const colon = target.find(':');
        size_t const slash = target.find('/');
        if (colon != std::string::npos) {
            host = target.substr(0, colon);
            std::string port_part = target.substr(colon + 1);
            if (slash != std::string::npos) {
                port_part = port_part.substr(0, slash - colon - 1);
            }
            port = port_part;
        } else {
            host = (slash != std::string::npos) ? target.substr(0, slash) : target;
        }

        LOG_INFO << "Target server: " << host << ":" << port;

        auto processor = std::make_shared<net_ferry::processor::OutsideProcessor>(io_context,
                config.inbox_dir(), config.outbox_dir(), host, port, config.file_ext_resp());

        auto watcher = std::make_shared<net_ferry::watcher::DirectoryWatcher>(io_context,
                config.inbox_dir(), config.file_ext_req(),
                [processor](const std::string& path) { processor->OnFileCreated(path); });
        watcher->Start();

        LOG_INFO << "Outside gateway initialized. Press Ctrl+C to stop.";
        io_context.run();

    } else {
        LOG_ERROR << "Unknown role: " << config.role() << " (expected 'inside' or 'outside')";
        return 1;
    }

    return 0;
}