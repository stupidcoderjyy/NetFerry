#ifndef NETFERRY_PROCESSOR_OUTSIDEPROCESSOR_H_
#define NETFERRY_PROCESSOR_OUTSIDEPROCESSOR_H_

#include <asio.hpp>
#include <string>

namespace net_ferry::processor {

class OutsideProcessor {
public:
    OutsideProcessor(asio::io_context& io_context,
            std::string inbox_dir,
            std::string outbox_dir,
            std::string target_host,
            std::string target_port,
            std::string file_ext_resp);

    void OnFileCreated(const std::string& filepath);

private:
    void ProcessReqFile(const std::string& filepath);

    asio::io_context& io_context_;
    std::string inbox_dir_;
    std::string outbox_dir_;
    std::string target_host_;
    std::string target_port_;
    std::string file_ext_resp_;
};

}  // namespace net_ferry::processor

#endif  // NETFERRY_PROCESSOR_OUTSIDEPROCESSOR_H_
