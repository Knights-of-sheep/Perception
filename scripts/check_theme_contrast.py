#!/usr/bin/env python3
"""主题对比度校验：解析 theme_catalog.h 全部主题，按 WCAG 校验文字/图标色组合。

用法：
    python scripts/check_theme_contrast.py

覆盖场景：
- 正文/弱文字/禁用文字 on 各区域底色（≥4.5 / ≥3.0 / ≥2.5）
- 选中文字/图标 on selectionBg、hover 文字/图标 on hoverBg（checked 按钮）
- 按钮文字 on accent、危险文字 on 危险背景
"""
import re
import sys

HEADER = sys.argv[1] if len(sys.argv) > 1 else "src/ui/theme/theme_catalog.h"
FIELDS = ["windowBg","panelBg","controlBg","viewBg",
          "text","textWeak","textDisabled","border","borderWeak",
          "accent","accentHover","accentPressed","selectionBg",
          "hoverBg","surfaceElev","itemHover","buttonHover","handleHover",
          "dockTitleBg","dangerText","dangerHoverBg",
          "success","warning","danger",
          "white","textOnSelection","textOnAccent",
          "dockDropHighlight"]

# (前景, 背景, 最低对比度, 说明)
CHECKS = [
    ("text", "windowBg", 4.5, "正文 on 窗口背景"),
    ("text", "panelBg", 4.5, "正文 on 面板"),
    ("text", "controlBg", 4.5, "正文 on 控件"),
    ("text", "viewBg", 4.5, "正文 on 视图"),
    ("textWeak", "windowBg", 3.0, "弱文字 on 窗口背景"),
    ("textWeak", "panelBg", 3.0, "弱文字 on 面板"),
    ("textWeak", "controlBg", 3.0, "弱文字 on 控件"),
    ("textWeak", "viewBg", 3.0, "弱文字 on 视图"),
    ("textWeak", "surfaceElev", 3.0, "弱文字 on 浮层"),
    ("textWeak", "hoverBg", 3.0, "普通图标 on 按钮 hover"),
    ("textWeak", "selectionBg", 3.0, "普通图标 on 选中容器"),
    ("textDisabled", "controlBg", 2.5, "禁用文字 on 控件"),
    ("textDisabled", "panelBg", 2.5, "禁用文字 on 面板"),
    ("textOnSelection", "selectionBg", 4.5, "选中文字/图标 on 选中底"),
    # textOnSelection 不会出现在 hoverBg 上：QSS 规则顺序保证 :checked 背景
    # （selectionBg）优先于 :hover（hoverBg），hover 文字走 @text@（已由
    # "text on 各背景"覆盖）。故不校验 textOnSelection on hoverBg。
    ("textOnAccent", "accent", 4.5, "按钮文字 on 强调色"),
    ("white", "dangerHoverBg", 3.0, "关闭按钮白字 on 危险 hover"),
    ("dangerText", "dockTitleBg", 3.0, "危险文字 on 面板标题"),
]


def _lum(h):
    h = h.lstrip("#")
    r, g, b = (int(h[i:i+2], 16)/255.0 for i in (0, 2, 4))
    def lin(x): return x/12.92 if x <= 0.04045 else ((x+0.055)/1.055)**2.4
    return 0.2126*lin(r) + 0.7152*lin(g) + 0.0722*lin(b)


def _ratio(a, b):
    la, lb = _lum(a), _lum(b)
    lo, hi = min(la, lb), max(la, lb)
    return (hi+0.05)/(lo+0.05)


def main():
    # 006-constitution-refactor：色块按明暗/对比拆分至独立头文件，需全量扫描
    from pathlib import Path
    root = Path(__file__).resolve().parent.parent
    text = open(root / HEADER, encoding="utf-8").read()
    blocks = {}
    for h in sorted((root / "src/ui/theme").glob("theme_catalog_*.h")):
        htext = h.read_text(encoding="utf-8")
        for m in re.finditer(r"inline const ThemeColors k(\w+)\s*=\s*\{(.*?)\n\};", htext, re.S):
            vals = re.findall(r'"#([0-9A-Fa-f]{6})"', m.group(2))
            if len(vals) == len(FIELDS):
                blocks[m.group(1)] = ["#" + v.lower() for v in vals]
    catalog = [(name, blocks[b]) for name, b in
               re.findall(r'\{"([\w-]+)"\s*,\s*"[^"]*"\s*,\s*"[^"]*"\s*,\s*k(\w+)\}', text)]

    bad = 0
    for tid, vals in catalog:
        c = dict(zip(FIELDS, vals))
        for fg, bg, mn, desc in CHECKS:
            r = _ratio(c[fg], c[bg])
            if r < mn:
                bad += 1
                print(f"[FAIL] {tid}: {desc} {fg} on {bg} = {r:.2f} (需 ≥{mn})")
    print(f"{len(catalog)} 套主题 × {len(CHECKS)} 组组合，失败 {bad} 项")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
