#pragma once

#include "core/io/reader.h"

namespace perception::core::io {

// .csv 曲线读取器（M2+ 实现）。
// 存在意义：演示「新格式 = 新 reader + 注册」的扩展路径，核心零改动。
class CsvReader : public ICurveReader {
public:
    std::string formatName() const override { return "csv"; }
    DataKind kind() const override { return DataKind::Curve; }
    std::vector<std::string> extensions() const override { return {".csv"}; }

    std::shared_ptr<model::IDataSet> readCurves(
        const std::string& path, const ReadOptions& opts) override;
};

// 注册 .csv 读取器。
void registerCsvReader();

} // namespace perception::core::io
