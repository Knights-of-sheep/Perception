# Quickstart: Constitution Compliance Refactor

**Branch**: `006-constitution-refactor` | **Date**: 2026-08-29 | **Plan**: [plan.md](plan.md)

> 验证指南：证明重构后代码库符合宪法 v4.0.0 且行为等价。契约见 [contracts/interfaces.md](contracts/interfaces.md)，目标结构见 [data-model.md](data-model.md)。

## 前置条件

- Windows + VS2022（MSVC）+ Ninja + CMake ≥ 3.16（现有 `scripts/build.ps1` 环境）。
- `clang-format`（LLVM）可用：`winget install LLVM` 或 VS2022 C++ 工具链组件；`clang-format --version` 可运行。
- Python 3.13（Anaconda，与内嵌运行时一致）用于 pytest。

## 场景 1：行数合规检查（FR-001/002/003）

**命令**：

```powershell
# 新增门禁脚本（tasks 阶段实现）
powershell -File scripts/check_line_counts.ps1
```

**预期**：
- 输出所有 `.cpp`/`.h`/`.hpp` 行数与上限对照表。
- `.cpp` > 800、普通 `.h` > 500（红线）、`.hpp` > 800 均报 ERROR 并退出码非 0。
- 重构后 `MainWindow.cpp` ≤ 800、`theme_catalog*.h` 各 < 300、无新增超标文件。

## 场景 2：#pragma once 检查（FR-004）

**命令**：

```powershell
powershell -File scripts/check_pragma_once.ps1
```

**预期**：所有 `src/` 与 `tests/cpp/` 下的 `.h`/`.hpp` 首部含 `#pragma once`；缺失项报 ERROR。

## 场景 3：格式门禁（FR-014 / SC-007）

**命令**：

```powershell
# 全库一次性改写（仅实施阶段执行一次，单独提交）
powershell -File scripts/format_all.ps1
# 之后每次变更用检查模式
powershell -File scripts/format_all.ps1 -Check
```

**预期**：`-Check` 模式 `clang-format --dry-run --Werror` 零输出、退出码 0；任何未格式化文件报错。

## 场景 4：行为等价回归（FR-011/FR-013 / SC-002）

**命令**：

```powershell
ctest --test-dir build --output-on-failure        # tests/cpp 单元测试
pytest tests/python -q                            # 命令层/Python 逻辑
```

**预期**：
- `ctest` 与 `pytest` 100% 通过，失败数为 0。
- 测试断言文件（`tests/cpp/*`、`tests/python/*`）除格式对齐外零 diff。
- 每次拆分提交后立即重跑，回归可定位到具体提交。

## 场景 5：截图回归（SC-003/SC-006 代理）

**命令**：

```powershell
powershell -File scripts/update_screenshots.ps1    # 或既有 --snapshot 机制
```

**预期**：启动快照与主题切换快照与重构前对比一致（无布局/配色偏移）；`docs/design/mockups/` 对比通过。

## 场景 6：人工评审清单（SC-004/005）

- 代码评审对照宪法逐项核对：
  - 无 God Class（成员函数 ≤ 30）、无 public 裸暴露成员、无超长函数（> 120 行）。
  - 命名：类名大驼峰、成员 `name_` 后缀、无下划线开头、无拼音命名。
  - 依赖：`git diff` 中 `CMakeLists.txt`/`requirements*` 无新增第三方运行依赖。
- `git diff --stat` 确认格式对齐与逻辑变更分次提交（spec Assumptions）。

## 完成定义（对应 SC）

| 场景 | 对应成功标准 | 通过条件 |
|------|--------------|----------|
| 1 | SC-001 | 100% 文件满足分级行数上限 |
| 2 | SC-003 | 100% 头文件带 `#pragma once` |
| 3 | SC-007 | 全库格式检查零失败 |
| 4 | SC-002 | 测试 100% 通过且断言行零变更 |
| 5 | SC-003 | 截图对比无差异 |
| 6 | SC-004/005 | 评审零违规项、零新运行依赖 |
