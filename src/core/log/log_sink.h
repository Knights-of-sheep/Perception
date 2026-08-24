#pragma once

// Qt 的 qobjectdefs.h 将 emit 定义为空宏，会破坏本接口的 emit 虚函数声明。
// 无论包含顺序如何（Qt 头在前或 log 头在前），此处统一解除该宏，
// 防止任何混合 Qt + core/log 的翻译单元编译失败。
// 影响：包含本头文件后 Qt 的 emit 关键字不可用，请用 Q_EMIT。
#if defined(emit)
#undef emit
#endif

#include "core/log/log_level.h"
#include "core/log/log_level_matrix.h"
#include "core/log/log_record.h"

#include <memory>

namespace perception::core::log {

// 日志输出目标抽象。统一 API 向所有已注册 sink 广播记录（按各 sink 矩阵过滤）。
class LogSink {
public:
    virtual ~LogSink() = default;

    // ---- 级别矩阵（FR-002）：每 sink 独立；控制台与文件可分别配置 ----
    void setLevelEnabled(LogLevel level, bool enabled) { matrix_.setEnabled(level, enabled); }
    bool isLevelEnabled(LogLevel level) const noexcept { return matrix_.isEnabled(level); }
    const LogLevelMatrix& levelMatrix() const noexcept { return matrix_; }
    void setLevelMatrix(const LogLevelMatrix& m) { matrix_ = m; }

    virtual void emit(const LogRecord& record) = 0;   // 不得抛异常
    virtual const char* name() const noexcept = 0;

protected:
    LogLevelMatrix matrix_;   // 默认 DEBUG 关、其余开
};

using LogSinkPtr = std::shared_ptr<LogSink>;

} // namespace perception::core::log
