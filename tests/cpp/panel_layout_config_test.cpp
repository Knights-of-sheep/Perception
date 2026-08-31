// ===== 面板布局配置纯逻辑单测（010-panel-layout-settings）=====
// 覆盖 FR-003（模式→区域映射 / 区域合法性）、FR-008（组合恒合法）、
// FR-005（expand 决策）、FR-002（模式默认显隐）、FR-007（序列化）与
// 默认值（contracts/panel-layout-config-api.md §1/§2；data-model §3/§6）。
// 仅依赖 QtCore 类型（QString），无 GUI 平台依赖，红-绿 TDD。
#include "ui/panellayout/panel_layout_config.h"

#include <cassert>
#include <cstdio>

using perception::ui::DockArea;
using perception::ui::PanelId;
using perception::ui::PanelLayoutConfig;
using perception::ui::PanelLayoutMode;
using perception::ui::isLegalArea;
using perception::ui::isLegalConfig;
using perception::ui::isPanelVisible;
using perception::ui::modeDefaultsConsoleVisible;
using perception::ui::modeFromKey;
using perception::ui::modeHasFullWidthConsole;
using perception::ui::modeToKey;
using perception::ui::targetArea;

namespace {

void expectArea(DockArea expected, PanelLayoutMode mode, PanelId id) {
    const DockArea got = targetArea(mode, id);
    if (got != expected) {
        fprintf(stderr, "targetArea(mode=%d, id=%d) expected %d got %d\n",
                static_cast<int>(mode), static_cast<int>(id),
                static_cast<int>(expected), static_cast<int>(got));
        assert(false);
    }
}

void expectLegal(bool expected, PanelId id, DockArea area) {
    const bool got = isLegalArea(id, area);
    if (got != expected) {
        fprintf(stderr, "isLegalArea(id=%d, area=%d) expected %d got %d\n",
                static_cast<int>(id), static_cast<int>(area), expected ? 1 : 0, got ? 1 : 0);
        assert(false);
    }
}

void expectConsoleDefault(bool expected, PanelLayoutMode mode) {
    const bool got = modeDefaultsConsoleVisible(mode);
    if (got != expected) {
        fprintf(stderr, "modeDefaultsConsoleVisible(mode=%d) expected %d got %d\n",
                static_cast<int>(mode), expected ? 1 : 0, got ? 1 : 0);
        assert(false);
    }
}

void expectVisible(bool expected, const PanelLayoutConfig& cfg, PanelId id) {
    const bool got = isPanelVisible(cfg, id);
    if (got != expected) {
        fprintf(stderr, "isPanelVisible(id=%d) expected %d got %d\n",
                static_cast<int>(id), expected ? 1 : 0, got ? 1 : 0);
        assert(false);
    }
}

}  // namespace

int main() {
    // ================= 默认值（data-model §1：DualWithConsole + 三面板全显） =================
    {
        const PanelLayoutConfig cfg;
        if (cfg.mode != PanelLayoutMode::DualWithConsole) assert(false);
        if (!cfg.dataVisible || !cfg.propertyVisible || !cfg.consoleVisible) assert(false);
    }

    // ================= FR-003：模式 → 区域映射 =================
    {
        // 非反向：左=Data 右=Property
        expectArea(DockArea::Left, PanelLayoutMode::DualOnly, PanelId::Data);
        expectArea(DockArea::Right, PanelLayoutMode::DualOnly, PanelId::Property);
        expectArea(DockArea::Left, PanelLayoutMode::DualWithConsole, PanelId::Data);
        expectArea(DockArea::Right, PanelLayoutMode::DualWithConsole, PanelId::Property);
        // 反向：左=Property 右=Data
        expectArea(DockArea::Right, PanelLayoutMode::DualReversedOnly, PanelId::Data);
        expectArea(DockArea::Left, PanelLayoutMode::DualReversedOnly, PanelId::Property);
        expectArea(DockArea::Right, PanelLayoutMode::DualReversedWithConsole, PanelId::Data);
        expectArea(DockArea::Left, PanelLayoutMode::DualReversedWithConsole, PanelId::Property);
        // PyShell 恒 Bottom（四种模式）
        for (int m = 0; m < 4; ++m) {
            expectArea(DockArea::Bottom, static_cast<PanelLayoutMode>(m), PanelId::PyShell);
        }
    }

    // ================= FR-003/FR-008：区域合法性 =================
    {
        // Data/Property ∈ {Left, Right}
        expectLegal(true, PanelId::Data, DockArea::Left);
        expectLegal(true, PanelId::Data, DockArea::Right);
        expectLegal(true, PanelId::Property, DockArea::Left);
        expectLegal(true, PanelId::Property, DockArea::Right);
        // Data/Property ∉ {Bottom}
        expectLegal(false, PanelId::Data, DockArea::Bottom);
        expectLegal(false, PanelId::Property, DockArea::Bottom);
        // PyShell ∈ {Bottom}，∉ {Left, Right}
        expectLegal(true, PanelId::PyShell, DockArea::Bottom);
        expectLegal(false, PanelId::PyShell, DockArea::Left);
        expectLegal(false, PanelId::PyShell, DockArea::Right);
    }

    // ================= FR-002：模式对应底部 PyShell 默认显隐 =================
    // 四种模式均含 PyShell（用户示意图 2026-08-31：*Only 非全尺寸 / *WithConsole 全尺寸），
    // 模式预设下 PyShell 默认显示，仅尺寸形态不同
    {
        for (int m = 0; m < 4; ++m) {
            expectConsoleDefault(true, static_cast<PanelLayoutMode>(m));
        }
        // PyShell 尺寸形态：*WithConsole 全尺寸（底部全宽 dock）；*Only 非全尺寸
        //（嵌入 Plot 下方窄条，两侧面板保持全高）
        if (modeHasFullWidthConsole(PanelLayoutMode::DualOnly)) assert(false);
        if (!modeHasFullWidthConsole(PanelLayoutMode::DualWithConsole)) assert(false);
        if (modeHasFullWidthConsole(PanelLayoutMode::DualReversedOnly)) assert(false);
        if (!modeHasFullWidthConsole(PanelLayoutMode::DualReversedWithConsole)) assert(false);
    }

    // ================= FR-005：expand 决策（isPanelVisible） =================
    {
        PanelLayoutConfig cfg;  // 默认全显
        expectVisible(true, cfg, PanelId::Data);
        expectVisible(true, cfg, PanelId::Property);
        expectVisible(true, cfg, PanelId::PyShell);

        cfg.dataVisible = false;
        expectVisible(false, cfg, PanelId::Data);
        expectVisible(true, cfg, PanelId::Property);
        expectVisible(true, cfg, PanelId::PyShell);

        cfg.propertyVisible = false;
        expectVisible(false, cfg, PanelId::Property);
        expectVisible(true, cfg, PanelId::PyShell);

        cfg.consoleVisible = false;
        expectVisible(false, cfg, PanelId::PyShell);
    }

    // ================= FR-008：组合恒合法（4 模式 × 8 显隐组合） =================
    {
        for (int m = 0; m < 4; ++m) {
            for (int d = 0; d < 2; ++d) {
                for (int p = 0; p < 2; ++p) {
                    for (int c = 0; c < 2; ++c) {
                        PanelLayoutConfig cfg;
                        cfg.mode = static_cast<PanelLayoutMode>(m);
                        cfg.dataVisible = (d != 0);
                        cfg.propertyVisible = (p != 0);
                        cfg.consoleVisible = (c != 0);
                        if (!isLegalConfig(cfg)) {
                            fprintf(stderr,
                                    "isLegalConfig(mode=%d d=%d p=%d c=%d) unexpectedly false\n",
                                    m, d, p, c);
                            assert(false);
                        }
                    }
                }
            }
        }
    }

    // ================= FR-007：序列化 round-trip + 未知 key 回退 =================
    {
        const PanelLayoutMode modes[] = {PanelLayoutMode::DualOnly,
                                         PanelLayoutMode::DualWithConsole,
                                         PanelLayoutMode::DualReversedOnly,
                                         PanelLayoutMode::DualReversedWithConsole};
        for (const PanelLayoutMode m : modes) {
            const QString key = modeToKey(m);
            if (modeFromKey(key, PanelLayoutMode::DualWithConsole) != m) {
                fprintf(stderr, "modeFromKey(modeToKey(%d)) != %d\n",
                        static_cast<int>(m), static_cast<int>(m));
                assert(false);
            }
        }
        // 精确 key 字符串（防拼写漂移；data-model §5 值域）
        if (modeToKey(PanelLayoutMode::DualOnly) != QLatin1String("DualOnly")) assert(false);
        if (modeToKey(PanelLayoutMode::DualWithConsole) != QLatin1String("DualWithConsole"))
            assert(false);
        if (modeToKey(PanelLayoutMode::DualReversedOnly) != QLatin1String("DualReversedOnly"))
            assert(false);
        if (modeToKey(PanelLayoutMode::DualReversedWithConsole) !=
            QLatin1String("DualReversedWithConsole"))
            assert(false);
        // 未知 / 空 key → 回退 fallback（契约 §2）
        if (modeFromKey(QStringLiteral("UnknownMode"), PanelLayoutMode::DualOnly) !=
            PanelLayoutMode::DualOnly)
            assert(false);
        if (modeFromKey(QString(), PanelLayoutMode::DualReversedOnly) !=
            PanelLayoutMode::DualReversedOnly)
            assert(false);
    }

    puts("panel_layout_config_test: ALL PASS");
    return 0;
}
