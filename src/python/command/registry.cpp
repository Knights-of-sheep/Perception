// ===== IWindowFactory 注册表实现（006 US4）=====
// 模块全局裸指针；生命周期归注册方（MainWindow 链路的 PythonConsole 适配器 /
// pytest mock），模块不持有所有权，shutdown 时须 register(nullptr)。
#include "registry.h"

namespace perception {
namespace python {

namespace {
IWindowFactory* g_windowFactory = nullptr;
}

void setWindowFactory(IWindowFactory* factory) { g_windowFactory = factory; }

IWindowFactory* getWindowFactory() { return g_windowFactory; }

}  // namespace python
}  // namespace perception
