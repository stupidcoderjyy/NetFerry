#ifndef NETFERRY_COMMON_LOGGER_H_
#define NETFERRY_COMMON_LOGGER_H_

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <sstream>
#include <string>

namespace net_ferry::common {

// Convert a native-encoding error string to UTF-8 for console output.
inline std::string ToUtf8(const std::string& native) {
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_ACP, 0, native.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return native;
    std::wstring wide(static_cast<std::size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_ACP, 0, native.c_str(), -1, wide.data(), wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return native;
    std::string utf8(static_cast<std::size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), ulen, nullptr, nullptr);
    // Remove null terminator from the counted length.
    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
    return utf8;
#else
    return native;
#endif
}

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
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
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