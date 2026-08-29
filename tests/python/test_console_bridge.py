"""REPL 桥模块测试（006 US4）。

覆盖：
- ConsoleOut 流对象：write/flush/writelines 经 IOutputSink 虚接口回调
- install_console_bridge：注入 _cpp_log、执行引导脚本（_check_complete /
  _format_traceback / _run_single 可用）
- _cpp_log：Python logging → ILogBridge 转发
（redirect=False：测试不污染宿主进程的 sys.std*）
"""

import pytest


def _make_sink(perception_console):
    class MockSink(perception_console.IOutputSink):
        def __init__(self):
            super().__init__()
            self.writes = []
            self.flushed = 0

        def write(self, text, is_stderr=False):
            self.writes.append((text, is_stderr))

        def flush(self):
            self.flushed += 1

    return MockSink()


def _make_bridge(perception_console):
    class MockBridge(perception_console.ILogBridge):
        def __init__(self):
            super().__init__()
            self.records = []

        def log(self, level, source, message):
            self.records.append((level, source, message))

    return MockBridge()


def _install(perception_console, sink, bridge):
    globals_ = {}
    perception_console.install_console_bridge(
        globals_, sink, sink, bridge, redirect=False)
    return globals_


def test_console_out_write_forwards_to_sink(perception_console):
    sink = _make_sink(perception_console)
    out = perception_console.ConsoleOut(sink, False)
    out.write("hello")
    out.flush()
    assert sink.writes == [("hello", False)]
    assert sink.flushed == 1


def test_console_out_stderr_flag(perception_console):
    sink = _make_sink(perception_console)
    err = perception_console.ConsoleOut(sink, True)
    err.write("boom")
    assert sink.writes == [("boom", True)]


def test_console_out_writelines(perception_console):
    sink = _make_sink(perception_console)
    out = perception_console.ConsoleOut(sink, False)
    out.writelines(["a", "b"])
    assert sink.writes == [("a", False), ("b", False)]


def test_install_bridge_injects_bootstrap(perception_console):
    sink = _make_sink(perception_console)
    bridge = _make_bridge(perception_console)
    globals_ = _install(perception_console, sink, bridge)
    for name in ("_cpp_log", "_check_complete", "_format_traceback", "_run_single"):
        assert callable(globals_[name]), name


def test_check_complete_verdicts(perception_console):
    sink = _make_sink(perception_console)
    bridge = _make_bridge(perception_console)
    globals_ = _install(perception_console, sink, bridge)
    check = globals_["_check_complete"]
    # 完整语句 → complete
    assert check("x = 1") == "complete"
    # 表达式 → complete
    assert check("1 + 1") == "complete"
    # 续行（缺缩进体）→ incomplete
    assert check("for i in []:") == "incomplete"
    # 语法错误 → error
    assert check("def f(:") == "error"


def test_cpp_log_forwards_to_bridge(perception_console):
    sink = _make_sink(perception_console)
    bridge = _make_bridge(perception_console)
    globals_ = _install(perception_console, sink, bridge)
    globals_["_cpp_log"](2, "test:1", "hello from python")
    assert bridge.records == [(2, "test:1", "hello from python")]


def test_cpp_log_clamps_out_of_range_level(perception_console):
    sink = _make_sink(perception_console)
    bridge = _make_bridge(perception_console)
    globals_ = _install(perception_console, sink, bridge)
    globals_["_cpp_log"](99, "test:2", "out of range")  # 越界回退 Info(1)
    assert bridge.records == [(1, "test:2", "out of range")]
