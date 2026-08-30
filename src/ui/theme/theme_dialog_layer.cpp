#include "ui/theme/theme_dialog_layer.h"

#include <QString>

#include <algorithm>
#include <cmath>

namespace perception {
namespace ui {
namespace theme {

namespace {
// HSL lightness 步进：+8 个百分点 ≈ 0.08 × 255 = 20.4 → 向上取整 21，
// 保证 |ΔL|/255 ≥ 8% 严格成立（SC-001 默认目标，不受整数取整影响）。
constexpr int kLightStep = 21;
// clamp 边界：70% × 255 = 178.5 → 178；30% × 255 = 76.5 → 76
constexpr int kDarkClampMax = 178;
constexpr int kLightClampMin = 76;
// 对比度保护（FR-003）：回调步长与 WCAG AA 下限
constexpr int kProtectStep = 3;
constexpr double kMinContrast = 4.5;

int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// WCAG 2.x 相对亮度与对比度
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
}  // namespace

// 派生策略（research R1 修订）：Dark 提亮一档、Light 压暗一档，均保持色相/饱和度
// （同一配色家族，FR-003）；High Contrast 不派生（返回无效色 → 渲染回退 windowBg，
// 层次靠 QSS 1px 高对比边框）。单一实现点覆盖全部 25 套主题。
// 对比度保护：对 windowBg 与 text 亮度接近的低对比色板（如 Solarized），逐级提亮
// 会迅速压垮正文可读性，故回调至 contrast ≥ 4.5（FR-003 硬约束）——层次退居其次，
// 但最低回退到 windowBg 本身（其与 text 的既有对比度已被色板保证）。
QColor deriveDialogBg(const QColor& windowBg, const QColor& text, const char* family) {
    if (!windowBg.isValid() || !text.isValid() || family == nullptr) return QColor();

    const QString fam = QString::fromLatin1(family);
    const int hue = windowBg.hslHue();
    const int sat = windowBg.hslSaturation();

    if (fam == QLatin1String("Dark")) {
        int l = clamp(windowBg.lightness() + kLightStep, 0, kDarkClampMax);
        QColor c = windowBg;
        c.setHsl(hue, sat, l);
        while (l > windowBg.lightness() && contrastRatio(c, text) < kMinContrast) {
            l -= kProtectStep;
            c.setHsl(hue, sat, l);
        }
        return c;
    }
    if (fam == QLatin1String("Light")) {
        int l = clamp(windowBg.lightness() - kLightStep, kLightClampMin, 255);
        QColor c = windowBg;
        c.setHsl(hue, sat, l);
        while (l < windowBg.lightness() && contrastRatio(c, text) < kMinContrast) {
            l += kProtectStep;
            c.setHsl(hue, sat, l);
        }
        return c;
    }
    // High Contrast 及其余 family：不派生
    return QColor();
}

}  // namespace theme
}  // namespace ui
}  // namespace perception
