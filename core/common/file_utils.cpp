#include "core/common/file_utils.h"

#include <cstdio>
#include <fstream>

namespace net_ferry::common {

bool WriteMessageToFile(const std::string& filepath, const google::protobuf::Message& message) {
    std::ofstream ofs(filepath, std::ios::binary);
    if (!ofs.is_open()) {
        return false;
    }
    if (!message.SerializeToOstream(&ofs)) {
        return false;
    }
    ofs.close();
    return ofs.good();
}

bool ReadMessageFromFile(const std::string& filepath, google::protobuf::Message& message) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }
    if (!message.ParseFromIstream(&ifs)) {
        return false;
    }
    return true;
}

bool RemoveFile(const std::string& filepath) {
    if (std::remove(filepath.c_str()) != 0) {
        return (errno == ENOENT);
    }
    return true;
}

}  // namespace net_ferry::common
