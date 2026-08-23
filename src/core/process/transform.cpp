#include "core/process/transform.h"

namespace perception::core::process {

void UnitScale::apply(model::DataSet& /*data*/) const
{
    // TODO(M2): 对 data 中每条曲线的 y 值乘以 factor_。
    // 骨架阶段 no-op（factor_ == 1.0 时本就无操作）。
}

} // namespace perception::core::process
