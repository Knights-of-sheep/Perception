#include "core/log/log_level_matrix.h"

namespace perception::core::log {

LogLevelMatrix::LogLevelMatrix()
    : bits_()
{
    // 默认：DEBUG 关，INFO/WARN/ERROR/FATAL 开
    setAll(false);
    setEnabled(LogLevel::Info, true);
    setEnabled(LogLevel::Warn, true);
    setEnabled(LogLevel::Error, true);
    setEnabled(LogLevel::Fatal, true);
}

void LogLevelMatrix::setEnabled(LogLevel level, bool enabled)
{
    const int idx = static_cast<int>(level);
    if (idx >= 0 && idx < 5)
        bits_.set(idx, enabled);
}

bool LogLevelMatrix::isEnabled(LogLevel level) const noexcept
{
    const int idx = static_cast<int>(level);
    if (idx < 0 || idx >= 5)
        return false;
    return bits_.test(idx);
}

void LogLevelMatrix::setAll(bool enabled)
{
    if (enabled)
        bits_.set();
    else
        bits_.reset();
}

} // namespace perception::core::log
