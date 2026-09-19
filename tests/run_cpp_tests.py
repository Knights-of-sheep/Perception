#!/usr/bin/env python3
"""Perception C++ 单元测试运行器（tests/run_cpp_tests.py）

封装 `ctest`：定位构建目录并运行全部 C++ 单元测试，输出摘要与可选 JUnit XML 报告。
对应 spec 011 FR-010（tests/ 下测试辅助脚本）。

依赖:
- CMake 附带的 `ctest`（与构建用的 cmake 同目录）或 PATH 中的 ctest
- 已存在的构建目录（默认 build/ 或 build-gui/，可用 --build-dir 指定）

用法:
    python tests/run_cpp_tests.py --config Release
    python tests/run_cpp_tests.py --build-dir build --junit report.xml
    python tests/run_cpp_tests.py --list          # 仅列出测试，不运行
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DIRS = ["build", "build-gui"]


def find_ctest() -> str:
    exe = shutil.which("ctest")
    if exe:
        return exe
    cmake = shutil.which("cmake")
    if cmake:
        cand = Path(cmake).parent / "ctest.exe"
        if cand.is_file():
            return str(cand)
    raise SystemExit(
        "未找到 ctest（请先安装 CMake 或将其加入 PATH；或经 scripts/build.ps1 -Gui 构建后由 cmake 自带）"
    )


def locate_build_dir(explicit: str | None) -> Path:
    if explicit:
        p = Path(explicit)
        if not p.is_dir():
            raise SystemExit(f"构建目录不存在: {p}")
        return p
    for name in DEFAULT_DIRS:
        p = ROOT / name
        if p.is_dir():
            return p
    raise SystemExit(
        "未发现构建目录（build/ 或 build-gui/）。请先运行 scripts/build.ps1 构建。"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Perception C++ 单元测试运行器")
    parser.add_argument("--config", default="Release", choices=["Debug", "Release"])
    parser.add_argument("--build-dir", default=None,
                        help="构建目录（默认自动探测 build/ 或 build-gui/）")
    parser.add_argument("--junit", default=None, help="输出 JUnit XML 报告路径")
    parser.add_argument("--timeout", type=int, default=None, help="单测试超时秒数")
    parser.add_argument("--list", action="store_true", help="仅列出测试，不运行")
    args = parser.parse_args()

    ctest = find_ctest()
    build_dir = locate_build_dir(args.build_dir)

    cmd = [ctest, "--test-dir", str(build_dir), "-C", args.config, "--output-on-failure"]
    if args.timeout:
        cmd += ["--timeout", str(args.timeout)]
    if args.junit:
        cmd += ["--output-junit", args.junit]
    if args.list:
        cmd += ["-N"]

    print(f"==> ctest 目录: {build_dir} (config={args.config})")
    rc = subprocess.run(cmd).returncode
    if rc != 0 and not args.list:
        print(f"[FAIL] CTest 退出码 {rc}")
        return rc
    print("CTest 完成" if not args.list else "已列出测试")
    return 0


if __name__ == "__main__":
    sys.exit(main())
