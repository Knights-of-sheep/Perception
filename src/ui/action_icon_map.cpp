#include "ui/action_icon_map.h"

namespace perception {
namespace ui {

namespace {

// 契约 icon-action-map.md §1~§3 的动作规格（顺序：菜单 → 左栏新增 → 右栏）。
// 未实现功能一律 enabled=false，禁止连接功能槽（FR-011）。
const ActionSpec kActions[] = {
    // ===== 菜单操作项（§1，FR-002）=====
    {"action.openFile",       "Open File…",             "Open data files",                    "file-open",              false, true,  true,  true,  false},
    {"action.exportCommands", "Export Command Script…", "Export current session operations to a command script", "file-export-data", false, false, true, false, false},
    {"action.exportImage",    "Export Main Window Image…", "Export the main window as an image", "file-export-screenshot", false, true, true, true, false},
    {"action.exit",           "Exit",                   "Exit the application",               "file-close",             false, true,  true,  false, false},
    {"action.undo",           "Undo",                   "Undo the last action (coming soon)",  "edit-undo",              false, false, true,  true,  false},
    {"action.redo",           "Redo",                   "Redo the undone action (coming soon)", "edit-redo",            false, false, true,  true,  false},
    {"action.toggleFileDock",      "Data Panel",       "Show/hide the data panel",         "view-panel-data",        true,  true,  true,  true,  false},
    {"action.togglePropertyDock",  "Properties Panel", "Show/hide the properties panel",   "view-panel-property",    true,  true,  true,  true,  false},
    {"action.togglePythonConsole", "Python Console",   "Show/hide the Python console",     "view-panel-console",     true,  true,  true,  true,  false},
    {"action.resetLayout",    "Reset Layout",           "Restore the default layout",        "view-layout-reset",      false, true,  true,  false, false},
    {"action.vtkLog",         "VTK Log Interception",   "Toggle VTK log interception",       "tools-log",              false, true,  true,  false, false},
    {"action.openLogDir",     "Open Log Directory",     "Open the log directory in the file manager", "tools-log-dir",   false, true, true, false, false},
    {"action.setLogPath",     "Set Log Path…",          "Choose where log files are saved",  "tools-log-path",         false, true,  true,  false, false},
    {"action.clearLog",       "Clear Log History",      "Clear the log history",             "edit-delete-selection",  false, true,  true,  false, false},
    {"action.help",           "Help",                   "View help documentation",           "tools-help",             false, true,  true,  false, false},
    {"action.about",          "About",                  "View application information",      "tools-about",            false, true,  true,  false, false},
    // ===== 左侧功能栏新增动作（§2，FR-011 禁用态）=====
    {"action.loadScript",   "Load Script",        "Load a Python script (coming soon)",     "file-load-script",    false, false, false, true,  false},
    {"action.recordScreen", "Record Main Window Video", "Record the main window as a video (coming soon)", "file-record-screen", false, false, false, true, false},
    {"action.refresh",      "Refresh",            "Refresh the current view (coming soon)", "view-refresh",        false, false, false, true,  false},
    // ===== 右侧功能栏动作（§3，FR-011 全部禁用态）=====
    {"action.zoomIn",       "Zoom In",            "Zoom in on the view (coming soon)",      "view-zoom-in",        false, false, false, false, true},
    {"action.zoomOut",      "Zoom Out",           "Zoom out of the view (coming soon)",     "view-zoom-out",       false, false, false, false, true},
    {"action.fitView",      "Fit View",           "Fit all data to the view (coming soon)", "view-fit-screen",     false, false, false, false, true},
    {"action.resetView",    "Reset View",         "Reset the camera view (coming soon)",    "view-reset-camera",   false, false, false, false, true},
    {"action.addCurve",     "Add Curve",          "Add a curve (coming soon)",              "analysis-curve-add",  false, false, false, false, true},
    {"action.removeCurve",  "Remove Curve",       "Remove the selected curve (coming soon)", "analysis-curve-remove", false, false, false, false, true},
    {"action.extractData",  "Extract Data",       "Extract data from the view (coming soon)", "analysis-extract",  false, false, false, false, true},
    {"action.axisSettings", "Axis Settings",      "Configure the axes (coming soon)",       "analysis-axis-settings", false, false, false, false, true},
    {"action.toggleLegend", "Legend",             "Toggle the legend (coming soon)",        "analysis-legend",     false, false, false, false, true},
};

const int kActionCount = sizeof(kActions) / sizeof(kActions[0]);

// 左栏顺序（§2）：撤销/重做/加载文件/加载脚本/截图/录制/刷新/数据/属性/命令窗口
const char* const kLeftOrder[] = {
    "action.undo", "action.redo", "action.openFile", "action.loadScript",
    "action.exportImage", "action.recordScreen", "action.refresh",
    "action.toggleFileDock", "action.togglePropertyDock", "action.togglePythonConsole",
};

// 右栏顺序（§3）：放大/缩小/自适应/重置视图/加曲线/删曲线/提取/坐标轴/图例
const char* const kRightOrder[] = {
    "action.zoomIn", "action.zoomOut", "action.fitView", "action.resetView",
    "action.addCurve", "action.removeCurve", "action.extractData",
    "action.axisSettings", "action.toggleLegend",
};

}  // namespace

const QVector<ActionSpec>& ActionIconMap::all() {
    static const QVector<ActionSpec> table(kActions, kActions + kActionCount);
    return table;
}

const ActionSpec* ActionIconMap::find(const QString& id) {
    for (const auto& a : all()) {
        if (a.id == id) return &a;
    }
    return nullptr;
}

QVector<const ActionSpec*> ActionIconMap::menuActions() {
    QVector<const ActionSpec*> result;
    for (const auto& a : all()) {
        if (a.inMenu) result.append(&a);
    }
    return result;
}

QStringList ActionIconMap::leftToolBarOrder() {
    QStringList ids;
    for (const char* id : kLeftOrder) ids << QString::fromLatin1(id);
    return ids;
}

QStringList ActionIconMap::rightToolBarOrder() {
    QStringList ids;
    for (const char* id : kRightOrder) ids << QString::fromLatin1(id);
    return ids;
}

}  // namespace ui
}  // namespace perception
