#include "core/processor/outside_processor.h"

#include <sstream>
#include <utility>

#include "core/common/file_utils.h"
#include "core/common/logger.h"
#include "core/network/http_client.h"
#include "packet.pb.h"

namespace net_ferry::processor {

OutsideProcessor::OutsideProcessor(asio::io_context& io_context,
        std::string inbox_dir,
        std::string outbox_dir,
        std::string target_host,
        std::string target_port,
        std::string file_ext_resp)
    : io_context_(io_context),
      inbox_dir_(std::move(inbox_dir)),
      outbox_dir_(std::move(outbox_dir)),
      target_host_(std::move(target_host)),
      target_port_(std::move(target_port)),
      file_ext_resp_(std::move(file_ext_resp)) {}

void OutsideProcessor::OnFileCreated(const std::string& filepath) {
    asio::post(io_context_, [this, filepath]() { ProcessReqFile(filepath); });
}

void OutsideProcessor::ProcessReqFile(const std::string& filepath) {
    gateway::RequestPacket request;
    if (!common::ReadMessageFromFile(filepath, request)) {
        LOG_ERROR << "Failed to read request file: " << filepath;
        common::RemoveFile(filepath);
        return;
    }

    LOG_INFO << filepath << " → " << request.method() << " " << request.url() << " ["
             << request.body().size() << "B]";

    std::ostringstream headers_oss;
    for (const auto& [key, value] : request.headers()) {
        headers_oss << key << ": " << value << "\r\n";
    }

    auto http_resp = network::SendHttpRequest(io_context_, target_host_, target_port_,
            request.method(), request.url(), headers_oss.str(), request.body());

    gateway::ResponsePacket response;
    response.set_request_id(request.request_id());
    response.set_status_code(http_resp.status_code);
    response.set_body(http_resp.body);

    std::istringstream header_stream(http_resp.headers);
    std::string header_line;
    while (std::getline(header_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        if (header_line.empty()) {
            continue;
        }
        auto colon = header_line.find(':');
        if (colon != std::string::npos) {
            std::string key = header_line.substr(0, colon);
            std::string value = header_line.substr(colon + 1);
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);
            }
            (*response.mutable_headers())[key] = value;
        }
    }

    std::string const out_file = outbox_dir_ + "/" + request.request_id() + file_ext_resp_;
    if (!common::WriteMessageToFile(out_file, response)) {
        LOG_ERROR << "Failed to write response file: " << out_file;
    } else {
        LOG_INFO << out_file << " ← " << http_resp.status_code << " [" << http_resp.body.size()
                 << "B]";
    }

    common::RemoveFile(filepath);
}

}  // namespace net_ferry::processor