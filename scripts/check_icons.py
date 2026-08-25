#!/usr/bin/env python3
"""Perception 图标符合性校验器（check_icons.py）

契约依据:
- specs/002-icon-design/contracts/icon-style-spec.md   (S/G/P/T/N/A)
- specs/002-icon-design/contracts/icon-function-map.md (命名规则 / schema / 覆盖规则)
- docs/design/ui-guidelines.md §3.1                    (Token 色板唯一来源)

校验项:
1. 色板白名单: 所有 SVG 的 stroke/fill 色值必须来自 Token 白名单，禁止新增色值 (P-01)
               fill="none" 允许（辅助描边元素，S-01 例外）
2. 命名规则  : actions/*.svg 文件名必须匹配 <功能区>-<功能>[-<变体>]，kebab-case (N-01)
3. 覆盖规则  : icon-map.yaml entries <-> actions/*.svg 一一对应 (icon-function-map 规则1/3)
4. 字段约束  : icon_id 唯一 / semantic 非空 / category 枚举 / states 含五态 / sizes 含 16
               / benchmark_ref.source 枚举 / benchmark_ref.function 非空 (schema §3)

退出码: 0 = 全部通过；1 = 存在违反项。
用法:   python scripts/check_icons.py [--icons-dir DIR] [--map PATH] [--app-dir DIR]
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False

# --- Token 白名单（唯一来源 docs/design/ui-guidelines.md §3.1） ---
TOKEN_WHITELIST = {
    "#1e1e1e",  # BG_WINDOW
    "#252526",  # BG_PANEL
    "#3c3c3c",  # BG_CONTROL
    "#161616",  # BG_VIEW
    "#d4d4d4",  # FG_TEXT
    "#9d9d9d",  # FG_TEXT_WEAK
    "#6e6e6e",  # FG_TEXT_DISABLED
    "#454545",  # BORDER
    "#3f3f3f",  # BORDER_WEAK
    "#0a84ff",  # ACCENT
    "#094771",  # SELECTION_BG
    "#4ec9b0",  # SUCCESS
    "#cca700",  # WARNING
    "#f14c4c",  # DANGER
}
CATEGORIES = {"file", "edit", "view", "analysis", "animation", "tools"}
SOURCE_ENUM = {"paraview", "svisual", "both", "custom"}
FIVE_STATES = {"normal", "hover", "pressed", "disabled", "selected"}

NAME_RE = re.compile(r"^(file|edit|view|analysis|animation|tools)-[a-z0-9]+(-[a-z0-9]+)*\.svg$")
COLOR_ATTR_RE = re.compile(r"(stroke|fill)=([\"'])(#[0-9a-fA-F]{3,8}|none|None|currentColor)\2")


def norm_color(raw: str) -> str:
    """归一化色值: 统一小写、3位十六进制展开为6位。"""
    c = raw.strip().lower()
    if c.startswith("#") and len(c) == 4:  # #abc -> #aabbcc
        c = "#" + "".join(ch * 2 for ch in c[1:])
    return c


def scan_svg(path: Path, violations: list) -> None:
    """检查单个 SVG 的色板白名单（S-01/P-01）。

    实心填充风格：fill 允许 none 或 Token 白名单色（负形挖空用
    BG_VIEW/BG_CONTROL、主体用 FG_TEXT 等），stroke 同理必须白名单。
    """
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        violations.append(f"[io] {path}: 无法读取 ({exc})")
        return
    for attr, _, color in COLOR_ATTR_RE.findall(text):
        if color.lower() in ("none", "currentcolor"):
            continue
        if norm_color(color) not in TOKEN_WHITELIST:
            violations.append(
                f"[P-01] {path}: 非 Token 色值 {color} (来源: ui-guidelines §3.1)"
            )


def check_naming(actions_dir: Path, violations: list) -> list:
    """检查 actions/*.svg 命名规则（N-01）。"""
    svg_files = sorted(actions_dir.glob("*.svg"))
    for f in svg_files:
        if not NAME_RE.match(f.name):
            violations.append(
                f"[N-01] {f.name}: 命名必须为 <功能区>-<功能>[-<变体>].svg，"
                f"功能区 ∈ {sorted(CATEGORIES)}"
            )
    return svg_files


def parse_yaml_simple(path: Path) -> list:
    """解析 icon-map.yaml 的 entries。

    优先 PyYAML（存在时）；回退到轻量行级解析（仅支持本项目 block-style 映射表）。
    返回 [{icon_id, semantic, category, benchmark_ref:{source,function}, states, sizes}]。
    """
    if HAS_YAML:
        try:
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
            if isinstance(data, dict):
                return data.get("entries") or []
        except Exception:
            pass
    entries = []
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        print(f"[io] {path}: 无法读取 ({exc})")
        return entries
    current = None
    key = None
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        m = re.match(r"- +icon_id:\s*(.+)", line)
        if m:
            if current:
                entries.append(current)
            current = {"icon_id": m.group(1).strip(), "_pending": []}
            continue
        if current is None:
            continue
        m = re.match(r"(\w+):\s*(.+)", line)
        if m:
            key, value = m.group(1), m.group(2).strip()
            if value.startswith("[") and value.endswith("]"):
                value = [v.strip() for v in value[1:-1].split(",") if v.strip()]
            if key in ("source", "function"):
                current.setdefault("benchmark_ref", {})[key] = value
            else:
                current[key] = value
            continue
        m = re.match(r"\s+(source|function):\s*(.+)", line)
        if m and current is not None:
            current.setdefault("benchmark_ref", {})[m.group(1)] = m.group(2).strip()
    if current:
        entries.append(current)
    return entries


def check_coverage(entries: list, svg_files: list, violations: list) -> None:
    """覆盖规则 1/3 + schema §3 字段约束。"""
    # 字段约束
    seen_ids, seen_semantics = set(), set()
    for e in entries:
        iid = e.get("icon_id", "")
        if not iid:
            violations.append("[schema] 存在缺少 icon_id 的条目")
            continue
        if iid in seen_ids:
            violations.append(f"[schema] icon_id 重复: {iid} (规则3)")
        seen_ids.add(iid)
        semantic = e.get("semantic", "")
        if not semantic:
            violations.append(f"[schema] {iid}: semantic 缺失")
        elif semantic in seen_semantics:
            violations.append(f"[schema] semantic 重复: {semantic} (规则3)")
        else:
            seen_semantics.add(semantic)
        cat = e.get("category", "")
        if cat not in CATEGORIES:
            violations.append(f"[schema] {iid}: category 非法 ({cat or '空'})")
        src = e.get("benchmark_ref", {}).get("source", "")
        if src not in SOURCE_ENUM:
            violations.append(f"[schema] {iid}: benchmark_ref.source 非法 ({src or '空'})")
        if not e.get("benchmark_ref", {}).get("function"):
            violations.append(f"[schema] {iid}: benchmark_ref.function 缺失")
        states = e.get("states")
        if not isinstance(states, list) or not FIVE_STATES.issubset(set(states)):
            violations.append(f"[schema] {iid}: states 必须含全部五态 {sorted(FIVE_STATES)}")
        sizes = e.get("sizes")
        if not isinstance(sizes, list) or "16" not in {str(s) for s in sizes}:
            violations.append(f"[schema] {iid}: sizes 必须至少含 16")
    # 覆盖规则 1: entries <-> 文件一一对应
    file_ids = {f.name[:-4] for f in svg_files}
    entry_ids = {e["icon_id"] for e in entries if e.get("icon_id")}
    missing_files = entry_ids - file_ids
    orphan_files = file_ids - entry_ids
    for iid in sorted(missing_files):
        violations.append(f"[coverage/规则1] {iid}: 映射表有条目但缺少 SVG 文件")
    for iid in sorted(orphan_files):
        violations.append(f"[coverage/规则1] {iid}: 存在 SVG 文件但映射表无条目")


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Perception 图标符合性校验器")
    parser.add_argument("--icons-dir", default=str(repo_root / "src/ui/theme/icons/actions"),
                        help="功能图标目录（默认 src/ui/theme/icons/actions）")
    parser.add_argument("--app-dir", default=str(repo_root / "src/ui/theme/icons/app"),
                        help="应用图标目录（默认 src/ui/theme/icons/app）")
    parser.add_argument("--map", default=str(repo_root / "src/ui/theme/icons/icon-map.yaml"),
                        help="映射表路径（默认 src/ui/theme/icons/icon-map.yaml）")
    args = parser.parse_args()

    actions_dir = Path(args.icons_dir)
    app_dir = Path(args.app_dir)
    map_path = Path(args.map)

    violations: list = []
    count_total, count_scanned = 0, 0

    # 1/2. 色板 + 填充约束（功能 + 应用图标）
    for d in (actions_dir, app_dir):
        if not d.is_dir():
            violations.append(f"[io] 目录不存在: {d}")
            continue
        for svg in sorted(d.glob("*.svg")):
            count_total += 1
            count_scanned += 1
            scan_svg(svg, violations)

    # 3. 命名规则（仅功能图标）
    svg_files = check_naming(actions_dir, violations) if actions_dir.is_dir() else []

    # 4/5. 覆盖 + 字段约束
    if map_path.is_file():
        entries = parse_yaml_simple(map_path)
        check_coverage(entries, svg_files, violations)
    else:
        violations.append(f"[io] 映射表不存在: {map_path}")

    # 报告
    if violations:
        print(f"check_icons: FAIL - {len(violations)} 项违反（扫描 {count_scanned} 个 SVG）")
        for v in violations:
            print(f"  [X] {v}")
        return 1
    print(f"check_icons: PASS - {count_scanned} 个 SVG、{len(svg_files)} 条命名、"
          f"{len(parse_yaml_simple(map_path))} 条映射全部符合")
    return 0


if __name__ == "__main__":
    sys.exit(main())
