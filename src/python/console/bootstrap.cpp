// ===== 引导脚本常量（006 US4，行为与迁移前一致）=====
// 续行判定（codeop.compile_command 为无状态的标准判定器，返回 None=不完整 /
// code=完整 / 抛异常=语法错误；与 code.InteractiveConsole 同源）
// + traceback 格式化 + 空 stdin（防止 help() 等交互函数读输入时阻塞 GUI）
// + PerceptionLogHandler：Python logging → _cpp_log 桥接（FR-008，契约 python-log-bridge.md）
// + _run_single：compile(...,'single') + exec —— 等价替代 Py_single_input 的
//   交互式语义（表达式语句自动求值打印）。
#include "bootstrap.h"

namespace perception {
namespace python {
namespace bootstrap {

const char kBootstrap[] =
    "import codeop, sys, traceback\n"
    "def _check_complete(src):\n"
    "    try:\n"
    "        result = codeop.compile_command(src, '<console>', symbol='single')\n"
    "    except KeyboardInterrupt:\n"
    "        return 'interrupt'  # 挂起的 Ctrl+C：标准 REPL 中断当前命令，不执行\n"
    "    except (SyntaxError, OverflowError, ValueError, TypeError):\n"
    "        return 'error'\n"
    "    return 'incomplete' if result is None else 'complete'\n"
    "def _format_traceback(tb):\n"
    "    return ''.join(traceback.format_exception(*tb))\n"
    "class _EmptyIn:\n"
    "    def read(self, *a, **k): return ''\n"
    "    def readline(self, *a, **k): return ''\n"
    "    def readlines(self, *a, **k): return []\n"
    "    def isatty(self): return False\n"
    "sys.stdin = _EmptyIn()\n"
    "import logging\n"
    "_LEVEL_MAP = {10: 0, 20: 1, 30: 2, 40: 3, 50: 4}  # Python -> C++ LogLevel\n"
    "class PerceptionLogHandler(logging.Handler):\n"
    "    def emit(self, record):\n"
    "        try:\n"
    "            level = _LEVEL_MAP.get(record.levelno, 1)\n"
    "            source = '%s:%d' % (record.name, record.lineno)\n"
    "            _cpp_log(level, source, record.getMessage())\n"
    "        except Exception:\n"
    "            self.handleError(record)  # 桥接失败不影响 Python 执行\n"
    "logging.getLogger().addHandler(PerceptionLogHandler())\n"
    "logging.getLogger().setLevel(logging.DEBUG)\n"
    "def _run_single(src, globals):\n"
    "    # 等价替代 CPython C API 的 PyRun_String(src, Py_single_input)：\n"
    "    # 'single' 模式的 code object 内嵌表达式自动打印指令，exec 与交互式一致\n"
    "    code = compile(src, '<stdin>', 'single')\n"
    "    exec(code, globals)\n";

}  // namespace bootstrap
}  // namespace python
}  // namespace perception
