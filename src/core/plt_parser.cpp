#include "plt_parser.h"

#include <fstream>
#include <stdexcept>

namespace perception::core {

std::unique_ptr<IDataSet> PltParser::parse(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("cannot open .plt file: " + path);
    }

    // TODO(M1): 解析 .plt 格式。
    //   - head 段：title、column labels（如 "vds (V)" / "id (A)"）
    //   - data 段：按空白切分为多列，同一曲线名分组为一条 Curve
    // 骨架阶段：仅返回空数据集，保证接口与构建链路可用。
    return std::make_unique<DataSet>(path);
}

} // namespace perception::core
