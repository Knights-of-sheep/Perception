#include "core/log/file_sink.h"
#include "core/log/log_format_internal.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace perception::core::log {

struct FileSink::Impl {
    std::mutex mutex;
    std::ofstream stream;
};

FileSink::FileSink(std::string path, std::uint64_t maxFileSize, int maxBackups)
    : impl_(std::make_unique<Impl>())
    , path_(std::move(path))
    , maxFileSize_(maxFileSize)
    , maxBackups_(maxBackups)
{
    // 自动创建父目录（FR-005）
    std::error_code ec;
    const std::filesystem::path parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, ec);

    impl_->stream.open(path_, std::ios::out | std::ios::app);
    writable_ = impl_->stream.is_open();
}

FileSink::~FileSink()
{
    if (impl_->stream.is_open())
        impl_->stream.close();
}

void FileSink::emit(const LogRecord& record)
{
    if (!writable_)
        return;  // 降级：静默丢弃，不抛异常（FR-005）

    std::lock_guard<std::mutex> lock(impl_->mutex);

    // US2 大小轮转（写前检查）：当前文件已超限时先归档，再写新记录
    // （保证主文件始终承载最新日志，滚动链 app.log -> .1 -> .2 -> ...，删除最旧）
    std::error_code ec;
    const std::uint64_t curSize = std::filesystem::file_size(path_, ec);
    if (!ec && curSize >= maxFileSize_)
        rotateLocked();

    // 格式：YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message\n（共享 detail::formatLine）
    impl_->stream << detail::formatLine(record) << '\n';
    impl_->stream.flush();
}

// 前置条件：已持有 impl_->mutex
void FileSink::rotateLocked()
{
    std::error_code ec;
    impl_->stream.close();
    // 归档命名：app.log -> app.1.log -> app.2.log -> ...（序号插在扩展名之前）
    const std::filesystem::path main = std::filesystem::path(path_);
    const std::filesystem::path dir = main.parent_path();
    const std::string stem = main.stem().string();
    const std::string ext = main.extension().string();

    if (maxBackups_ > 0) {
        // 删除最旧归档（.N），然后逐级后移
        std::filesystem::remove(dir / (stem + "." + std::to_string(maxBackups_) + ext), ec);
        for (int i = maxBackups_ - 1; i >= 1; --i) {
            const auto from = dir / (stem + "." + std::to_string(i) + ext);
            const auto to = dir / (stem + "." + std::to_string(i + 1) + ext);
            if (std::filesystem::exists(from, ec))
                std::filesystem::rename(from, to, ec);
        }
        std::filesystem::rename(main, dir / (stem + ".1" + ext), ec);
    } else {
        // maxBackups<=0：直接丢弃当前内容
        std::filesystem::remove(main, ec);
    }
    impl_->stream.open(path_, std::ios::out | std::ios::trunc);
    writable_ = impl_->stream.is_open();
}

bool FileSink::migrateTo(const std::string& newPath)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (newPath == path_)
        return writable_;

    // 关闭旧文件句柄（Windows 上文件被占用无法移动/删除）
    if (impl_->stream.is_open())
        impl_->stream.close();

    std::error_code ec;
    const std::filesystem::path newMain(newPath);
    const std::filesystem::path newDir = newMain.parent_path();
    if (!newDir.empty())
        std::filesystem::create_directories(newDir, ec);
    ec.clear();

    // 迁移主文件与归档链：app.log -> app.1.log -> ...（序号插在扩展名之前）
    const std::filesystem::path oldMain(path_);
    const std::filesystem::path oldDir = oldMain.parent_path();
    const std::string oldStem = oldMain.stem().string();
    const std::string oldExt = oldMain.extension().string();
    const std::string newStem = newMain.stem().string();
    const std::string newExt = newMain.extension().string();

    for (int i = 0; i <= maxBackups_; ++i) {
        const auto oldFile = (i == 0)
            ? oldMain : oldDir / (oldStem + "." + std::to_string(i) + oldExt);
        ec.clear();
        if (!std::filesystem::exists(oldFile, ec))
            continue;
        const auto newFile = (i == 0)
            ? newMain : newDir / (newStem + "." + std::to_string(i) + newExt);
        std::filesystem::rename(oldFile, newFile, ec);  // 同盘快速移动
        if (ec) {  // 跨盘等场景 rename 失败：退化为复制 + 删除
            ec.clear();
            std::filesystem::copy_file(oldFile, newFile,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec)
                std::filesystem::remove(oldFile, ec);
        }
    }

    path_ = newPath;
    impl_->stream.open(path_, std::ios::out | std::ios::app);
    writable_ = impl_->stream.is_open();
    return writable_;
}

bool FileSink::clearHistory()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->stream.is_open())
        impl_->stream.close();

    // 删除主文件与全部归档
    std::error_code ec;
    const std::filesystem::path main(path_);
    const std::filesystem::path dir = main.parent_path();
    const std::string stem = main.stem().string();
    const std::string ext = main.extension().string();
    for (int i = 0; i <= maxBackups_; ++i) {
        const auto f = (i == 0) ? main : dir / (stem + "." + std::to_string(i) + ext);
        ec.clear();
        std::filesystem::remove(f, ec);
    }

    impl_->stream.open(path_, std::ios::out | std::ios::trunc);  // 重建空文件
    writable_ = impl_->stream.is_open();
    return writable_;
}

void FileSink::setMaxFileSize(std::uint64_t bytes)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    maxFileSize_ = bytes;
}

void FileSink::setMaxBackups(int count)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    maxBackups_ = count;
}

} // namespace perception::core::log
