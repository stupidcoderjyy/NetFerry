#include "core/watcher/directory_watcher.h"

#include <utility>

#include "core/common/logger.h"

namespace net_ferry::watcher {

DirectoryWatcher::DirectoryWatcher(asio::io_context& io_context,
        std::string watch_dir,
        std::string file_extension,
        FileCallback callback)
    : io_context_(io_context),
      watch_dir_(std::move(watch_dir)),
      file_extension_(std::move(file_extension)),
      callback_(std::move(callback)) {}

DirectoryWatcher::~DirectoryWatcher() {
    Stop();
}

void DirectoryWatcher::Start() {
    watch_id_ = watcher_.addWatch(watch_dir_, this, true);
    if (watch_id_ < 0) {
        LOG_ERROR << "Failed to add watch for directory: " << watch_dir_;
        return;
    }
    watcher_.watch();
    LOG_INFO << "DirectoryWatcher started on " << watch_dir_ << " (ext=" << file_extension_ << ")";
}

void DirectoryWatcher::Stop() {
    if (watch_id_ > 0) {
        watcher_.removeWatch(watch_id_);
        watch_id_ = 0;
    }
}

void DirectoryWatcher::handleFileAction(efsw::WatchID,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        const std::string&) {
    if (action != efsw::Actions::Add) {
        return;
    }

    if (!file_extension_.empty()) {
        if (filename.size() < file_extension_.size() || !filename.ends_with(file_extension_)) {
            return;
        }
    }

    std::string const full_path = dir + filename;
    asio::post(io_context_, [this, full_path]() { callback_(full_path); });
}

}  // namespace net_ferry::watcher
