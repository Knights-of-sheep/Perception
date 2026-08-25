#!/usr/bin/env python3
"""Perception 图标渲染器（render_icons.py）

将 SVG 源渲染为 PNG 位图（T017）：
- 功能图标: 16/24/32 px -> src/ui/theme/icons/png/actions/<icon_id>-<size>.png
- 应用图标: 16/24/32/48/64/128/256 px + .ico -> src/ui/theme/icons/png/app/

依赖: PyQt5 (QtSvg 插件) + Pillow (仅 .ico 合成)。
用法: python scripts/render_icons.py
"""
from __future__ import annotations

import sys
from pathlib import Path

from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QImage, QPainter
from PyQt5.QtSvg import QSvgRenderer

ACTION_SIZES = [16, 24, 32]
APP_SIZES = [16, 24, 32, 48, 64, 128, 256]
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]

ROOT = Path(__file__).resolve().parent.parent
ACTIONS_SRC = ROOT / "src/ui/theme/icons/actions"
APP_SRC = ROOT / "src/ui/theme/icons/app"
PNG_ACTIONS = ROOT / "src/ui/theme/icons/png/actions"
PNG_APP = ROOT / "src/ui/theme/icons/png/app"


def render_png(svg_path: Path, out_path: Path, size: int) -> None:
    """用 QSvgRenderer 渲染单个尺寸 PNG（抗锯齿 + 平滑缩放）。"""
    renderer = QSvgRenderer(str(svg_path))
    image = QImage(size, size, QImage.Format_ARGB32)
    image.fill(0x00000000)  # 全透明
    painter = QPainter(image)
    painter.setRenderHint(QPainter.Antialiasing, True)
    painter.setRenderHint(QPainter.SmoothPixmapTransform, True)
    renderer.render(painter, QRectF(0, 0, size, size))
    painter.end()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    ok = image.save(str(out_path), "PNG")
    if not ok:
        raise RuntimeError(f"PNG 保存失败: {out_path}")


def build_ico(ico_path: Path) -> None:
    """用 Pillow 将各尺寸 PNG 合成为多分辨率 .ico。"""
    from PIL import Image
    images = []
    for size in ICO_SIZES:
        png = PNG_APP / f"app-icon-{size}.png"
        if png.is_file():
            images.append(Image.open(str(png)))
    ico_path.parent.mkdir(parents=True, exist_ok=True)
    if images:
        images[0].save(
            str(ico_path), format="ICO",
            append_images=images[1:], sizes=[(s, s) for s in ICO_SIZES],
        )
        print(f"[ico] {ico_path} ({len(images)} 分辨率)")


def main() -> int:
    if not ACTIONS_SRC.is_dir():
        print(f"[err] 缺少功能图标源目录: {ACTIONS_SRC}")
        return 1

    total = 0
    for svg in sorted(ACTIONS_SRC.glob("*.svg")):
        icon_id = svg.stem
        for size in ACTION_SIZES:
            out = PNG_ACTIONS / f"{icon_id}-{size}.png"
            render_png(svg, out, size)
            total += 1
    print(f"[actions] 渲染 {total} 个 PNG ({ACTIONS_SRC.name})")

    app_svg = APP_SRC / "app-icon.svg"
    if app_svg.is_file():
        for size in APP_SIZES:
            out = PNG_APP / f"app-icon-{size}.png"
            render_png(app_svg, out, size)
        build_ico(ROOT / "src/ui/theme/icons/app/app-icon.ico")
        print(f"[app] 渲染 {len(APP_SIZES)} 个 PNG + .ico")

    print("render_icons: DONE")
    return 0


if __name__ == "__main__":
    sys.exit(main())
