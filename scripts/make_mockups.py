#!/usr/bin/env python3
"""Perception 图标集视觉稿生成器（make_mockups.py）

T020: preview.png        全部图标总览（网格 + 语义标签）
T021: icon-bar-mockup    侧边栏图标栏 mockup（含 hover/selected/disabled 状态示例）
      main-window-mockup 主窗口 mockup（工具栏 + 视图区）

依赖: PyQt5 + PyYAML。用法: python scripts/make_mockups.py
"""
from __future__ import annotations

import sys
from pathlib import Path

import yaml
from PyQt5.QtCore import QRect, Qt
from PyQt5.QtGui import QColor, QFont, QGuiApplication, QPainter, QPixmap

ROOT = Path(__file__).resolve().parent.parent
MAP = ROOT / "src/ui/theme/icons/icon-map.yaml"
PNG_ACTIONS = ROOT / "src/ui/theme/icons/png/actions"
PNG_APP = ROOT / "src/ui/theme/icons/png/app"
OUT = ROOT / "docs/design/mockups/005-icon-set"

BG_VIEW = QColor("#161616")
BG_PANEL = QColor("#252526")
BG_CONTROL = QColor("#3C3C3C")
BG_SELECTED = QColor("#094771")
BORDER = QColor("#454545")
FG_TEXT = QColor("#D4D4D4")
FG_WEAK = QColor("#9D9D9D")
ACCENT = QColor("#0A84FF")


def icon_ids() -> list:
    data = yaml.safe_load(MAP.read_text(encoding="utf-8"))
    return [e["icon_id"] for e in data["entries"]]


def draw_icon(painter: QPainter, icon_id: str, x: int, y: int, size: int = 24,
              alpha: int = 255) -> None:
    png = PNG_ACTIONS / f"{icon_id}-{size}.png"
    pm = QPixmap(str(png))
    if alpha < 255:
        t = QPixmap(pm.size())
        t.fill(Qt.transparent)
        p = QPainter(t)
        p.setOpacity(alpha / 255)
        p.drawPixmap(0, 0, pm)
        p.end()
        pm = t
    painter.drawPixmap(x, y, size, size, pm)


def draw_label(painter: QPainter, text: str, x: int, y: int, color=FG_WEAK,
               font_size: int = 11, align=Qt.AlignHCenter | Qt.AlignTop) -> None:
    painter.setPen(color)
    font = QFont("Microsoft YaHei", font_size)
    painter.setFont(font)
    painter.drawText(x, y, 120, 20, align, text)


def make_preview() -> None:
    ids = icon_ids()
    cols, cell_w, cell_h = 8, 120, 68
    rows = (len(ids) + cols - 1) // cols
    img = QPixmap(cols * cell_w + 20, rows * cell_h + 30)
    img.fill(BG_VIEW)
    p = QPainter(img)
    p.setRenderHint(QPainter.Antialiasing, True)
    for i, iid in enumerate(ids):
        r, c = divmod(i, cols)
        x, y = 10 + c * cell_w, 10 + r * cell_h
        draw_icon(p, iid, x + 8, y + 2, 24)
        draw_label(p, iid, x, y + 30)
    # 应用图标
    draw_icon(p, "app-icon", 10 + 4 * cell_w + 8, 10 + rows * cell_h + 2, 24)
    draw_label(p, "app-icon", 10 + 4 * cell_w, 10 + rows * cell_h + 30)
    p.end()
    out = OUT / "preview.png"
    img.save(str(out), "PNG")
    print(f"[preview] {out}")


def make_icon_bar() -> None:
    """侧边栏图标栏 mockup：竖条 + 状态示例。"""
    w, h = 220, 320
    img = QPixmap(w, h)
    img.fill(BG_PANEL)
    p = QPainter(img)
    p.setRenderHint(QPainter.Antialiasing, True)
    # 栏背景
    p.fillRect(0, 0, 220, h, BG_PANEL)
    # 标题
    p.setPen(FG_WEAK)
    p.setFont(QFont("Microsoft YaHei", 10))
    p.drawText(QRect(10, 8, 200, 18), Qt.AlignLeft, "ICON BAR (sidebar)")
    # 第一组: 正常态
    bar_icons = ["file-open", "file-save", "file-export-data",
                 "view-zoom-in", "view-zoom-out", "view-fit-screen",
                 "analysis-probe", "analysis-extract", "animation-play"]
    y = 34
    for i, iid in enumerate(bar_icons):
        # 选中态示例（第 4 个高亮）
        if i == 3:
            p.fillRect(10, y - 2, 40, 28, BG_SELECTED)
            p.setPen(ACCENT)
        else:
            p.setPen(Qt.transparent)
        draw_icon(p, iid, 16, y, 24)
        p.setPen(FG_WEAK)
        p.setFont(QFont("Microsoft YaHei", 9))
        p.drawText(QRect(54, y, 156, 24), Qt.AlignVCenter, iid)
        y += 30
    # 分隔线
    p.setPen(BORDER)
    p.drawLine(10, y + 2, 210, y + 2)
    # 状态示例: normal / hover / pressed / disabled / selected
    states = [
        ("normal  ", "file-open", 255, None),
        ("hover   ", "file-save", 255, BG_CONTROL),
        ("pressed ", "file-export-data", 255, BG_CONTROL),
        ("disabled", "file-close", 102, None),
        ("selected", "file-remove", 255, BG_SELECTED),
    ]
    p.setPen(FG_WEAK)
    p.setFont(QFont("Microsoft YaHei", 9))
    p.drawText(QRect(10, y + 6, 200, 18), Qt.AlignLeft, "STATES")
    y += 28
    for name, iid, alpha, bg in states:
        if bg is not None:
            p.fillRect(10, y, 200, 26, bg)
        draw_icon(p, iid, 16, y, 24, alpha=alpha)
        p.setPen(FG_WEAK)
        p.drawText(QRect(54, y, 156, 26), Qt.AlignVCenter, name)
        y += 30
    p.end()
    out = OUT / "icon-bar-mockup.png"
    img.save(str(out), "PNG")
    print(f"[icon-bar] {out}")


def make_main_window() -> None:
    """主窗口 mockup：顶部工具栏 + 视图区。"""
    w, h = 640, 400
    img = QPixmap(w, h)
    img.fill(BG_VIEW)
    p = QPainter(img)
    p.setRenderHint(QPainter.Antialiasing, True)
    # 工具栏
    p.fillRect(0, 0, w, 48, BG_PANEL)
    p.setPen(BORDER)
    p.drawLine(0, 48, w, 48)
    toolbar = ["file-open", "file-save", "edit-undo", "edit-redo",
               "view-rotate", "view-pan", "view-zoom-in", "view-zoom-out",
               "analysis-probe", "animation-play", "animation-pause",
               "tools-settings"]
    x = 8
    for i, iid in enumerate(toolbar):
        if i in (3, 8):
            x += 6  # 分隔
        draw_icon(p, iid, x, 10, 24)
        x += 28
    # 左侧面板
    p.fillRect(0, 48, 96, h - 48, BG_PANEL)
    for i, iid in enumerate(["file-open", "file-save", "file-export-data",
                             "view-zoom-in", "view-zoom-out", "animation-play"]):
        if i == 0:
            p.fillRect(8, 56 + i * 36, 80, 30, BG_SELECTED)
        draw_icon(p, iid, 14, 60 + i * 36, 20)
    # 视图区
    p.setPen(BORDER)
    p.drawRect(103, 55, w - 110, h - 62)
    p.setPen(FG_WEAK)
    p.setFont(QFont("Microsoft YaHei", 10))
    p.drawText(QRect(110, 60, 200, 18), Qt.AlignLeft, "VIEWPORT")
    # 状态栏
    p.setPen(BORDER)
    p.drawLine(0, h - 24, w, h - 24)
    p.setPen(FG_WEAK)
    p.setFont(QFont("Microsoft YaHei", 8))
    p.drawText(QRect(8, h - 22, w - 16, 18), Qt.AlignLeft,
               "Icon set mockup - Perception (dark theme)")
    p.end()
    out = OUT / "main-window-mockup.png"
    img.save(str(out), "PNG")
    print(f"[main-window] {out}")


def main() -> int:
    _app = QGuiApplication(sys.argv)  # QPainter 绘制必需
    OUT.mkdir(parents=True, exist_ok=True)
    try:
        make_preview()
        make_icon_bar()
        make_main_window()
    except Exception:
        import traceback
        traceback.print_exc()
        return 1
    print("make_mockups: DONE")
    return 0


if __name__ == "__main__":
    sys.exit(main())
