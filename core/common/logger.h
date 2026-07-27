#ifndef NETFERRY_COMMON_LOGGER_H_
#define NETFERRY_COMMON_LOGGER_H_

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace net_ferry::common {

class Logger {
public:
    enum class Level { kInfo, kWarn, kError };

    Logger(Level level, const char* file, int line) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
                  1000;
        stream_ << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        stream_ << '.' << std::setfill('0') << std::setw(3) << ms.count() << ' ';
        switch (level) {
            case Level::kInfo:
                stream_ << "[INFO] ";
                break;
            case Level::kWarn:
                stream_ << "[WARN] ";
                break;
            case Level::kError:
                stream_ << "[ERROR] ";
                break;
        }
        stream_ << file << ':' << line << " | ";
    }

    ~Logger() {
        stream_ << '\n';
        std::cout << stream_.str() << std::flush;
    }

    template <typename T>
    Logger& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    std::ostringstream stream_;
};

}  // namespace net_ferry::common

#define LOG_INFO \
    ::net_ferry::common::Logger(::net_ferry::common::Logger::Level::kInfo, __FILE__, __LINE__)
#define LOG_WARN \
    ::net_ferry::common::Logger(::net_ferry::common::Logger::Level::kWarn, __FILE__, __LINE__)
#define LOG_ERROR \
    ::net_ferry::common::Logger(::net_ferry::common::Logger::Level::kError, __FILE__, __LINE__)

#endif  // NETFERRY_COMMON_LOGGER_H_
