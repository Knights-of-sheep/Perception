// ===== 命令层骨架实现（006 US4）=====
// 占位命令一律抛 NotImplementedError（模块内经 register_exception 注册为
// Python 内建 NotImplementedError 子类），接口签名即 M5 契约；
// createWindow 真实命令经 IWindowFactory 注册表派发。
#include "command_service.h"

#include "registry.h"

namespace perception {
namespace python {

std::string CommandServiceImpl::createWindow(const std::string& title) {
    IWindowFactory* factory = getWindowFactory();
    return factory ? factory->createWindow(title) : std::string();
}

py::object CommandServiceImpl::load(const std::string& /*path*/) {
    throw NotImplementedError("load() not implemented yet (M5)");  // 数据加载（load_plt 等）
}

py::object CommandServiceImpl::transform(py::object /*ds*/,
                                         const std::string& /*name*/,
                                         py::kwargs /*kwargs*/) {
    throw NotImplementedError("transform() not implemented yet (M5)");  // 数据变换
}

py::object CommandServiceImpl::query(py::object /*ds*/, py::kwargs /*kwargs*/) {
    throw NotImplementedError("query() not implemented yet (M5)");  // 数据查询
}

py::object CommandServiceImpl::exportData(py::object /*ds*/,
                                          const std::string& /*path*/) {
    throw NotImplementedError("export() not implemented yet (M5)");  // 数据导出
}

}  // namespace python
}  // namespace perception
