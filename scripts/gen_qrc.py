#!/usr/bin/env python3
"""Perception theme.qrc 生成器（gen_qrc.py）—— T022

扫描 src/ui/theme/icons/png/ 下已渲染的 PNG/ICO，生成
qresource prefix="/perception/icons" 的 <file> 条目，
保留既有 theme qresource（QSS）不变。

用法: python scripts/gen_qrc.py
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
THEME_DIR = ROOT / "src/ui/theme"
QRC = THEME_DIR / "theme.qrc"
PNG_ACTIONS = THEME_DIR / "icons/png/actions"
PNG_APP = THEME_DIR / "icons/png/app"
ICO = THEME_DIR / "icons/app/app-icon.ico"


def build_icons_qresource() -> str:
    files = sorted(PNG_ACTIONS.glob("*.png")) + sorted(PNG_APP.glob("*.png"))
    if ICO.is_file():
        files.append(ICO)
    lines = ['    <qresource prefix="/perception/icons">']
    for f in files:
        rel = f.relative_to(THEME_DIR).as_posix()
        lines.append(f'        <file>{rel}</file>')
    lines.append('    </qresource>')
    return "\n".join(lines)


def main() -> int:
    if not PNG_ACTIONS.is_dir() or not list(PNG_ACTIONS.glob("*.png")):
        print("[err] 缺少已渲染 PNG，请先运行 scripts/render_icons.py")
        return 1
    old = QRC.read_text(encoding="utf-8") if QRC.is_file() else ""
    theme_block = '    <qresource prefix="/perception/theme">\n        <file>theme_template.qss</file>\n    </qresource>'
    icons_block = build_icons_qresource()
    content = f"<RCC>\n{theme_block}\n{icons_block}\n</RCC>\n"
    if content == old:
        print("theme.qrc: 无变化")
    else:
        QRC.write_text(content, encoding="utf-8")
        print(f"theme.qrc: 更新（{len(list(PNG_ACTIONS.glob('*.png')))} actions PNG + "
              f"{len(list(PNG_APP.glob('*.png')))} app PNG + 1 ICO）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
