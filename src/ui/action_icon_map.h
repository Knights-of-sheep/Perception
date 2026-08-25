// ===== ActionIconMap：动作-图标映射（契约唯一代码实现）=====
// 对应 specs/003-install-icon-bars/contracts/icon-action-map.md §1~§3，
// 与 tests/cpp/icon_action_map_test.cpp 同源（V-1~V-8）。
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace perception {
namespace ui {

// 单个动作的契约规格（对应 data-model UiAction）
struct ActionSpec {
    QString id;         // 唯一标识（kebab-case），如 action.openFile
    QString text;       // 显示文字（菜单项 / 功能栏 tooltip 基准）
    QString tooltip;    // 悬停提示（FR-006，必须非空）
    QString iconId;     // icon-map.yaml 图标 id（FR-002/003/005）
    bool checkable = false;  // 面板显隐开关 = true
    bool enabled = true;     // 初始可用状态（未实现功能 = false，FR-011）
    bool inMenu = false;     // 属于菜单
    bool inLeftBar = false;  // 属于左侧功能栏
    bool inRightBar = false; // 属于右侧功能栏
};

class ActionIconMap {
public:
    static const QVector<ActionSpec>& all();          // 全部动作（契约全表）
    static const ActionSpec* find(const QString& id); // 按 id 查找，未命中返回 nullptr
    static QVector<const ActionSpec*> menuActions();  // 菜单操作项（FR-002 全覆盖）
    static QStringList leftToolBarOrder();            // 左栏顺序（FR-003，10 项固定）
    static QStringList rightToolBarOrder();           // 右栏顺序（FR-005，9 项固定）
};

}  // namespace ui
}  // namespace perception
