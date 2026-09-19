# Data Model: scripts 脚本说明文档

**Branch**: `011-document-scripts` | **Date**: 2026-09-19
**Feature**: [spec.md](./spec.md) | **Research**: [research.md](./research.md)

本特性无业务数据模型（不触及 `src/core` 数据层），仅描述「文档」与「脚本」两类被文档化的对象及其关系。

---

## Entities

### Script（脚本）

被 `scripts/README.md` 文档化的任意脚本（位于 `scripts/` 或 `tests/`）。

| 字段 | 类型 | 说明 | 约束（来源） |
|------|------|------|--------------|
| `name` | string | 文件名（含扩展名） | 唯一；作为文档条目主键 |
| `language` | enum | `ps1` \| `py` | — |
| `purpose` | string | 一句话用途 | FR-002 必填 |
| `parameters` | list<Param> | 参数/开关及其含义 | FR-002 必填 |
| `dependencies` | list<string> | 前置依赖/安装方式 | FR-002/FR-005 必填 |
| `example` | string | 可复制运行示例 | FR-002 必填 |
| `notes` | string | 退出码/关联 spec/产物路径 | FR-002 选填 |
| `category` | enum | 构建与清理 / 代码门禁 / 资源生成 / 测试数据·截图 / 图标·主题校验 / 文档同步 / 测试辅助 | FR-007 归类 |
| `status` | enum | `active` \| `deprecated-candidate` \| `removed` | FR-008 流程 |

**Param 子结构**：`{ flag: string, meaning: string }`

**状态迁移（删除流程 FR-008）**：
`active` →（审计标记为 `deprecated-candidate`）→（用户确认）`removed`（从仓库与文档移除）；未确认则维持 `active`。

---

### Documentation File（说明文档）

单一来源文档 `scripts/README.md`。

| 字段 | 类型 | 说明 |
|------|------|------|
| `path` | string | `scripts/README.md` |
| `entries` | list<Script> | 文档化的全部 Script 投影 |
| `testSection` | bool | 是否含「测试」章节（FR-009） |
| `indexTable` | bool | 是否含快速索引表（FR-007） |

**一致性规则**：`entries` 必须与 `scripts/` + `tests/` 下实际脚本集合一致（FR-006 同步约定）。

---

### Test Helper Script（测试辅助脚本）

`tests/` 下新增的、用于运行/汇总各类型测试的脚本（R2 设计）。是 `Script` 的子类，额外属性：

| 字段 | 类型 | 说明 |
|------|------|------|
| `testType` | enum | `cpp-ctest` \| `python-pytest` |
| `reportOutput` | string? | 报告输出路径（可选） |

候选实例：`tests/run_cpp_tests.py`（`testType=cpp-ctest`）、`tests/run_python_tests.py`（`testType=python-pytest`）。

---

## Relationships

- `Documentation File` 1 — *documents* — * N `Script`（含 `Test Helper Script`）
- `Script.status` 受 FR-008 删除流程驱动；`removed` 后 `Documentation File.entries` 同步移除。

## Validation Rules（来自需求）

- 每个 `Script` 条目必含 `purpose` + `example` + `dependencies`（SC-004）。
- `Documentation File.entries` 覆盖率 = 100%（`scripts/` 保留脚本 + `tests/` 辅助脚本无遗漏，SC-002）。
- 脚本增删改必须同 PR 同步文档（FR-006）。
