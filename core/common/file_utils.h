#ifndef NETFERRY_COMMON_FILEUTILS_H_
#define NETFERRY_COMMON_FILEUTILS_H_

#include <google/protobuf/message.h>

#include <string>

namespace net_ferry::common {

bool WriteMessageToFile(const std::string& filepath, const google::protobuf::Message& message);

bool ReadMessageFromFile(const std::string& filepath, google::protobuf::Message& message);

bool RemoveFile(const std::string& filepath);

}  // namespace net_ferry::common

#endif  // NETFERRY_COMMON_FILEUTILS_H_
