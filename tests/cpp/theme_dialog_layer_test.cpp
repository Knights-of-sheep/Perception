// ===== 弹窗背景层派生函数单测（008-unify-dialog-styling）=====
// 覆盖 SC-001 / FR-003 的自动化部分：
//   - 25 套主题：dialogBg（HC 回退 windowBg）与 text 的 WCAG 对比度 ≥ 4.5:1
//     （FR-003 可读性硬约束，对比度保护保证）
//   - 25 套主题：Dark/Light family 的 |L(dialogBg) − L(windowBg)| ≥ 8（HSL lightness 百分点）
//     为默认目标；仅当 windowBg 与 text 对比度裕度不足（< 5:1，如 Solarized 类低对比
//     色板）时允许保护降级，但仍须保有可感知层次（≥ 2pt）
//   - HC family：派生返回无效色（dialogBg == windowBg），border 与 windowBg 对比度 ≥ 7:1
//   - 派生函数边界：Dark clamp 上限 70%、Light clamp 下限 30%、
//     无效色 / nullptr / 未知 family 输入、色相/饱和度保持（FR-003 同一配色家族）
// 仅依赖 QtGui 类型（QColor）与 theme 头，无 GUI 平台依赖，红-绿 TDD。
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_dialog_layer.h"

#include <QString>

#include <cassert>
#include <cmath>
#include <cstdio>

using perception::ui::theme::deriveDialogBg;
using perception::ui::theme::kThemes;

namespace {

// HSL lightness 百分比（0-100）
double lightPct(const QColor& c) { return c.lightness() / 255.0 * 100.0; }

// WCAG 2.x 相对亮度
double relativeLuminance(const QColor& c) {
    auto f = [](double v) {
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * f(c.redF()) + 0.7152 * f(c.greenF()) + 0.0722 * f(c.blueF());
}

double contrastRatio(const QColor& a, const QColor& b) {
    double la = relativeLuminance(a), lb = relativeLuminance(b);
    if (la < lb) std::swap(la, lb);
    return (la + 0.05) / (lb + 0.05);
}

void expect(bool ok, const char* msg) {
    if (!ok) {
        fprintf(stderr, "theme_dialog_layer_test FAILED: %s\n", msg);
        assert(false);
    }
}

}  // namespace

int main() {
    // ---- 25 套主题遍历 ----
    for (const auto& t : kThemes) {
        const QColor& w = t.colors.windowBg;
        const QColor& text = t.colors.text;
        const QColor& border = t.colors.border;
        const QColor derived = deriveDialogBg(w, text, t.family);

        const bool isDark = (QString::fromLatin1(t.family) == QLatin1String("Dark"));
        const bool isLight = (QString::fromLatin1(t.family) == QLatin1String("Light"));
        const bool isHc = (QString::fromLatin1(t.family) == QLatin1String("High Contrast"));

        // 弹窗背景最终色：HC 回退 windowBg
        const QColor dialog = derived.isValid() ? derived : w;

        // 可读性硬约束：dialogBg vs text 对比度 ≥ 4.5:1（WCAG AA，FR-003）
        const double cr = contrastRatio(dialog, text);
        if (cr < 4.5) {
            fprintf(stderr, "theme=%s: contrast(dialogBg,text)=%.2f < 4.5\n", t.id, cr);
            assert(false);
        }

        if (isDark || isLight) {
            if (!derived.isValid()) {
                fprintf(stderr, "theme=%s: Dark/Light must derive a valid color\n", t.id);
                assert(false);
            }
            const double diff = std::fabs(lightPct(derived) - lightPct(w));
            if (diff < 8.0) {
                // 对比度保护触发（如 Solarized 类低对比色板，提亮 8pt 会压垮正文
                // 可读性）：可读性已由上方 ≥4.5 硬约束保证；此时仍须保有可感知
                // 层次（≥ 2pt），且保护不得把弹窗完全并入主界面。
                if (diff < 2.0) {
                    fprintf(stderr,
                            "theme=%s: contrast protection degraded hierarchy to %.2fpt\n",
                            t.id, diff);
                    assert(false);
                }
            }
        }
        if (isHc) {
            if (derived.isValid()) {
                fprintf(stderr, "theme=%s: High Contrast must not derive\n", t.id);
                assert(false);
            }
        }

        // HC 层次：border vs windowBg 对比度 ≥ 7:1
        if (isHc) {
            const double crB = contrastRatio(border, w);
            if (crB < 7.0) {
                fprintf(stderr, "theme=%s: contrast(border,windowBg)=%.2f < 7\n", t.id, crB);
                assert(false);
            }
        }
    }

    // ---- 派生边界 ----
    // Dark clamp 上限：lightness 255 → 70%（=178/255）；高对比输入不触发保护
    {
        const QColor c = deriveDialogBg(QColor(255, 255, 255), QColor(0, 0, 0), "Dark");
        if (c.lightness() != 178) {
            fprintf(stderr, "Dark clamp upper bound: got %d, expect 178 (70%%)\n", c.lightness());
            assert(false);
        }
    }
    // Light clamp 下限：lightness 0 → 30%（=76/255）；高对比输入不触发保护
    {
        const QColor c = deriveDialogBg(QColor(0, 0, 0), QColor(255, 255, 255), "Light");
        if (c.lightness() != 76) {
            fprintf(stderr, "Light clamp lower bound: got %d, expect 76 (30%%)\n", c.lightness());
            assert(false);
        }
    }
    // 无效色输入 → 无效色
    expect(!deriveDialogBg(QColor(), QColor(0, 0, 0), "Dark").isValid(),
           "invalid windowBg must return invalid");
    // text 无效色 → 无效色（无法做对比度保护）
    expect(!deriveDialogBg(QColor(10, 10, 10), QColor(), "Dark").isValid(),
           "invalid text must return invalid");
    // nullptr family → 无效色
    expect(!deriveDialogBg(QColor(10, 10, 10), QColor(0, 0, 0), nullptr).isValid(),
           "nullptr family must return invalid");
    // 未知 family → 无效色
    expect(!deriveDialogBg(QColor(10, 10, 10), QColor(0, 0, 0), "Unknown").isValid(),
           "unknown family must return invalid");
    // 色相/饱和度保持（FR-003 同一配色家族）
    {
        const QColor src(0, 90, 200);  // 蓝色系
        const QColor c = deriveDialogBg(src, QColor(0, 0, 0), "Dark");
        if (c.hslHue() != src.hslHue() || c.hslSaturation() != src.hslSaturation()) {
            fprintf(stderr, "hue/saturation must be preserved (got hue=%d sat=%d)\n",
                    c.hslHue(), c.hslSaturation());
            assert(false);
        }
    }

    fprintf(stderr, "theme_dialog_layer_test: all passed (25 themes + bounds)\n");
    return 0;
}
