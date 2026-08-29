"""pytest 配置：桥接与命令层测试（006 US4）。

布局约定：
- 本目录测试 pybind11 桥接模块：
    perception_console（REPL 桥：ConsoleOut / _cpp_log / 引导脚本）
    perception_py（命令层：ICommandService 骨架 + create_window 真实命令）
- .pyd 由 CMake 构建产出至构建目录（exe 同级）。测试时经环境变量
  PERCEPTION_PYD_DIR 或常见构建目录约定注入 sys.path。
- 模块未构建时对应 fixture 自动 skip。

运行：python -m pytest tests/python -v
"""

import os
import sys

import pytest

# 本文件位于 tests/python/，项目根需要上两级
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_BUILD_CANDIDATES = [
    os.environ.get("PERCEPTION_PYD_DIR"),
    os.path.join(_PROJECT_ROOT, "bin"),          # 直接 cmake 单配置
    os.path.join(_PROJECT_ROOT, "bin", "Release"),
    os.path.join(_PROJECT_ROOT, "bin", "Debug"),
    os.path.join(_PROJECT_ROOT, "bin", "RelWithDebInfo"),
    os.path.join(_PROJECT_ROOT, "build"),
    os.path.join(_PROJECT_ROOT, "build", "Debug"),
    os.path.join(_PROJECT_ROOT, "build", "Release"),
    os.path.join(_PROJECT_ROOT, "build", "RelWithDebInfo"),
]
# insert(0) 会让后插入的目录排在前面；reversed 保证 _BUILD_CANDIDATES 中
# 靠前的候选（bin 输出目录）最终优先级最高，避免加载 build/ 下的残留 pyd。
for _d in reversed(_BUILD_CANDIDATES):
    if _d and os.path.isdir(_d) and _d not in sys.path:
        sys.path.insert(0, _d)


@pytest.fixture(scope="session")
def perception_console():
    """返回已导入的 perception_console 模块（REPL 桥，006 US4）。"""
    try:
        import perception_console
    except ImportError:
        pytest.skip("perception_console not built (006 US4)", allow_module_level=True)
    return perception_console


@pytest.fixture(scope="session")
def perception_py():
    """返回已导入的 perception_py 模块（命令层，006 US4）。"""
    try:
        import perception_py
    except ImportError:
        pytest.skip("perception_py not built (006 US4)", allow_module_level=True)
    return perception_py


@pytest.fixture
def sample_plt(tmp_path):
    """生成一份最小 .plt 曲线数据文件。"""
    content = (
        "title = \"smoke\"\n"
        "xlabel = \"vds (V)\"\n"
        "ylabel = \"id (A)\"\n"
        "0.0 0.1\n"
        "1.0 0.5\n"
        "2.0 1.2\n"
    )
    path = tmp_path / "sample.plt"
    path.write_text(content, encoding="utf-8")
    return str(path)
