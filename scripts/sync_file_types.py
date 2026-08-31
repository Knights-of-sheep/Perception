#!/usr/bin/env python3
"""文件类型清单同步/校验：解析 file_type_catalog.h 派生文档（单一事实来源）。

用法：
    python scripts/sync_file_types.py --check    # 校验文档与目录一致（退出码 0/1）
    python scripts/sync_file_types.py --update   # 按目录重生成文档标记块

依据：specs/009-supported-file-types（宪法「工作流规则」）：
目录（src/core/io/file_type_catalog.h）是程序格式范围的唯一事实来源
（FR-001/FR-005/FR-006），文档不得手抄格式清单；目录变更后必须
--update 同步，--check 作为门禁（FR-011 / SC-005）。

管理的文档产物（<!-- sync_file_types:start/end --> 标记块）：
- README.md：打开文件过滤行（全量条目，与打开对话框一致）
- docs/architecture.md：格式范围表（全量条目，含状态）

解析失败（空目录）时输出明确消息并不崩溃。
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CATALOG_HEADER = ROOT / "src" / "core" / "io" / "file_type_catalog.h"
README = ROOT / "README.md"
ARCH = ROOT / "docs" / "architecture.md"

START_MARK = "<!-- sync_file_types:start -->"
END_MARK = "<!-- sync_file_types:end -->"

# 目录条目：{"名称", {".ext", ...}, FileTypeFamily::X, FileTypeKind::Y, FileTypeStatus::Z}
ENTRY_RE = re.compile(
    r'\{"([^"]+)"\s*,\s*\{(.*?)\}\s*,\s*'
    r"FileTypeFamily::(\w+)\s*,\s*FileTypeKind::(\w+)\s*,\s*"
    r"FileTypeStatus::(\w+)\}",
    re.S,
)
EXT_RE = re.compile(r'"(\.[A-Za-z0-9]+)"')

# 格式族枚举名 → 打开过滤分组键（与 file_type_catalog.cpp familyKey 一致）
FAMILY_GROUP = {
    "VtkLegacy": "VTK",
    "VtkXml": "VTK",
    "VtkComposite": "VTK",
    "VtkParallel": "VTK",
    "SVisual": "SVisual",
    "Hdf5": "HDF5",
    "Curve": "Curve Data",
}
# 分组键 → 文档显示名（README 过滤行）
FAMILY_LABEL = {"VTK": "VTK", "SVisual": "SVisual", "HDF5": "HDF5", "Curve Data": "曲线"}
KIND_LABEL = {"Curve": "曲线", "Structure": "结构", "Both": "双用途"}
STATUS_LABEL = {"Supported": "已支持", "Planned": "规划中"}


def parse_catalog():
    """解析权威目录头，返回有序条目列表（含格式族枚举名）。"""
    text = CATALOG_HEADER.read_text(encoding="utf-8")
    entries = []
    for m in ENTRY_RE.finditer(text):
        name, ext_group, family, kind, status = m.groups()
        exts = EXT_RE.findall(ext_group)
        entries.append({
            "format_name": name,
            "extensions": exts,
            "family_group": FAMILY_GROUP.get(family, family),
            "kind": kind,
            "status": status,
        })
    return entries


def all_groups(entries):
    """全量条目（含规划中）按格式族分组 → [(分组键, [*.ext 模式])]，保持目录顺序。

    与打开对话框过滤一致（FR-006 / FR-009-变更：展示全部类型供发现与核对，
    规划中格式由加载路径按「不支持」拦截）；与 file_type_catalog.cpp filterGroups()
    保持同一交叉聚合规则：曲线数据组追加其他格式族中 kind==Curve 的条目
    （当前为 SVisual .plt）。
    """
    groups = []
    for e in entries:
        key = e["family_group"]
        patterns = ["*" + x for x in e["extensions"]]
        for g in groups:
            if g[0] == key:
                g[1].extend(patterns)
                break
        else:
            groups.append([key, patterns])
    # 交叉聚合（与 C++ filterGroups 一致）
    for g in groups:
        if g[0] != "Curve Data":
            continue
        for e in entries:
            if e["family_group"] == "Curve Data" or e["kind"] != "Curve":
                continue
            for ext in e["extensions"]:
                pattern = "*" + ext
                if pattern not in g[1]:
                    g[1].append(pattern)
    return groups


def readme_bullet(entries):
    """README 打开文件过滤行（全量条目，与打开对话框一致）。"""
    groups = all_groups(entries)
    if not groups:
        return "- **打开文件过滤**：（目录无「已支持」格式）"
    labels = " / ".join(
        "%s（`%s`）" % (FAMILY_LABEL.get(g[0], g[0]), " ".join(g[1]))
        for g in groups
    )
    return "- **打开文件过滤**：%s" % labels


def arch_table(entries):
    """architecture.md 格式范围表（全量条目，含状态；FR-005 完整清单）。"""
    if not entries:
        return "（目录为空）"
    rows = ["| 格式名称 | 扩展名 | 数据种类 | 状态 |", "|---|---|---|---|"]
    for e in entries:
        exts = " ".join("`%s`" % x for x in e["extensions"])
        rows.append("| %s | %s | %s | %s |" % (
            e["format_name"],
            exts,
            KIND_LABEL.get(e["kind"], e["kind"]),
            STATUS_LABEL.get(e["status"], e["status"]),
        ))
    return "\n".join(rows)


def render_block(content):
    return "%s\n%s\n%s" % (START_MARK, content, END_MARK)


def current_block(text):
    m = re.search(
        r"%s\n(.*?)\n%s" % (re.escape(START_MARK), re.escape(END_MARK)),
        text, re.S)
    return m.group(1) if m else None


def update_marked(text, content, fallback_anchor):
    """替换标记块；无标记时首次迁移（README 换掉过滤行 / architecture 追加到尾部）。"""
    block = render_block(content)
    if START_MARK in text:
        return re.sub(
            r"%s\n.*?\n%s" % (re.escape(START_MARK), re.escape(END_MARK)),
            lambda _: block, text, count=1, flags=re.S)
    if fallback_anchor:
        idx = text.find(fallback_anchor)
        if idx >= 0:
            line_start = text.rfind("\n", 0, idx) + 1
            line_end = text.find("\n", idx)
            if line_end == -1:
                line_end = len(text)
            return text[:line_start] + block + text[line_end:]
    return text.rstrip("\n") + "\n\n" + block + "\n"


def ensure_marked(text, content, fallback_anchor):
    """确保标记块存在且内容正确；返回 (新文本, 是否有差异)。"""
    if current_block(text) == content:
        return text, False
    return update_marked(text, content, fallback_anchor), True


def main():
    parser = argparse.ArgumentParser(description="同步/校验文件类型清单文档")
    parser.add_argument("--check", action="store_true",
                        help="校验文档与目录一致；不一致打印差异并退出码 1")
    parser.add_argument("--update", action="store_true",
                        help="按目录重生成文档标记块")
    args = parser.parse_args()
    if not (args.check or args.update):
        parser.error("需要 --check 或 --update")

    entries = parse_catalog()
    if not entries:
        print("同步中止：%s 未解析到任何条目（目录为空或格式变更）" %
              CATALOG_HEADER.relative_to(ROOT))
        return 2

    readme_text = README.read_text(encoding="utf-8")
    arch_text = ARCH.read_text(encoding="utf-8")
    new_readme, d_readme = ensure_marked(readme_text, readme_bullet(entries),
                                         "- **打开文件过滤**")
    new_arch, d_arch = ensure_marked(arch_text, arch_table(entries), None)

    if args.update:
        if d_readme:
            README.write_text(new_readme, encoding="utf-8")
            print("已更新 README.md：打开文件过滤（%d 条格式）" % len(entries))
        if d_arch:
            ARCH.write_text(new_arch, encoding="utf-8")
            print("已更新 docs/architecture.md：格式范围表（%d 条格式）" % len(entries))
        if not (d_readme or d_arch):
            print("无需更新：文档与目录一致")
        return 0

    # --check
    if not (d_readme or d_arch):
        print("OK：文档与目录一致（%d 条格式）" % len(entries))
        return 0
    if d_readme:
        print("[DIFF] README.md 打开文件过滤与目录不一致")
        print("  期望: %s" % readme_bullet(entries))
    if d_arch:
        print("[DIFF] docs/architecture.md 格式范围表与目录不一致")
    return 1


if __name__ == "__main__":
    sys.exit(main())
