#include "ui/action_icon_map.h"

namespace perception {
namespace ui {

namespace {

// 契约 icon-action-map.md §1~§3 的动作规格（顺序：菜单 → 左栏新增 → 右栏）。
// 未实现功能一律 enabled=false，禁止连接功能槽（FR-011）。
const ActionSpec kActions[] = {
    // ===== 菜单操作项（§1，FR-002）=====
    {"action.openFile",       "打开文件…",       "打开数据文件",                   "file-open",              false, true,  true,  true,  false},
    {"action.exportCommands", "导出命令脚本…",   "将当前会话操作导出为命令脚本",   "file-export-data",       false, false, true,  false, false},
    {"action.exportImage",    "导出主界面图片…", "将主界面导出为图片",             "file-export-screenshot", false, true,  true,  true,  false},
    {"action.exit",           "退出",            "退出程序",                       "file-close",             false, true,  true,  false, false},
    {"action.undo",           "撤销",            "撤销上一步操作（功能即将推出）", "edit-undo",              false, false, true,  true,  false},
    {"action.redo",           "重做",            "重做被撤销的操作（功能即将推出）", "edit-redo",            false, false, true,  true,  false},
    {"action.toggleFileDock",      "数据面板",       "显示/隐藏数据面板",        "view-panel-toggle",      true,  true,  true,  true,  false},
    {"action.togglePropertyDock",  "属性面板",       "显示/隐藏属性面板",        "view-panel-toggle",      true,  true,  true,  true,  false},
    {"action.togglePythonConsole", "Python 控制台",  "显示/隐藏 Python 控制台",  "view-panel-toggle",      true,  true,  true,  true,  false},
    {"action.resetLayout",    "重置布局",        "恢复默认布局",                   "view-reset-camera",      false, true,  true,  false, false},
    {"action.vtkLog",         "VTK 日志拦截",    "开关 VTK 日志拦截",              "tools-settings",         false, true,  true,  false, false},
    {"action.openLogDir",     "打开日志目录",    "在文件管理器中打开日志目录",     "tools-settings",         false, true,  true,  false, false},
    {"action.setLogPath",     "设置日志路径…",   "选择日志文件保存位置",           "tools-settings",         false, true,  true,  false, false},
    {"action.clearLog",       "清除历史日志",    "清空历史日志",                   "edit-delete-selection",  false, true,  true,  false, false},
    {"action.help",           "帮助",            "查看帮助文档",                   "tools-help",             false, true,  true,  false, false},
    {"action.about",          "关于",            "查看程序信息",                   "tools-about",            false, true,  true,  false, false},
    // ===== 左侧功能栏新增动作（§2，FR-011 禁用态）=====
    {"action.loadScript",   "加载脚本",       "加载 Python 脚本（功能即将推出）",      "file-load-script",    false, false, false, true,  false},
    {"action.recordScreen", "主界面视频录制", "录制主界面为视频（功能即将推出）",      "file-record-screen",  false, false, false, true,  false},
    {"action.refresh",      "刷新",           "刷新当前视图（功能即将推出）",          "view-refresh",        false, false, false, true,  false},
    // ===== 右侧功能栏动作（§3，FR-011 全部禁用态）=====
    {"action.zoomIn",       "放大",           "放大视图（功能即将推出）",              "view-zoom-in",        false, false, false, false, true},
    {"action.zoomOut",      "缩小",           "缩小视图（功能即将推出）",              "view-zoom-out",       false, false, false, false, true},
    {"action.fitView",      "自适应显示",     "自适应显示全部数据（功能即将推出）",    "view-fit-screen",     false, false, false, false, true},
    {"action.resetView",    "重置视图",       "重置相机视角（功能即将推出）",          "view-reset-camera",   false, false, false, false, true},
    {"action.addCurve",     "添加曲线",       "添加一条曲线（功能即将推出）",          "analysis-curve-add",  false, false, false, false, true},
    {"action.removeCurve",  "移除曲线",       "移除选中曲线（功能即将推出）",          "analysis-curve-remove", false, false, false, false, true},
    {"action.extractData",  "数据提取",       "从视图提取数据（功能即将推出）",        "analysis-extract",    false, false, false, false, true},
    {"action.axisSettings", "坐标轴设置",     "设置坐标轴（功能即将推出）",            "analysis-axis-settings", false, false, false, false, true},
    {"action.toggleLegend", "图例",           "开关图例显示（功能即将推出）",          "analysis-legend",     false, false, false, false, true},
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
