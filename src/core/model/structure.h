#pragma once

#include <string>
#include <utility>
#include <vector>

namespace perception::core::model {

// 结构数据（.tdr）的场：名称 + 顶点/单元上的数值数组。
struct FieldData {
    std::string name;
    std::vector<double> values;
};

// 结构数据抽象：网格 + 场集合（对标 ParaView 网格/结构数据查看）。
// M2+ 细化网格（节点坐标 / 单元拓扑 / 连接表），骨架阶段仅定义场模型。
class IStructure {
public:
    virtual ~IStructure() = default;

    virtual const std::string& name() const noexcept = 0;
    virtual const std::vector<FieldData>& fields() const noexcept = 0;
    virtual void addField(FieldData field) = 0;
};

// 默认实现。
class Structure : public IStructure {
public:
    explicit Structure(std::string name) : name_(std::move(name)) {}

    const std::string& name() const noexcept override { return name_; }
    const std::vector<FieldData>& fields() const noexcept override { return fields_; }
    void addField(FieldData field) override { fields_.push_back(std::move(field)); }

private:
    std::string name_;
    std::vector<FieldData> fields_;
};

} // namespace perception::core::model
