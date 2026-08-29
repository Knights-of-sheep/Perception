// ===== ConsoleOut 流对象实现（006 US4）=====
// 行为与迁移前（python_bridge.cpp ConsoleOutObject）等价：
//   write(text)   → IOutputSink::write(text, isStderr)，isStderr 决定错误色
//   flush()       → 空实现（Qt 控件无缓冲）
//   writelines()  → 逐行转 write（绝大多数库只用 write/flush）
#include "console_out.h"

#include <pybind11/pybind11.h>

namespace perception {
namespace python {

ConsoleOut::ConsoleOut(ui::console::IOutputSink* sink, bool isStderr)
    : sink_(sink), isStderr_(isStderr) {}

void ConsoleOut::write(const std::string& text) {
    if (sink_) {
        sink_->write(text.data(), static_cast<int>(text.size()), isStderr_);
    }
}

void ConsoleOut::flush() {
    if (sink_) sink_->flush();
}

void ConsoleOut::writelines(py::iterable lines) {
    for (py::handle line : lines) {
        write(py::str(line).cast<std::string>());
    }
}

}  // namespace python
}  // namespace perception
