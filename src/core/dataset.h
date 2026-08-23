#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace perception::core {

// 一条曲线：x/y 数据 + 可选名称。
// 对应 .plt 文件中的一个数据块（如 drain current vs gate voltage）。
struct Curve {
    std::string name;
    std::vector<double> x;
    std::vector<double> y;

    bool empty() const noexcept { return x.empty() || x.size() != y.size(); }
    std::size_t size() const noexcept
    {
        return x.size() < y.size() ? x.size() : y.size();
    }
};

// 数据集抽象：一个 .plt 文件解析出的全部曲线集合。
class IDataSet {
public:
    virtual ~IDataSet() = default;

    virtual const std::string& name() const noexcept = 0;
    virtual const std::vector<Curve>& curves() const noexcept = 0;
    virtual void addCurve(Curve curve) = 0;
};

// 默认实现（M1 细化：列名/单位/元数据）。
class DataSet : public IDataSet {
public:
    explicit DataSet(std::string name) : name_(std::move(name)) {}

    const std::string& name() const noexcept override { return name_; }
    const std::vector<Curve>& curves() const noexcept override { return curves_; }
    void addCurve(Curve curve) override { curves_.push_back(std::move(curve)); }

private:
    std::string name_;
    std::vector<Curve> curves_;
};

} // namespace perception::core
