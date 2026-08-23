"""pytest 配置：命令层测试。

布局约定：
- 本目录测试 Python 命令驱动层（perception_py），覆盖真实使用链路：
  加载（load_plt）→ 变换（transform）→ 查询（query）→ 导出（export）。
- 命令层通过 pybind11 模块 perception_py 暴露 src/core 能力。
- M5 之前模块未构建，fixture 自动 skip；M5 后移除 skip 并补真实断言。

运行：python -m pytest tests/python -v
"""

import pytest


@pytest.fixture(scope="session")
def perception_py():
    """返回已导入的 perception_py 模块（M5 后接入构建）。"""
    try:
        import perception_py  # 由 src/python/ 构建产出
    except ImportError:
        pytest.skip("perception_py not built yet (M5)", allow_module_level=True)
    return perception_py


@pytest.fixture
def sample_plt(tmp_path):
    """生成一份最小 .plt 曲线数据文件（M1 解析实现后可用）。"""
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
