// ===== perception_console 模块：ConsoleOut 流对象（006 US4）=====
// 职责：Python 侧 sys.stdout/sys.stderr 的自定义流对象，write/flush/writelines
//       经 IOutputSink 虚接口回调 C++ 侧（PythonConsole）追加到控件。
// 迁移自 src/ui/console/python_bridge.cpp 的 ConsoleOutObject（手写 C API →
// pybind11 py::class_），行为等价。
#pragma once

#include "ui/console/bridge_api.h"

#include <pybind11/pybind11.h>

#include <string>

namespace py = pybind11;

namespace perception {
namespace python {

// Python 可写流对象（只重写 write/flush/writelines；文本为 UTF-8）。
class ConsoleOut {
public:
    ConsoleOut(ui::console::IOutputSink* sink, bool isStderr);
    void write(const std::string& text);
    void flush();
    void writelines(py::iterable lines);

private:
    ui::console::IOutputSink* sink_ = nullptr;  // 借用指针（持有方为 PythonConsole/调用方）
    bool isStderr_ = false;
};

}  // namespace python
}  // namespace perception
