// 契约单测：specs/003-install-icon-bars/contracts/icon-action-map.md（V-1~V-8）。
// 与 src/ui/action_icon_map.cpp 同源；资源存在性断言依赖 theme.qrc（链接 perception_ui）。
#include "ui/action_icon_map.h"

#include <QCoreApplication>
#include <QFile>
#include <QStringList>

#include <cstdio>

using namespace perception::ui;

namespace {

int g_fail = 0;

void check(bool cond, const char* expr, const char* file, int line) {
    if (!cond) {
        std::printf("FAIL %s:%d  %s\n", file, line, expr);
        ++g_fail;
    }
}

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

void checkSizes(const QString& iconId) {
    const int kSizes[] = {16, 24, 32};
    for (int s : kSizes) {
        const QString p =
            QStringLiteral(":/perception/icons/icons/png/actions/%1-%2.png").arg(iconId).arg(s);
        CHECK(QFile::exists(p));  // V-5：PNG 三档资源存在
    }
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // 静态库中的 AUTORCC 资源符号需显式引用才会被链接（MSVC /OPT:REF）
    Q_INIT_RESOURCE(theme);

    // ---- V-1：左侧功能栏集合 = 10 个固定 id，顺序固定（FR-003）----
    const QStringList kLeftExpected = {
        "action.undo", "action.redo", "action.openFile", "action.loadScript",
        "action.exportImage", "action.recordScreen", "action.refresh",
        "action.toggleFileDock", "action.togglePropertyDock", "action.togglePythonConsole",
    };
    CHECK(ActionIconMap::leftToolBarOrder() == kLeftExpected);

    // ---- V-2：右侧功能栏集合 = 9 个固定 id，顺序固定（FR-005）----
    const QStringList kRightExpected = {
        "action.zoomIn", "action.zoomOut", "action.fitView", "action.resetView",
        "action.addCurve", "action.removeCurve", "action.extractData",
        "action.axisSettings", "action.toggleLegend",
    };
    CHECK(ActionIconMap::rightToolBarOrder() == kRightExpected);

    // ---- V-3：每个菜单操作项 iconId 非空（FR-002）----
    const auto menu = ActionIconMap::menuActions();
    CHECK(menu.size() == 16);  // 契约 §1 全部操作项（不含动态主题/日志级别列表）
    for (const auto* a : menu) {
        CHECK(!a->iconId.isEmpty());
    }

    // ---- V-4：每个动作 tooltip 非空（FR-006）----
    for (const auto& a : ActionIconMap::all()) {
        CHECK(!a.tooltip.isEmpty());
    }

    // ---- V-5：每个动作 iconId 对应 PNG（16/24/32）资源存在（FR-008）----
    for (const auto& a : ActionIconMap::all()) {
        checkSizes(a.iconId);
    }

    // ---- V-6：未实现动作 enabled=false（FR-011；无槽连接由 UI 集成验证）----
    const QStringList kDisabledIds = {
        "action.exportCommands", "action.undo", "action.redo",
        "action.loadScript", "action.recordScreen", "action.refresh",
        "action.zoomIn", "action.zoomOut", "action.fitView", "action.resetView",
        "action.addCurve", "action.removeCurve", "action.extractData",
        "action.axisSettings", "action.toggleLegend",
    };
    for (const auto& id : kDisabledIds) {
        const ActionSpec* a = ActionIconMap::find(id);
        CHECK(a != nullptr);
        if (a) CHECK(!a->enabled);
    }

    // ---- V-7：三个面板开关 checkable=true（FR-003）----
    const QStringList kToggleIds = {
        "action.toggleFileDock", "action.togglePropertyDock", "action.togglePythonConsole",
    };
    for (const auto& id : kToggleIds) {
        const ActionSpec* a = ActionIconMap::find(id);
        CHECK(a != nullptr);
        if (a) CHECK(a->checkable);
    }

    // ---- V-8：左栏可用按钮与菜单同动作（FR-004）----
    const QStringList kEnabledLeft = {
        "action.openFile", "action.exportImage",
        "action.toggleFileDock", "action.togglePropertyDock", "action.togglePythonConsole",
    };
    for (const auto& id : kEnabledLeft) {
        const ActionSpec* a = ActionIconMap::find(id);
        CHECK(a != nullptr);
        if (a) CHECK(a->inMenu && a->inLeftBar && a->enabled);
    }

    if (g_fail > 0) {
        std::printf("icon_action_map_test: %d CHECK(S) FAILED\n", g_fail);
        return 1;
    }
    std::printf("icon_action_map_test: ALL PASS\n");
    return 0;
}
