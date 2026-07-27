#ifndef NETFERRY_WATCHER_DIRECTORYWATCHER_H_
#define NETFERRY_WATCHER_DIRECTORYWATCHER_H_

#include <asio.hpp>
#include <efsw/efsw.hpp>
#include <functional>
#include <string>

namespace net_ferry::watcher {

class DirectoryWatcher : public efsw::FileWatchListener {
public:
    using FileCallback = std::function<void(const std::string&)>;

    DirectoryWatcher(asio::io_context& io_context,
            std::string watch_dir,
            std::string file_extension,
            FileCallback callback);

    ~DirectoryWatcher() override;

    void Start();
    void Stop();

    void handleFileAction(efsw::WatchID watchid,
            const std::string& dir,
            const std::string& filename,
            efsw::Action action,
            const std::string& oldFilename) override;

private:
    asio::io_context& io_context_;
    std::string watch_dir_;
    std::string file_extension_;
    FileCallback callback_;
    efsw::FileWatcher watcher_;
    efsw::WatchID watch_id_ = 0;
};

}  // namespace net_ferry::watcher

#endif  // NETFERRY_WATCHER_DIRECTORYWATCHER_H_
