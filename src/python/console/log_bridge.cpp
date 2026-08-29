// ===== _cpp_log 桥实现（006 US4）=====
// 行为与迁移前（python_bridge.cpp cpp_log_impl）等价：防御 level 越界回退 Info；
// 经 ILogBridge::log 转发，无异常逃逸（桥接失败不影响 Python 执行）。
#include "log_bridge.h"

namespace perception {
namespace python {

py::cpp_function makeCppLog(ui::console::ILogBridge* bridge) {
    return py::cpp_function(
        [bridge](int level, const std::string& source, const std::string& message) {
            if (!bridge) return;
            const int lvl = (level < 0 || level > 4) ? 1 : level;  // 防御：越界回退 Info
            bridge->log(lvl, source.c_str(), message.c_str());
        },
        py::arg("level"), py::arg("source"), py::arg("message"));
}

}  // namespace python
}  // namespace perception
