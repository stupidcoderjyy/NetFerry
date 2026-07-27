#ifndef NETFERRY_COMMON_CONFIG_H_
#define NETFERRY_COMMON_CONFIG_H_

#include <string>

namespace net_ferry::common {

class Config {
public:
    static Config LoadFromFile(const std::string& filepath);

    const std::string& role() const { return role_; }
    int listen_port() const { return listen_port_; }
    const std::string& inbox_dir() const { return inbox_dir_; }
    const std::string& outbox_dir() const { return outbox_dir_; }
    const std::string& target_server() const { return target_server_; }
    const std::string& file_ext_req() const { return file_ext_req_; }
    const std::string& file_ext_resp() const { return file_ext_resp_; }

private:
    std::string role_;
    int listen_port_ = 0;
    std::string inbox_dir_;
    std::string outbox_dir_;
    std::string target_server_;
    std::string file_ext_req_ = ".req";
    std::string file_ext_resp_ = ".resp";
};

}  // namespace net_ferry::common

#endif  // NETFERRY_COMMON_CONFIG_H_
