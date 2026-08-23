#pragma once

#include "core/model/dataset.h"

#include <string>

namespace perception::core::process {

// 数据变换（管道）：对数据集做就地变换。
// 新增变换 = 实现 ITransform（或派生 + 注册）；骨架阶段仅定义契约。
class ITransform {
public:
    virtual ~ITransform() = default;
    virtual std::string name() const = 0;
    virtual void apply(model::DataSet& data) const = 0;
};

// 示例占位：单位换算（M2+ 细化）。factor_ 为 y 轴缩放系数。
class UnitScale : public ITransform {
public:
    explicit UnitScale(double factor = 1.0) : factor_(factor) {}

    std::string name() const override { return "unit_scale"; }
    void apply(model::DataSet& data) const override;

private:
    double factor_;
};

} // namespace perception::core::process
