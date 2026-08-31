// ===== 面板布局配置（010-panel-layout-settings）=====
// 纯 C++ 逻辑层（仅依赖 QtCore 类型），是所有面板布局的单一事实源：
//   - 模式 → 面板目标区域映射（FR-003）
//   - 区域合法性判定（FR-003/FR-008）
//   - 模式对应底部 PyShell 默认显隐（FR-002）
//   - expand 决策——给定配置下面板可见性（FR-005）
//   - 任意「模式 × 显隐」组合合法判定（FR-008）
//   - 与 QSettings 四 key 的序列化转换（FR-007，契约 §1）
// 设计：布局语义与 UI 分离，使 tests/cpp 无窗口环境可测（宪法 II Test-First）。
#pragma once

#include <QString>

namespace perception {
namespace ui {

// 三个可布局面板（data-model §1：与现有 dock objectName 对应：
// fileDock / propertyDock / pythonConsoleDock）
enum class PanelId { Data, Property, PyShell };

// 停靠区域（FR-003：左右各至多一个面板，底部仅 PyShell）
enum class DockArea { Left, Right, Bottom };

// 四种预设布局模式（FR-002，data-model §1）：
// 左右分配（Data↔Property 互换） × PyShell 尺寸形态（非全尺寸/全尺寸）。
// 四种模式均包含 PyShell，仅形态不同（用户示意图 2026-08-31）：
//   - 非全尺寸（*Only）：PyShell 窄条嵌入 Plot 下方（只占中央宽度），两侧面板保持全高
//   - 全尺寸（*WithConsole）：PyShell 横跨底部全宽，两侧面板在其上方
enum class PanelLayoutMode {
  DualOnly,                 // 左=Data 右=Property，PyShell 非全尺寸（嵌入 Plot 下方窄条）
  DualWithConsole,          // 左=Data 右=Property，PyShell 全尺寸（底部全宽，默认）
  DualReversedOnly,         // 左=Property 右=Data，PyShell 非全尺寸（嵌入 Plot 下方窄条）
  DualReversedWithConsole,  // 左=Property 右=Data，PyShell 全尺寸（底部全宽）
};

// 面板布局配置（单一事实源，data-model §1）
struct PanelLayoutConfig {
  PanelLayoutMode mode = PanelLayoutMode::DualWithConsole;
  bool dataVisible = true;      // FR-004 Data 面板显隐
  bool propertyVisible = true;  // FR-004 Property 面板显隐
  bool consoleVisible = true;   // FR-004 PyShell 面板显隐

  // 全字段相等（对话框 configChanged 去重 / 测试断言用）
  bool operator==(const PanelLayoutConfig& other) const {
    return mode == other.mode && dataVisible == other.dataVisible &&
           propertyVisible == other.propertyVisible && consoleVisible == other.consoleVisible;
  }
};

// FR-003：面板在指定模式下的目标区域（Data/Property 左右互换，PyShell 恒 Bottom）
DockArea targetArea(PanelLayoutMode mode, PanelId id);

// FR-003/FR-008：区域合法性（Data/Property → Left|Right；PyShell → Bottom）
bool isLegalArea(PanelId id, DockArea area);

// FR-002：模式对应的底部 PyShell 默认显隐（四种模式均含 PyShell，默认显示）
bool modeDefaultsConsoleVisible(PanelLayoutMode mode);

// FR-002：模式是否采用全尺寸（底部全宽）PyShell。
// true  = *WithConsole：PyShell 全宽 dock（两侧面板在其上方）
// false = *Only：PyShell 非全尺寸（嵌入 Plot 下方窄条，两侧面板保持全高）
bool modeHasFullWidthConsole(PanelLayoutMode mode);

// FR-005：expand 决策——面板在目标显隐下的可见性（依据 cfg 与区域约束）
bool isPanelVisible(const PanelLayoutConfig& cfg, PanelId id);

// FR-008：任何「模式 × 显隐」组合恒合法（模式预设保证区域合法）
bool isLegalConfig(const PanelLayoutConfig& cfg);

// FR-007：PanelLayoutMode 与 QSettings key（panelSettings/mode）的双向转换
QString modeToKey(PanelLayoutMode mode);
PanelLayoutMode modeFromKey(const QString& key, PanelLayoutMode fallback);

// FR-007：面板布局持久化 QSettings key（data-model §5 四 key；对话框/MainWindow 共用
// 的单一事实源；MainWindow.cpp 经此引用，不再各自定义字符串）
inline constexpr const char* kPanelSettingsModeKey     = "panelSettings/mode";
inline constexpr const char* kPanelSettingsDataKey     = "panelSettings/dataVisible";
inline constexpr const char* kPanelSettingsPropertyKey = "panelSettings/propertyVisible";
inline constexpr const char* kPanelSettingsConsoleKey  = "panelSettings/consoleVisible";

}  // namespace ui
}  // namespace perception
