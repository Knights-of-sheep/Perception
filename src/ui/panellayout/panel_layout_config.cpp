// ===== 面板布局配置实现（010-panel-layout-settings）=====
// 见 panel_layout_config.h 的契约说明（contracts/panel-layout-config-api.md §1/§2）。
#include "ui/panellayout/panel_layout_config.h"

namespace perception {
namespace ui {

DockArea targetArea(PanelLayoutMode mode, PanelId id) {
    switch (id) {
    case PanelId::Data:
        return (mode == PanelLayoutMode::DualReversedOnly ||
                mode == PanelLayoutMode::DualReversedWithConsole)
                   ? DockArea::Right
                   : DockArea::Left;
    case PanelId::Property:
        return (mode == PanelLayoutMode::DualReversedOnly ||
                mode == PanelLayoutMode::DualReversedWithConsole)
                   ? DockArea::Left
                   : DockArea::Right;
    case PanelId::PyShell:
        return DockArea::Bottom;
    }
    return DockArea::Left;
}

bool isLegalArea(PanelId id, DockArea area) {
    switch (id) {
    case PanelId::Data:
    case PanelId::Property:
        return area == DockArea::Left || area == DockArea::Right;
    case PanelId::PyShell:
        return area == DockArea::Bottom;
    }
    return false;
}

bool modeDefaultsConsoleVisible(PanelLayoutMode) {
    // 四种模式均含 PyShell（用户示意图：非全尺寸 *Only / 全尺寸 *WithConsole），
    // 模式预设下 PyShell 默认显示，仅尺寸形态不同（modeHasFullWidthConsole）
    return true;
}

bool modeHasFullWidthConsole(PanelLayoutMode mode) {
    return mode == PanelLayoutMode::DualWithConsole ||
           mode == PanelLayoutMode::DualReversedWithConsole;
}

bool isPanelVisible(const PanelLayoutConfig& cfg, PanelId id) {
    switch (id) {
    case PanelId::Data:
        return cfg.dataVisible;
    case PanelId::Property:
        return cfg.propertyVisible;
    case PanelId::PyShell:
        return cfg.consoleVisible;
    }
    return false;
}

bool isLegalConfig(const PanelLayoutConfig& cfg) {
    // FR-008：模式为预设枚举，左右各一（Data/Property 区域互异且合法）、
    // PyShell 恒落 Bottom（合法），显隐维度不产生非法布局。
    return targetArea(cfg.mode, PanelId::Data) != targetArea(cfg.mode, PanelId::Property) &&
           isLegalArea(PanelId::Data, targetArea(cfg.mode, PanelId::Data)) &&
           isLegalArea(PanelId::Property, targetArea(cfg.mode, PanelId::Property)) &&
           isLegalArea(PanelId::PyShell, DockArea::Bottom);
}

QString modeToKey(PanelLayoutMode mode) {
    switch (mode) {
    case PanelLayoutMode::DualOnly:
        return QStringLiteral("DualOnly");
    case PanelLayoutMode::DualWithConsole:
        return QStringLiteral("DualWithConsole");
    case PanelLayoutMode::DualReversedOnly:
        return QStringLiteral("DualReversedOnly");
    case PanelLayoutMode::DualReversedWithConsole:
        return QStringLiteral("DualReversedWithConsole");
    }
    return QStringLiteral("DualWithConsole");
}

PanelLayoutMode modeFromKey(const QString& key, PanelLayoutMode fallback) {
    if (key == QLatin1String("DualOnly")) return PanelLayoutMode::DualOnly;
    if (key == QLatin1String("DualWithConsole")) return PanelLayoutMode::DualWithConsole;
    if (key == QLatin1String("DualReversedOnly")) return PanelLayoutMode::DualReversedOnly;
    if (key == QLatin1String("DualReversedWithConsole"))
        return PanelLayoutMode::DualReversedWithConsole;
    return fallback;
}

}  // namespace ui
}  // namespace perception
