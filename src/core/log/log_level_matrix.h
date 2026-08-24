#pragma once

#include "core/log/log_level.h"

#include <bitset>

namespace perception::core::log {

// 某输出目标的级别开关集合（FR-002）：每级别一个独立布尔位。
// 控制台与文件各持一个独立实例，可分别配置。
class LogLevelMatrix {
public:
    LogLevelMatrix();   // 默认：DEBUG 关，INFO/WARN/ERROR/FATAL 开

    void setEnabled(LogLevel level, bool enabled);
    bool isEnabled(LogLevel level) const noexcept;

    // 一次打开/关闭全部级别（测试与 UI 重置用）
    void setAll(bool enabled);

private:
    std::bitset<5> bits_;   // 索引 = static_cast<int>(LogLevel)
};

} // namespace perception::core::log
