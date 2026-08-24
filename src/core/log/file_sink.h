#pragma once

#include "core/log/log_sink.h"

#include <cstdint>
#include <memory>
#include <string>

namespace perception::core::log {

// 文件输出目标（US2 扩展轮转）。默认追加写，日志目录不存在时自动创建（FR-005）。
class FileSink final : public LogSink {
public:
    explicit FileSink(std::string path,
                      std::uint64_t maxFileSize = 5 * 1024 * 1024,
                      int maxBackups = 3);
    ~FileSink() override;

    void emit(const LogRecord& record) override;
    const char* name() const noexcept override { return "FileSink"; }

    const std::string& path() const noexcept { return path_; }
    bool isWritable() const noexcept { return writable_; }  // 降级探测（FR-005）

    // ---- 运行期日志路径管理（设置菜单：日志路径迁移 / 清除历史日志；FR-016/017）----
    // 关闭旧文件句柄，把旧路径的 app.log 及全部归档迁移到 newPath 所在目录后按新路径重开；
    // 新目录不存在时自动创建（FR-005）。返回迁移后是否可写。
    bool migrateTo(const std::string& newPath);
    // 关闭文件句柄，删除 app.log 及全部归档，重建空文件继续写入。返回是否可写。
    bool clearHistory();

    // ---- US2：大小轮转（plan.md §5.2：5MB×3，锁内滚动链）----
    std::uint64_t maxFileSize() const noexcept { return maxFileSize_; }
    int maxBackups() const noexcept { return maxBackups_; }
    void setMaxFileSize(std::uint64_t bytes);
    void setMaxBackups(int count);

private:
    void rotateLocked();  // 前置条件：已持有 impl_->mutex
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string path_;
    std::uint64_t maxFileSize_;
    int maxBackups_;
    bool writable_ = false;
};

} // namespace perception::core::log
