#include "core/processor/inside_processor.h"

#include <sstream>
#include <utility>

#include "core/common/file_utils.h"
#include "core/common/logger.h"
#include "core/network/session.h"
#include "packet.pb.h"

namespace net_ferry::processor {

InsideProcessor::InsideProcessor(asio::io_context& io_context,
        std::shared_ptr<network::TcpServer> server,
        std::string inbox_dir)
    : io_context_(io_context), server_(std::move(server)), inbox_dir_(std::move(inbox_dir)) {}

void InsideProcessor::OnFileCreated(const std::string& filepath) {
    asio::post(io_context_, [this, filepath]() { ProcessRespFile(filepath); });
}

void InsideProcessor::ProcessRespFile(const std::string& filepath) {
    gateway::ResponsePacket response;
    if (!common::ReadMessageFromFile(filepath, response)) {
        LOG_ERROR << "Failed to read response file: " << filepath;
        common::RemoveFile(filepath);
        return;
    }

    const std::string& request_id = response.request_id();
    auto session = server_->FindSession(request_id);
    if (!session) {
        LOG_ERROR << "No session found for request_id: " << request_id;
        common::RemoveFile(filepath);
        return;
    }

    std::ostringstream headers_oss;
    for (const auto& [key, value] : response.headers()) {
        headers_oss << key << ": " << value << "\r\n";
    }

    session->SendResponse(response.status_code(), headers_oss.str(), response.body());

    server_->RemoveSession(request_id);
    common::RemoveFile(filepath);

    LOG_INFO << "InsideProcessor sent response for " << request_id;
}

}  // namespace net_ferry::processor
