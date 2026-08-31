// ===== 命令驱动层对外虚接口：ICommandService（006 US4）=====
// 职责：数据 CRUD 的唯一入口（宪法「命令驱动与 Python 包」：perception 整体功能、
//       extract 数据计算，对标 SVisual）。本期为骨架：
//       - createWindow：真实命令，打通「Python import → 虚接口 → C++ 实现」全链路
//       - load/transform/query/export：M5 占位，抛 NotImplementedError
// 一接口类 = 一动态库：本接口对应 perception_py 命令层模块。
// 本头依赖 pybind11（py::object 数据类型），仅供 src/python 侧目标包含；
// UI 侧不得包含（需要跨层接口请用 i_window_factory.h / bridge_api.h）。
#pragma once

#include <pybind11/pybind11.h>

#include <string>

namespace py = pybind11;

namespace perception {
namespace python {

class ICommandService {
public:
    virtual ~ICommandService() = default;

    // 真实命令：创建渲染子窗口（契约 python-create-window.md），经 IWindowFactory 派发。
    virtual std::string createWindow(const std::string& title) = 0;

    // 真实命令（只读查询）：返回文件类型权威目录完整清单
    // （契约 python-supported-formats.md，009-supported-file-types）。
    virtual py::object supportedFormats() = 0;

    // ---- M5 占位命令：本期一律抛 NotImplementedError ----
    virtual py::object load(const std::string& path) = 0;
    virtual py::object transform(py::object ds, const std::string& name, py::kwargs kwargs) = 0;
    virtual py::object query(py::object ds, py::kwargs kwargs) = 0;
    virtual py::object exportData(py::object ds, const std::string& path) = 0;
};

}  // namespace python
}  // namespace perception
