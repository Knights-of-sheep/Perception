#!/usr/bin/env python3
"""Perception Python 命令层测试运行器（tests/run_python_tests.py）

封装 `pytest`：运行 tests/python 下命令层测试，输出摘要与可选报告文件。
对应 spec 011 FR-010（tests/ 下测试辅助脚本）。

依赖:
- pytest（安装：python -m pip install pytest）
- 命令层 Python 包（perception / extract）：经构建后的可导入环境

用法:
    python tests/run_python_tests.py
    python tests/run_python_tests.py --report report.txt
    python tests/run_python_tests.py -k extract        # 仅运行匹配用例
    python tests/run_python_tests.py tests/python/test_io.py -v
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PYTHON_TESTS = ROOT / "tests" / "python"


def main() -> int:
    parser = argparse.ArgumentParser(description="Perception Python 命令层测试运行器")
    parser.add_argument("--tests-dir", default=str(PYTHON_TESTS),
                        help="测试目录（默认 tests/python）")
    parser.add_argument("--report", default=None,
                        help="输出摘要报告路径（纯文本，含 pytest 标准输出）")
    parser.add_argument("pytest_args", nargs="*",
                        help="透传给 pytest 的额外参数（如 具体文件 / -k / -v / -x）")
    args = parser.parse_args()

    if not Path(args.tests_dir).is_dir():
        print(f"[err] 测试目录不存在: {args.tests_dir}")
        return 2

    cmd = [sys.executable, "-m", "pytest", args.tests_dir, "-v"] + args.pytest_args
    print(f"==> pytest: {args.tests_dir}")

    try:
        proc = subprocess.run(cmd, capture_output=True, text=True)
    except FileNotFoundError:
        print("[err] 未找到 pytest，请先安装：python -m pip install pytest")
        return 2

    print(proc.stdout)
    if proc.stderr:
        print(proc.stderr, file=sys.stderr)

    if args.report:
        Path(args.report).write_text(proc.stdout, encoding="utf-8")
        print(f"[report] 已写入 {args.report}")

    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
