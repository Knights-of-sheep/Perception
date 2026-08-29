// ===== perception_py 模块：IWindowFactory 注册表（006 US4）=====
// create_window 真实命令的 C++ 实现侧注册：
//   - 宿主（PythonConsole 的 WindowFactoryAdapter）在 initPython 时经
//     py::capsule 注册；pytest 经 trampoline 子类实例注册。
//   - 裸指针持有（生命周期归注册方）；shutdown 前 MUST register(nullptr) 清空。
#pragma once

#include "python/api/i_window_factory.h"

namespace perception {
namespace python {

// 注册 / 清空窗口工厂（初始化期单线程调用）。
void setWindowFactory(IWindowFactory* factory);
// 取当前注册实例（未注册返回 nullptr，create_window 将返回 None）。
IWindowFactory* getWindowFactory();

}  // namespace python
}  // namespace perception
