# Quickstart: Panel Layout Settings 验证指南

> Phase 1 输出。端到端验证场景；契约与数据模型见 `contracts/` 与 `data-model.md`，不重复实现细节。

## 前置条件

- 完整构建通过：`scripts/build.ps1`（CMake ≥ 3.16 + Ninja + VS2022，PERCEPTION_BUILD_GUI 开启）
- 单元测试：`ctest`（`tests/cpp/`）+ 新增 `panel_layout_config_test`、`panel_settings_dialog_test`

## 自动验证（回归门禁）

```powershell
# 1. 纯逻辑单测：4 模式 × 8 显隐组合的区域映射 / 合法组合 / expand 决策
ctest -R panel_layout_config_test --output-on-failure

# 2. GUI 交互测试：对话框控件 / configChanged 信号 / OK-Cancel 行为
ctest -R panel_settings_dialog_test --output-on-failure

# 3. 全量回归（宪法质量门禁）
ctest
pytest   # tests/python 保持全 skip，不新增
```

**预期**：全部通过；`panel_layout_config_test` 覆盖 FR-003（区域合法）、FR-008（组合恒合法）、FR-005（expand 决策）断言；`panel_settings_dialog_test` 覆盖 FR-002/FR-004 信号与 spec US3 OK/Cancel。

## 手动端到端验证

### 场景 1：打开 Panel Settings 并切换四种模式（FR-001/FR-002）

1. 启动应用（默认：左 Data、右 Property、底部 PyShell）。
2. 菜单 **视图 → Panel Settings...**，弹出无边框对话框（自定义标题栏可拖拽）。
3. 依次点击四种模式：
   - `DualWithConsole`（默认）：左 Data、右 Property、底部 PyShell。
   - `DualReversedWithConsole`：左 Property、右 Data、底部 PyShell。
   - `DualOnly`：左 Data、右 Property，PyShell 隐藏（中央区向下扩展）。
   - `DualReversedOnly`：左 Property、右 Data，PyShell 隐藏。
4. **预期**：每次点击主窗口布局立即更新；对话框内示意图同步；无重叠、无空白死区。

### 场景 2：显隐与 expand（FR-004/FR-005）

1. 在任意模式下勾选/取消 Data / Property / PyShell 显隐。
2. **预期**：取消勾选后对应面板立即消失，其余可见面板与中央区按比例扩展填满释放空间；恢复勾选后面板回到模式定义区域。全隐 → 中央区占满；左右同隐 → 中央区占满宽度。

### 场景 3：预览与取消（FR-006 / spec US3 场景 3）

1. 打开对话框，随意切换模式/显隐，观察主窗口实时变化。
2. 点击 **Cancel**。
3. **预期**：主窗口恢复到打开对话框前的布局（快照回滚）。
4. 再次切换模式/显隐，点击 **OK** 后关闭，主窗口保留最新布局。

### 场景 4：持久化与恢复默认（FR-007 / SC-005）

1. 选择 `DualReversedOnly`，取消勾选 Data，OK 关闭。
2. 重启应用：**预期** 左 Property、右 Data、底部无 PyShell、Data 隐藏。
3. 菜单 **视图 → Reset Layout**（Ctrl+Shift+L）：**预期** 恢复默认（左 Data、右 Property、底部 PyShell、三面板全显）。
4. 再重启：**预期** 仍为默认布局（reset 已清空 panel settings）。

### 场景 5：与现有功能协同（回归）

- 全屏（视图 → Fullscreen）：面板临时隐藏、退出恢复；与 Panel Settings 显隐状态不冲突。
- 手动拖拽 dock 到任意位置后打开 Panel Settings 切换任一模式：dock 回到模式定义区域。
- 深/浅主题切换后打开对话框：控件与示意图颜色随主题，无花斑。
