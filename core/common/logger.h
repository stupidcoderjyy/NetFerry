#ifndef NETFERRY_COMMON_LOGGER_H_
#define NETFERRY_COMMON_LOGGER_H_

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <sstream>

namespace net_ferry::common {

// RAII helper that collects stream-style input and flushes to spdlog.
class SpdlogStream {
public:
    explicit SpdlogStream(spdlog::level::level_enum lvl) : lvl_(lvl) {}

    ~SpdlogStream() { spdlog::log(lvl_, "{}", stream_.str()); }

    template <typename T>
    SpdlogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    spdlog::level::level_enum lvl_;
    std::ostringstream stream_;
};

inline void InitLogger() {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("net_ferry", std::move(sink));
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(std::move(logger));
    spdlog::set_level(spdlog::level::info);
}

}  // namespace net_ferry::common

#define LOG_INFO ::net_ferry::common::SpdlogStream(spdlog::level::info)
#define LOG_WARN ::net_ferry::common::SpdlogStream(spdlog::level::warn)
#define LOG_ERROR ::net_ferry::common::SpdlogStream(spdlog::level::err)

#endif  // NETFERRY_COMMON_LOGGER_H_