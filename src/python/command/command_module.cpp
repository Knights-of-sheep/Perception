// ===== perception_py：命令层 pybind11 模块（006 US4，一接口类 = 一动态库）=====
// 导出：
//   - IWindowFactory：create_window 命令实现侧接口（Python 可继承：pytest mock）
//   - ICommandService / CommandService：命令驱动层对外虚接口骨架
//     （create_window 真实命令 + load/transform/query/export 占位抛 NotImplementedError）
//   - create_window(title="")：模块函数（REPL globals 注入 / 直接调用）
//   - register_window_factory(factory)：宿主注册 IWindowFactory（capsule / 实例 / None）
// 契约：specs/004-dock-layout-manager/contracts/python-create-window.md
// Python.h / pybind11 最先包含（规避 Qt slots/signals/emit 宏与 object.h 冲突）。
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "python/api/i_command_service.h"
#include "python/api/i_window_factory.h"

#include "command_service.h"
#include "registry.h"

namespace py = pybind11;

using perception::python::ICommandService;
using perception::python::IWindowFactory;

namespace {

// IWindowFactory 的 Python trampoline（Python 可继承实现：pytest mock / 二次开发）。
// 注意：C++ 虚函数 createWindow 对应 Python 侧绑定名 create_window（蛇形），
// 必须用 PYBIND11_OVERRIDE_PURE_NAME 显式映射，否则 get_override("createWindow")
// 查不到 Python override，误报 pure virtual。
class PyWindowFactory : public IWindowFactory {
public:
    using IWindowFactory::IWindowFactory;
    std::string createWindow(const std::string& title) override {
        PYBIND11_OVERRIDE_PURE_NAME(std::string, IWindowFactory, "create_window",
                                    createWindow, title);
    }
};

// 持有 Python 侧工厂对象，防止临时实例（如 FailingFactory()）注册后立即被 GC 导致
// 注册表裸指针悬垂。capsule 场景（宿主 C++ 侧）生命周期归宿主，不在此持有。
py::object g_factoryHolder;

// 注册 IWindowFactory：接受 None（清空）/ capsule（宿主 C++ 侧）/ trampoline 实例（Python 侧）。
void registerWindowFactory(py::object factory) {
    if (factory.is_none()) {
        perception::python::setWindowFactory(nullptr);
        g_factoryHolder = py::object();
        return;
    }
    if (py::isinstance<py::capsule>(factory)) {
        perception::python::setWindowFactory(
            factory.cast<py::capsule>().get_pointer<IWindowFactory>());
        g_factoryHolder = py::object();  // 宿主 C++ 对象，生命周期归注册方
        return;
    }
    try {
        perception::python::setWindowFactory(factory.cast<IWindowFactory*>());
    } catch (const py::cast_error&) {
        throw py::type_error(
            "register_window_factory: expected IWindowFactory instance or capsule");
    }
    g_factoryHolder = factory;  // 持有引用：trampoline 实例必须存活到注销
}

// create_window 真实命令（契约 python-create-window.md）：
//   宿主未连接（未注册）→ None；实现侧失败（空 id）→ None；否则返回 id 字符串。
// 注意：不能用 `id.empty() ? py::none() : py::str(id)`——三元运算符会经 pybind11
// 非 explicit 的 py::none(const object&) 构造把 py::str 转成 py::none 而抛
// "Object of type 'str' is not an instance of 'none'"（Py_None_Check 失败）。
py::object createWindow(const std::string& title) {
    IWindowFactory* factory = perception::python::getWindowFactory();
    if (!factory) return py::none();
    const std::string id = factory->createWindow(title);
    if (id.empty()) return py::none();
    return py::str(id);
}

}  // namespace

PYBIND11_MODULE(perception_py, m) {
    m.doc() = "Perception command layer module (006 US4)";

    // ---- 对外虚接口：IWindowFactory（Python 可继承：mock / 二次开发）----
    py::class_<IWindowFactory, PyWindowFactory>(m, "IWindowFactory")
        .def(py::init<>())
        .def("create_window",
             [](IWindowFactory& self, const std::string& title) {
                 return self.createWindow(title);
             },
             py::arg("title") = std::string());

    // ---- 命令驱动层对外虚接口（本期骨架）----
    // 注册占位命令异常：C++ NotImplementedError → Python 内建 NotImplementedError 子类
    py::register_exception<perception::python::NotImplementedError>(
        m, "NotImplementedError", PyExc_NotImplementedError);

    py::class_<ICommandService>(m, "ICommandService");  // 抽象类型标记（接口即库边界）
    py::class_<perception::python::CommandServiceImpl, ICommandService>(
        m, "CommandService")
        .def(py::init<>())
        .def("create_window",
             &perception::python::CommandServiceImpl::createWindow,
             py::arg("title") = std::string())
        // 009-supported-file-types：只读查询（契约 python-supported-formats.md）
        .def("supported_formats",
             &perception::python::CommandServiceImpl::supportedFormats)
        .def("load", &perception::python::CommandServiceImpl::load, py::arg("path"))
        .def("transform", &perception::python::CommandServiceImpl::transform,
             py::arg("ds"), py::arg("name"))
        .def("query", &perception::python::CommandServiceImpl::query, py::arg("ds"))
        .def("export", &perception::python::CommandServiceImpl::exportData,
             py::arg("ds"), py::arg("path"));

    // ---- create_window 模块函数（REPL globals 注入 / pytest 直接调用）----
    m.def("create_window", &createWindow, py::arg("title") = std::string(),
          "Create a rendering subwindow; returns its id ('Plot_N') or None if host disconnected");

    // ---- IWindowFactory 注册表（宿主侧）----
    m.def("register_window_factory", &registerWindowFactory, py::arg("factory"),
          "Register the IWindowFactory implementation (host side); pass None to clear");
}
