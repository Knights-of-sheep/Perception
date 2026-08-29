// ===== perception_console 模块：_cpp_log 桥（006 US4）=====
// 构造 Python 侧可调用对象 _cpp_log(level, source, message)，经 ILogBridge
// 虚接口转发至 C++ 统一日志流（契约 specs/001-unified-logging/contracts/python-log-bridge.md）。
#pragma once

#include "ui/console/bridge_api.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace perception {
namespace python {

// 构造 _cpp_log 可调用对象（level 越界回退 Info；错误在内部捕获，不逃逸到 Python）。
// bridge 借用指针（持有方为 PythonConsole），寿命须长于本可调用对象的使用期。
py::cpp_function makeCppLog(ui::console::ILogBridge* bridge);

}  // namespace python
}  // namespace perception
