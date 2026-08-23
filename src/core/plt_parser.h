#pragma once

#include "dataset.h"

#include <memory>
#include <string>

namespace perception::core {

// .plt 文件解析器。
// M1 实现完整格式解析（head 段标题/列定义 + data 段多曲线切分）。
// 骨架阶段为占位实现：仅校验文件可打开，返回空数据集。
class PltParser {
public:
    // 解析 .plt 文件，返回数据集。
    // 文件不存在或无法打开时抛出 std::runtime_error。
    static std::unique_ptr<IDataSet> parse(const std::string& path);
};

} // namespace perception::core
