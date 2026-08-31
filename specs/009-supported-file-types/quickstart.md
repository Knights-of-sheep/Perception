# Quickstart: 文件类型支持目录验证

> Phase 1 输出。端到端验证场景；实现细节见 [tasks.md](../tasks.md)（实施阶段生成）。

## 前置

- VS2022 + Ninja + CMake ≥ 3.16；CPython 3.13（含 `pytest`）。
- GUI 验证需要 Qt 5.15.2 + VTK 9.4.1（`scripts/build.ps1 -Gui` 链路）。

## 1. 核心层构建 + C++ 测试（无头）

```powershell
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

**预期**：新增 `file_type_catalog_test` 通过（目录覆盖 / 一致性 / 过滤分组 / 规划中排除 / 大小写 / 差异检测），且既有测试全部保持绿（无回归）。

## 2. pytest（命令层契约，需 GUI 构建产出 `perception_py`）

```powershell
.\scripts\build.ps1 -Gui -Pytest
# 或仅跑命令层测试：
python -m pytest tests\python\test_supported_formats.py -v
```

**预期**：`test_supported_formats.py` 全部通过（返回结构 / 字段值域 / 扩展名唯一 / 状态语义 / 调用稳定）。

## 3. 应用内验证（获取能力）

```powershell
.\bin\Release\perception.exe
```

在底部 Python Console 输入：

```python
perception_py.CommandService().supported_formats()
```

**预期**：返回完整清单（含 `.plt`/`.csv` 为 `supported`，`.tdr`/`.dat`/`.vtk` 系列/`.h5` 为 `planned`）；1 秒内返回（SC-004）。

## 4. 应用内验证（生成能力：打开过滤）

File → Open Data File…，观察过滤下拉框。

**预期**（FR-006 / FR-009 / SC-003）：

- 过滤仅含 `SVisual Files (*.plt)` 与 `Curve Data (*.csv)`，外加 `All Files (*)`。
- 不再出现 `.tdr`/`.dat`/`.vtk` 系列/`.h5` 等「规划中」格式（它们保留在查询结果与文档中）。
- 用 `All Files` 打开一个 `.csv` 文件：能正常加载（若 CSV reader 已实现）或得到类型化错误而非崩溃（宪法安全约束）。

## 5. 文档同步门禁（生成能力：文档）

```powershell
python scripts\sync_file_types.py --check
```

**预期**：退出码 0（README「打开文件过滤」与 docs/architecture.md 格式描述与目录一致）；人为改乱文档后重跑返回非零并指出差异位置。

## 验收对照

| spec SC | 验证方式 |
|---|---|
| SC-001 覆盖 4 格式族 | CTest（每族 ≥1 条）+ 第 3 步清单人工比对 |
| SC-002 已支持⇄实际可打开一一对应 | CTest `inconsistencies()` 为空 + 第 3 步 |
| SC-003 过滤列表与目录一致 | 第 4 步 |
| SC-004 查询 ≤1s | 第 3 步 |
| SC-005 单一来源同步 | 第 5 步（--update 后 --check 通过） |
| SC-006 差异可检测 | CTest 差异检测用例 + 第 5 步人为不一致场景 |
