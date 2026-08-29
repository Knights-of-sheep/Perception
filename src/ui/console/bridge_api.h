// ===== REPL 桥回调虚接口（006 US4：pybind11 迁移 + 虚接口抽象）=====
// 职责：perception_console 模块（.pyd）与 C++ 侧（PythonConsole）之间的回调契约。
//   - IOutputSink：ConsoleOut 自定义流对象写 stdout/stderr 的目标
//   - ILogBridge：Python logging → C++ 统一日志流的转发目标（_cpp_log）
// 约定：实例由 PythonConsole 创建，经 py::capsule 传入 perception_console 模块，
//       模块内 capsule.get_pointer<T>() 取回后按纯虚接口调用；
//       pyd 仅依赖本头（vtable 调用），不链接 perception_ui 静态库。
// 本头 MUST NOT 包含 Python.h（保持纯虚、跨层零依赖）。
#pragma once

namespace perception {
namespace ui {
namespace console {

// 输出流写目标：Python 侧 sys.stdout/sys.stderr 重定向后文本落点。
class IOutputSink {
public:
    virtual ~IOutputSink() = default;

    // 追加文本到 REPL 控件；isStderr=true 用错误色。text 为 UTF-8，长度 len。
    virtual void write(const char* text, int len, bool isStderr) = 0;
    // 冲刷（Python 侧 flush() 调用；Qt 控件无缓冲，可空实现）
    virtual void flush() = 0;
};

// Python logging → C++ 统一日志流 的转发目标。
class ILogBridge {
public:
    virtual ~ILogBridge() = default;

    // 记录一条日志：level 为 C++ LogLevel 整数值（0=Debug..4=Fatal，越界回退 Info）；
    // source 形如 "name:lineno"，message 为 UTF-8。不得抛异常逃逸到 Python。
    virtual void log(int level, const char* source, const char* message) = 0;
};

}  // namespace console
}  // namespace ui
}  // namespace perception
