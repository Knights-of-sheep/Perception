// ===== 命令层对外虚接口：IWindowFactory（006 US4，一接口类 = 一动态库）=====
// 职责：create_window 命令的 C++ 实现侧契约。perception_py 模块（.pyd）内
//       create_window 绑定经注册的 IWindowFactory 实例派发至真实 UI 实现。
// 实现方：MainWindow（调用 createSubwindow 真实创建渲染子窗口）；
//         pytest 用 mock 实现验证接口派发与契约行为。
// 注册方式：实现方经 registry.h 的 setWindowFactory(IWindowFactory*) 注册，
//           实例指针以普通裸指针持有（生命周期归注册方，模块不拥有）。
// 本头 MUST NOT 包含 Python.h（跨层零依赖，UI 侧可直接包含）。
#pragma once

#include <string>

namespace perception {
namespace python {

// 渲染子窗口创建工厂（契约 specs/004-dock-layout-manager/contracts/python-create-window.md）。
class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;

    // 创建渲染子窗口并返回生成的 id（"Plot_" + 全局递增序号）。
    // title 空白时由实现侧默认 title = id；失败（宿主未连接/异常）返回空串。
    virtual std::string createWindow(const std::string& title) = 0;
};

}  // namespace python
}  // namespace perception
