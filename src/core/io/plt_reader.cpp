#include "core/io/plt_reader.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace perception::core::io {

std::shared_ptr<model::IDataSet> PltReader::readCurves(
    const std::string& path, const ReadOptions& opts)
{
    // 安全约束：路径可访问性 + 大小上限，先校验再读入。
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        throw std::runtime_error("plt: cannot access file: " + path);
    }
    if (size > opts.maxBytes) {
        throw std::runtime_error("plt: file exceeds size limit (" +
                                 std::to_string(opts.maxBytes) + " bytes): " + path);
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("plt: cannot open file: " + path);
    }

    // TODO(M1): 解析 .plt 格式。
    //   - head 段：title、column labels（如 "vds (V)" / "id (A)"）
    //   - data 段：按空白切分为多列，同一曲线名分组为一条 Curve
    // 骨架阶段：仅返回空数据集，保证接口与构建链路可用。
    return std::make_shared<model::DataSet>(path);
}

void registerPltReader()
{
    ReaderRegistry::instance().registerReader(std::make_shared<PltReader>());
}

} // namespace perception::core::io
