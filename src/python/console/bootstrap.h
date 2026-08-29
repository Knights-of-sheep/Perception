// ===== perception_console 模块：引导脚本（006 US4）=====
// 迁移自 src/ui/console/python_bridge.cpp 的 kBootstrap，内容不变，
// 仅新增 _run_single（等价替代 Py_single_input 的交互式执行语义）。
#pragma once

namespace perception {
namespace python {
namespace bootstrap {

// 引导脚本：续行判定 / traceback 格式化 / 空 stdin / PerceptionLogHandler /
// _run_single。注入 _cpp_log 后、任何命令执行前必须执行一次（Py_file_input）。
extern const char kBootstrap[];

}  // namespace bootstrap
}  // namespace python
}  // namespace perception
