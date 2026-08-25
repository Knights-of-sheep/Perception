# 图标语义识别盲测记录（SC-003）

> **Feature**: 002-icon-design | **验收标准**: SC-003 —— 无歧义识别 ≥90%，抽样 ≥10 枚 × 3 人
> **协议**: 评审者在无标注状态下独立识别图标语义，正确率 = 正确识别数 / 总测试数。

## 抽样清单（10 枚，覆盖六类）

| # | icon_id | 预期语义（答案） | 类别 |
|---|---|---|---|
| 1 | `file-open` | 打开文件 | file |
| 2 | `file-export-data` | 导出数据 | file |
| 3 | `edit-undo` | 撤销 | edit |
| 4 | `view-zoom-in` | 放大 | view |
| 5 | `view-display-3d` | 三维显示 | view |
| 6 | `analysis-probe` | 探针取点 | analysis |
| 7 | `analysis-contour` | 等值线 | analysis |
| 8 | `animation-play` | 播放 | animation |
| 9 | `animation-step-backward` | 步进后退 | animation |
| 10 | `tools-measure` | 测量 | tools |

## 执行记录（正式评审阶段填写）

| 评审者 | 正确数 / 10 | 正确率 | 结论（≥90% 通过） |
|---|---|---|---|
| R1 | _/_ | _% | □ 通过 □ 未通过 |
| R2 | _/_ | _% | □ 通过 □ 未通过 |
| R3 | _/_ | _% | □ 通过 □ 未通过 |

**汇总**: 平均正确率 ___% | 是否满足 SC-003: □ 是 □ 否

## 说明

- 抽样 10 枚覆盖全部六大类别（file/edit/view/analysis/animation/tools），每类 ≥1 枚。
- 测试材料：`src/ui/theme/icons/png/actions/<icon_id>-16.png`（16px，模拟真实 UI 尺寸）。
- 测试时不得展示本文件的答案列。
- 相关评审：五态与极端场景对照 `specs/002-icon-design/contracts/conformance-checklist.md` Section D/F；16px 辨识度对照 SC-004/SC-006。
