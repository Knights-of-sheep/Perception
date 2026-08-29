// ===== perception_console：REPL 桥 pybind11 模块（006 US4）=====
// Python.h / pybind11 必须最先包含（Qt 的 slots/signals/emit 宏与 object.h 冲突，
// 沿用 PythonConsole.cpp 头部注释约定；本 TU 无 Qt 头，遵守以防未来引入）。
// 迁移自 src/ui/console/python_bridge.cpp（手写 CPython C API → pybind11），
// 行为等价：_cpp_log 注入 / 引导脚本（含 _run_single）/ sys.stdout/stderr 重定向。
// 职责边界：
//   - IOutputSink / ILogBridge 绑定（对外虚接口的 Python 镜像，可继承 mock）
//   - ConsoleOut 流对象
//   - install_console_bridge(globals, out_sink, err_sink, log_bridge)
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>

#include "ui/console/bridge_api.h"

#include "bootstrap.h"
#include "console_out.h"
#include "log_bridge.h"

namespace py = pybind11;

using perception::ui::console::IOutputSink;
using perception::ui::console::ILogBridge;
using perception::python::makeCppLog;

namespace {

// ---- 对外虚接口的 Python trampoline（Python 可继承实现，供 mock/二次开发）----
class PyOutputSink : public IOutputSink {
public:
    using IOutputSink::IOutputSink;
    void write(const char* text, int len, bool isStderr) override {
        PYBIND11_OVERRIDE_PURE(void, IOutputSink, write,
                               std::string(text, len), isStderr);
    }
    void flush() override { PYBIND11_OVERRIDE_PURE(void, IOutputSink, flush); }
};

class PyLogBridge : public ILogBridge {
public:
    using ILogBridge::ILogBridge;
    void log(int level, const char* source, const char* message) override {
        PYBIND11_OVERRIDE_PURE(void, ILogBridge, log, level,
                               std::string(source ? source : ""),
                               std::string(message ? message : ""));
    }
};

// 接口实例解析：C++ 侧经 py::capsule（持有 void*，避免库间类型注册耦合）；
// Python 侧经 trampoline 子类实例（cast<IOutputSink*>）。
IOutputSink* sinkFromObject(py::handle h) {
    if (h.is_none()) return nullptr;
    if (py::isinstance<py::capsule>(h)) {
        return h.cast<py::capsule>().get_pointer<IOutputSink>();
    }
    try {
        return h.cast<IOutputSink*>();
    } catch (const py::cast_error&) {
        return nullptr;
    }
}

ILogBridge* bridgeFromObject(py::handle h) {
    if (h.is_none()) return nullptr;
    if (py::isinstance<py::capsule>(h)) {
        return h.cast<py::capsule>().get_pointer<ILogBridge>();
    }
    try {
        return h.cast<ILogBridge*>();
    } catch (const py::cast_error&) {
        return nullptr;
    }
}

// 安装 REPL 桥：注入 _cpp_log → 执行引导脚本 → （可选）重定向 sys.stdout/stderr。
// redirect=false 供 pytest 使用（不污染宿主进程的 sys.std*）。
void installConsoleBridge(py::dict globals, py::object outSink, py::object errSink,
                          py::object logBridge, bool redirect) {
    IOutputSink* out = sinkFromObject(outSink);
    IOutputSink* err = sinkFromObject(errSink);
    ILogBridge* bridge = bridgeFromObject(logBridge);
    if (!out) out = err;  // 允许只提供一个 sink（stdout/stderr 同目标）
    if (!err) err = out;
    if (!bridge) {
        throw py::value_error("install_console_bridge: log_bridge is required");
    }

    // 1) 注入 _cpp_log（必须在引导脚本执行前，PerceptionLogHandler.emit 引用它）
    globals["_cpp_log"] = makeCppLog(bridge);

    // 2) 引导脚本（续行判定 / traceback 格式化 / 空 stdin / logging handler / _run_single）
    py::exec(perception::python::bootstrap::kBootstrap, globals, globals);

    // 3) 重定向 sys.stdout / sys.stderr → console 控件（isStderr 决定错误色）
    if (redirect) {
        py::module_ sys = py::module_::import("sys");
        if (out) {
            sys.attr("stdout") = py::cast(perception::python::ConsoleOut(out, false));
        }
        if (err) {
            sys.attr("stderr") = py::cast(perception::python::ConsoleOut(err, true));
        }
    }
}

}  // namespace

PYBIND11_MODULE(perception_console, m) {
    m.doc() = "Perception REPL console bridge module (006 US4)";

    // 对外虚接口：Python 可继承实现（pytest mock / 二次开发扩展点）
    py::class_<IOutputSink, PyOutputSink>(m, "IOutputSink")
        .def(py::init<>())
        .def("write",
             [](IOutputSink& self, const std::string& text, bool is_stderr) {
                 self.write(text.data(), static_cast<int>(text.size()), is_stderr);
             },
             py::arg("text"), py::arg("is_stderr"))
        .def("flush", &IOutputSink::flush);

    py::class_<ILogBridge, PyLogBridge>(m, "ILogBridge")
        .def(py::init<>())
        .def("log",
             [](ILogBridge& self, int level, const std::string& source,
                const std::string& message) {
                 self.log(level, source.c_str(), message.c_str());
             },
             py::arg("level"), py::arg("source"), py::arg("message"));

    // ConsoleOut 流对象（sys.stdout/stderr 重定向载体）
    py::class_<perception::python::ConsoleOut>(m, "ConsoleOut")
        .def(py::init<IOutputSink*, bool>(), py::arg("sink"), py::arg("is_stderr"))
        .def("write", &perception::python::ConsoleOut::write, py::arg("text"))
        .def("flush", &perception::python::ConsoleOut::flush)
        .def("writelines", &perception::python::ConsoleOut::writelines,
             py::arg("lines"));

    // 安装 REPL 桥：注入 _cpp_log + 执行引导脚本 + 重定向 sys.stdout/stderr
    // redirect=False 供测试使用（不污染宿主 sys.std*）
    m.def("install_console_bridge", &installConsoleBridge,
          py::arg("globals"), py::arg("out_sink"), py::arg("err_sink"),
          py::arg("log_bridge"), py::arg("redirect") = true,
          "Install REPL bridge: inject _cpp_log, run bootstrap, redirect sys.std*");
}
