#pragma once

#include "core/model/dataset.h"
#include "core/model/structure.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace perception::core::io {

// 数据种类：曲线数据 or 结构数据。
enum class DataKind { Curve, Structure };

// 读取选项：安全限制的统一入口。
struct ReadOptions {
    std::size_t maxBytes = 64ull * 1024 * 1024;  // 默认 64 MiB
};

// 格式读取器接口：新增格式 = 新增实现并注册，不修改核心或消费者。
class IReader {
public:
    virtual ~IReader() = default;
    virtual std::string formatName() const = 0;
    virtual DataKind kind() const = 0;
    virtual std::vector<std::string> extensions() const = 0;
};

// 曲线读取器（.plt / .csv / .dat ...）。
class ICurveReader : public IReader {
public:
    // 读取曲线数据集；失败抛 std::runtime_error（含路径与原因）。
    virtual std::shared_ptr<model::IDataSet> readCurves(
        const std::string& path, const ReadOptions& opts = {}) = 0;
};

// 结构读取器（.tdr ...）。
class IStructureReader : public IReader {
public:
    virtual std::shared_ptr<model::IStructure> readStructure(
        const std::string& path, const ReadOptions& opts = {}) = 0;
};

// 读取器注册表：按文件扩展名分派到已注册读取器。
// 线程安全；应用启动时注册全部内置读取器。
class ReaderRegistry {
public:
    static ReaderRegistry& instance();

    void registerReader(std::shared_ptr<IReader> reader);
    std::shared_ptr<IReader> findByPath(const std::string& path) const;
    std::shared_ptr<ICurveReader> findCurveReader(const std::string& path) const;
    std::shared_ptr<IStructureReader> findStructureReader(const std::string& path) const;

    // 已注册读取器的全部扩展名（小写、含点；供权威目录一致性校验，FR-011）。
    std::vector<std::string> registeredExtensions() const;

private:
    ReaderRegistry() = default;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<IReader>> readers_;
};

} // namespace perception::core::io
