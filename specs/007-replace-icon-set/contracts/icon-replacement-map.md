# Contract: 图标替换映射表（icon_id → Material Icons）

**Feature**: 007-replace-icon-set | **Version**: 1.0.0 | **Date**: 2026-08-29 | **Source**: research.md 决策 1/4

> 本契约是**图标替换的唯一可检查定义**。替换后 `actions/<icon_id>.svg` 的图形必须与下表「Material 图标名」一致（形状可辨识对应）；任一不满足即计为不符合项。
>
> - 素材源：Google Material Icons（Figma 官方社区版，Apache-2.0），默认 **Outlined** 风格；标注 `filled` 者使用实心变体。
> - 替换范围：58 枚（`view-panel-console` 保留，不在表内）。
> - 唯一性约束：Material 图标名全表唯一，禁止两枚 icon 共用同一图标（N-02）。

## 映射表

### file（9 枚）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `file-open` | `folder_open` | outlined | 打开文件 → 文件夹开启 |
| `file-save` | `save` | outlined | 保存 |
| `file-export-screenshot` | `photo_camera` | outlined | 导出截图 → 相机（评审两轮调换定稿：screenshot 手机形 → crop_landscape 矩形框 → photo_camera） |
| `file-export-data` | `download` | outlined | 导出数据 → 下载 |
| `file-save-session` | `bookmark` | outlined | 保存会话 → 书签 |
| `file-close` | `close` | outlined | 关闭 |
| `file-remove` | `folder_delete` | outlined | 移除文件 → 删除文件夹 |
| `file-load-script` | `code` | outlined | 加载脚本 → 代码 |
| `file-record-screen` | `videocam` | outlined | 视频录制 → 摄像机 |

### edit（3 枚）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `edit-undo` | `undo` | outlined | 撤销 |
| `edit-redo` | `redo` | outlined | 重做 |
| `edit-delete-selection` | `delete` | outlined | 删除所选 |

### view（18 枚，不含保留的 view-panel-console）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `view-rotate` | `3d_rotation` | outlined | 旋转视图 |
| `view-pan` | `open_with` | outlined | 平移 → 四向箭头 |
| `view-zoom-in` | `zoom_in` | outlined | 放大 |
| `view-zoom-out` | `zoom_out` | outlined | 缩小 |
| `view-zoom-box` | `zoom_in_map` | outlined | 框选缩放 → 放大至区域 |
| `view-fit-screen` | `fit_screen` | outlined | 自适应显示全部 |
| `view-reset-camera` | `center_focus_strong` | outlined | 重置视图 → 中心聚焦 |
| `view-x` | `swap_horiz` | outlined | 沿 X 轴正视 → 水平轴 |
| `view-y` | `swap_vert` | outlined | 沿 Y 轴正视 → 垂直轴 |
| `view-z` | `rotate_90_degrees_cw` | outlined | 沿 Z 轴正视 → 深度旋转 |
| `view-display-2d` | `grid_view` | outlined | 二维显示 → 平面网格 |
| `view-display-3d` | `view_in_ar` | outlined | 三维显示 → 立体物 |
| `view-layer-visibility` | `layers` | outlined | 图层可见性 → 多层 |
| `view-multi-view` | `view_module` | outlined | 多视图 → 模块网格 |
| `view-refresh` | `refresh` | outlined | 刷新 |
| `view-panel-toggle` | `view_sidebar` | outlined | 面板显隐 → 侧栏开关 |
| `view-panel-data` | `list_alt` | outlined | 数据面板 → 数据列表 |
| `view-panel-property` | `edit_attributes` | outlined | 属性面板 → 属性编辑 |

### analysis（16 枚）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `analysis-select-point` | `ads_click` | outlined | 点选 → 瞄准点击 |
| `analysis-select-cell` | `select_all` | outlined | 单元选择 → 全选网格 |
| `analysis-select-region` | `highlight_alt` | outlined | 区域选择 → 框选高亮 |
| `analysis-cutline` | `straighten` | outlined | 截线 → 直线尺 |
| `analysis-probe` | `my_location` | outlined | 探针取点 → 定位点 |
| `analysis-annotate` | `edit_note` | outlined | 标注 → 批注 |
| `analysis-extract` | `output` | outlined | 数据提取 → 输出 |
| `analysis-clip` | `content_cut` | outlined | 裁剪 → 剪刀 |
| `analysis-slice` | `crop` | outlined | 切片 → 裁剪平面 |
| `analysis-contour` | `terrain` | outlined | 等值线 → 等高线地形 |
| `analysis-threshold` | `filter_alt` | outlined | 阈值 → 过滤 |
| `analysis-warp` | `transform` | outlined | 变形 → 变换 |
| `analysis-curve-add` | `add_chart` | outlined | 添加曲线 → 图表加号 |
| `analysis-curve-remove` | `remove` | outlined | 移除曲线 → 减号 |
| `analysis-axis-settings` | `linear_scale` | outlined | 坐标轴设置 → 线性刻度 |
| `analysis-legend` | `legend_toggle` | outlined | 图例 → 图例开关 |

### animation（7 枚）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `animation-play` | `play_arrow` | outlined | 播放 |
| `animation-pause` | `pause` | outlined | 暂停 |
| `animation-first-frame` | `first_page` | outlined | 首帧 → 首页 |
| `animation-last-frame` | `last_page` | outlined | 末帧 → 末页 |
| `animation-step-forward` | `skip_next` | outlined | 步进前进 → 下一段 |
| `animation-step-backward` | `skip_previous` | outlined | 步进后退 → 上一段 |
| `animation-param-scan` | `tune` | outlined | 参数扫描 → 调参滑块 |

### tools（5 枚）

| icon_id | Material 图标名 | 风格 | 备注 |
|---|---|---|---|
| `tools-settings` | `settings` | outlined | 设置 |
| `tools-colormap` | `palette` | outlined | 配色方案 → 调色板 |
| `tools-measure` | `square_foot` | outlined | 测量 → 量尺 |
| `tools-help` | `help_outline` | outlined | 帮助 |
| `tools-about` | `info` | outlined | 关于 |

## 覆盖与验收规则

1. **一一对应**：替换后 59 枚 `actions/*.svg` 与 `icon-map.yaml` 59 条目一一对应（覆盖规则 1），`check_icons.py` 通过。
2. **映射一致**：58 枚 replace 图标的 SVG 图形必须与上表 Material 图标名对应；保留的 `view-panel-console` 不在表内。
3. **唯一性**：上表 Material 图标名 58 项全表唯一。
4. **门禁**：所有 SVG 通过 `scripts/check_icons.py`（P-01 色板 / N-01 命名 / 覆盖 / schema）退出码 0。
5. **盲测**：语义盲测抽样 ≥10 枚（覆盖六类，从映射表中抽取），3 名评审者正确率 ≥90%（SC-003）。
