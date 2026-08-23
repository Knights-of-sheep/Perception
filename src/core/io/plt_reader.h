#pragma once

#include "core/io/reader.h"

namespace perception::core::io {

// .plt 曲线读取器。
// M1 实现完整格式解析（head 段标题 / 列定义 + data 段多曲线切分）；
// 骨架阶段为占位：校验文件存在与大小上限，返回空数据集。
class PltReader : public ICurveReader {
public:
    std::string formatName() const override { return "plt"; }
    DataKind kind() const override { return DataKind::Curve; }
    std::vector<std::string> extensions() const override { return {".plt"}; }

    std::shared_ptr<model::IDataSet> readCurves(
        const std::string& path, const ReadOptions& opts) override;
};

// 注册 .plt 读取器（应用初始化或 Python 命令层调用）。
void registerPltReader();

} // namespace perception::core::io
