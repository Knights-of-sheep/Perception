# 图标语义识别盲测记录（SC-003）

> **Feature**: 007-replace-icon-set（替换后 v2.0.0 盲测）| **验收标准**: SC-003 —— 无歧义识别 ≥90%，抽样 ≥10 枚 × 3 人
> **协议**: 评审者在无标注状态下独立识别图标语义，正确率 = 正确识别数 / 总测试数。
> **测试材料**: `docs/design/mockups/005-icon-set/blind-test-16px.png`（12 枚 × 16px 无标签拼图，对应 `src/ui/theme/icons/png/actions/<icon_id>-16.png`）。

## 抽样清单（12 枚，覆盖六类）

| # | icon_id | 预期语义（答案） | 类别 |
|---|---|---|---|
| 1 | `file-open` | 打开文件 | file |
| 2 | `file-export-data` | 导出数据 | file |
| 3 | `edit-undo` | 撤销 | edit |
| 4 | `view-zoom-in` | 放大 | view |
| 5 | `view-fit-screen` | 自适应显示全部 | view |
| 6 | `analysis-probe` | 探针取点 | analysis |
| 7 | `analysis-clip` | 裁剪 | analysis |
| 8 | `animation-play` | 播放 | animation |
| 9 | `animation-pause` | 暂停 | animation |
| 10 | `tools-settings` | 设置 | tools |
| 11 | `tools-help` | 帮助 | tools |
| 12 | `tools-measure` | 测量 | tools |

## 执行记录（正式评审阶段填写）

| 评审者 | 正确数 / 12 | 正确率 | 结论（≥90% 通过） |
|---|---|---|---|
| R1 | _/_ | _% | □ 通过 □ 未通过 |
| R2 | _/_ | _% | □ 通过 □ 未通过 |
| R3 | _/_ | _% | □ 通过 □ 未通过 |

**汇总**: 平均正确率 ___% | 是否满足 SC-003: □ 是 □ 否

## 说明

- 抽样 12 枚覆盖全部六大类别（file 2 / edit 1 / view 2 / analysis 2 / animation 2 / tools 3），每类 ≥1 枚，≥10 枚下限。
- 测试时不得展示本文件的答案列。
- 相关评审：五态与极端场景对照 `specs/002-icon-design/contracts/conformance-checklist.md` Section D/F；16px 辨识度对照 SC-004/SC-006（Material Outlined 原生 2px@24 → 16px ≈1.33px，可读性由本轮盲测兜底，契约 `specs/007-replace-icon-set/contracts/icon-source-and-style.md` §5）。
