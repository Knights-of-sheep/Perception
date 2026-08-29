"""命令层骨架测试（006 US4）。

覆盖：
- perception_py 可独立 import（一接口类 = 一动态库，Python 侧按模块加载）
- CommandService 接口签名（create_window / load / transform / query / export）
- 占位命令（load/transform/query/export）抛 NotImplementedError（M5 实现）
- create_window 真实命令：经 IWindowFactory 虚接口派发（mock 实现验证契约）
  契约：specs/004-dock-layout-manager/contracts/python-create-window.md
"""

import pytest


def test_module_importable(perception_py):
    assert callable(perception_py.create_window)
    assert callable(perception_py.register_window_factory)
    assert hasattr(perception_py, "IWindowFactory")
    assert hasattr(perception_py, "ICommandService")
    assert hasattr(perception_py, "CommandService")


def test_command_service_interface(perception_py):
    svc = perception_py.CommandService()
    for name in ("create_window", "load", "transform", "query", "export"):
        assert callable(getattr(svc, name)), name


def test_placeholder_commands_raise_not_implemented(perception_py, sample_plt):
    svc = perception_py.CommandService()
    with pytest.raises(NotImplementedError):
        svc.load(sample_plt)
    with pytest.raises(NotImplementedError):
        svc.transform([], "unit_scale")
    with pytest.raises(NotImplementedError):
        svc.query([])
    with pytest.raises(NotImplementedError):
        svc.export([], "out.csv")


def test_create_window_without_host_returns_none(perception_py):
    # 宿主未注册 IWindowFactory → None（契约「宿主未连接」）
    perception_py.register_window_factory(None)
    assert perception_py.create_window() is None


def test_create_window_mock_factory(perception_py):
    class MockFactory(perception_py.IWindowFactory):
        def __init__(self):
            super().__init__()
            self.calls = []

        def create_window(self, title):
            self.calls.append(title)
            return "Plot_1"

    factory = MockFactory()
    perception_py.register_window_factory(factory)
    try:
        # 显式 title
        assert perception_py.create_window("曲线图") == "Plot_1"
        # 缺省 title（空串，契约：实现侧负责缺省 = id）
        assert perception_py.create_window() == "Plot_1"
        assert factory.calls == ["曲线图", ""]
        # 非 str → TypeError（契约）
        with pytest.raises(TypeError):
            perception_py.create_window(123)
    finally:
        perception_py.register_window_factory(None)


def test_create_window_factory_failure_returns_none(perception_py):
    class FailingFactory(perception_py.IWindowFactory):
        def __init__(self):
            super().__init__()

        def create_window(self, title):
            return ""  # 实现侧失败 → 空串 → 模块层返回 None

    perception_py.register_window_factory(FailingFactory())
    try:
        assert perception_py.create_window("x") is None
    finally:
        perception_py.register_window_factory(None)
