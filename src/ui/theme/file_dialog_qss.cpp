// ===== QFileDialog 主题化 QSS 实现 =====
// 008-unify-dialog-styling 跟进：覆盖 QFileDialog 内部区域（侧边栏/面包屑/视图/
// 文件名行/类型下拉/按钮行），使弹窗外观与全局主题一致。
#include "ui/theme/file_dialog_qss.h"

#include "ui/theme/theme_types.h"

namespace perception {
namespace ui {

QString buildFileDialogQss(const theme::ThemeColors& c, const QColor& dialogBg) {
    const auto hx = [](const QColor& col) { return col.name(QColor::HexRgb); };
    return QStringLiteral(R"(
QFileDialog {
    background-color: %1;   /* 弹窗层背景（与全局 QDialog 规则一致） */
    color: %2;
}
QFileDialog QListView, QFileDialog QTreeView {
    background-color: %3;   /* 侧边栏 + 文件视图：BG_PANEL */
    alternate-background-color: %4;
    color: %2;
    border: 1px solid %5;
    outline: none;
}
QFileDialog QListView::item, QFileDialog QTreeView::item {
    padding: 4px 6px;
    border: none;
    border-radius: 4px;
}
QFileDialog QListView::item:hover, QFileDialog QTreeView::item:hover {
    background: %6;
}
QFileDialog QListView::item:selected, QFileDialog QTreeView::item:selected {
    background: %7;
    color: %8;
}
QFileDialog QHeaderView::section {
    background-color: %4;
    color: %9;
    border: none;
    border-right: 1px solid %10;
    border-bottom: 1px solid %10;
    padding: 4px 8px;
}
QFileDialog QToolButton {   /* 面包屑 / 导航 / 视图切换 */
    background: transparent;
    border: none;
    border-radius: 4px;
    padding: 4px 6px;
    color: %2;
}
QFileDialog QToolButton:hover { background: %11; }
QFileDialog QToolButton:pressed { background: %7; color: %8; }
QFileDialog QToolButton:checked { background: %7; color: %8; }
QFileDialog QLineEdit {     /* 文件名 / Look in */
    background-color: %12;
    color: %2;
    border: 1px solid %5;
    border-radius: 4px;
    padding: 4px 6px;
    selection-background-color: %13;
    selection-color: %14;
}
QFileDialog QLineEdit:focus { border-color: %13; }
QFileDialog QComboBox {     /* 文件类型下拉 */
    background-color: %12;
    color: %2;
    border: 1px solid %5;
    border-radius: 4px;
    padding: 4px 6px;
    selection-background-color: %13;
    selection-color: %14;
}
QFileDialog QComboBox::drop-down { border: none; width: 20px; }
QFileDialog QComboBox QAbstractItemView {
    background-color: %12;
    color: %2;
    border: 1px solid %5;
    selection-background-color: %7;
    selection-color: %8;
    outline: none;
}
QFileDialog QPushButton {   /* Open / Cancel */
    background-color: %12;
    color: %2;
    border: 1px solid %5;
    border-radius: 4px;
    padding: 5px 14px;
}
QFileDialog QPushButton:hover { background-color: %15; }
QFileDialog QPushButton:pressed { background-color: %7; color: %8; }
QFileDialog QPushButton:default {   /* Open = accent 主按钮（QSS :default 语义一致） */
    background-color: %13;
    color: %14;
    border-color: %13;
}
QFileDialog QPushButton:default:hover { background-color: %16; }
QDialogButtonBox { background: transparent; }   /* 按钮行底：透出弹窗层 */
)")
        .arg(hx(dialogBg))           // %1 弹窗层背景
        .arg(hx(c.text))             // %2 主文字
        .arg(hx(c.panelBg))          // %3 面板层（侧边栏/视图底）
        .arg(hx(c.surfaceElev))      // %4 抬升面（交替行/表头）
        .arg(hx(c.border))           // %5 边框
        .arg(hx(c.itemHover))        // %6 item hover
        .arg(hx(c.selectionBg))      // %7 选中底
        .arg(hx(c.textOnSelection))  // %8 选中文字
        .arg(hx(c.textWeak))         // %9 次要文字
        .arg(hx(c.borderWeak))       // %10 弱分隔线
        .arg(hx(c.hoverBg))          // %11 通用 hover
        .arg(hx(c.controlBg))        // %12 控件底
        .arg(hx(c.accent))           // %13 accent
        .arg(hx(c.textOnAccent))     // %14 accent 上文字
        .arg(hx(c.buttonHover))      // %15 按钮 hover
        .arg(hx(c.accentHover));     // %16 主按钮 hover
}

}  // namespace ui
}  // namespace perception
