"""supported_formats 查询契约测试（009-supported-file-types / US2）。

契约：specs/009-supported-file-types/contracts/python-supported-formats.md
依据：FR-005（运行时可查询、完整清单）、FR-003/FR-004（字段与状态语义）、
      FR-010（大小写不敏感）。

断言：
1. 返回 list、非空、条目数 ≥ 12（覆盖 4 格式族）。
2. 每条目含且仅含 5 个字段，字段值非空且符合值域。
3. 扩展名全局无重复（大小写归一后）。
4. 至少 1 条 supported 且与当前可打开格式对应（.plt/.csv）；planned 存在且不混入。
5. 连续两次调用返回一致（稳定性）。
"""

_FAMILY_VALUES = {
    "vtk-legacy", "vtk-xml", "vtk-composite", "vtk-parallel",
    "svisual", "hdf5", "curve",
}
_KIND_VALUES = {"curve", "structure", "both"}
_STATUS_VALUES = {"supported", "planned"}


def _call(perception_py):
    svc = perception_py.CommandService()
    return svc.supported_formats()


def test_returns_nonempty_list_covering_all_families(perception_py):
    result = _call(perception_py)
    assert isinstance(result, list)
    assert len(result) >= 12  # 契约：条目 ≥ 12 覆盖 4 格式族
    families = {e["family"] for e in result}
    # 4 格式族全覆盖：VTK（任一 vtk-*）/ SVisual / HDF5 / 曲线
    assert any(f.startswith("vtk-") for f in families)
    assert "svisual" in families
    assert "hdf5" in families
    assert "curve" in families


def test_each_entry_has_exactly_five_valid_fields(perception_py):
    for entry in _call(perception_py):
        assert set(entry.keys()) == {"extensions", "format_name", "family", "kind", "status"}
        assert entry["format_name"]
        assert isinstance(entry["extensions"], list) and entry["extensions"]
        for ext in entry["extensions"]:
            assert ext.startswith(".") and ext == ext.lower()  # 小写、含点（FR-010）
        assert entry["family"] in _FAMILY_VALUES
        assert entry["kind"] in _KIND_VALUES
        assert entry["status"] in _STATUS_VALUES


def test_extensions_globally_unique(perception_py):
    seen = set()
    for entry in _call(perception_py):
        for ext in entry["extensions"]:
            key = ext.lower()
            assert key not in seen, key
            seen.add(key)


def test_supported_and_planned_not_mixed(perception_py):
    result = _call(perception_py)
    supported = [e for e in result if e["status"] == "supported"]
    planned = [e for e in result if e["status"] == "planned"]
    # supported 存在且与当前可打开格式对应（.plt/.csv）
    supported_exts = {e for entry in supported for e in entry["extensions"]}
    assert ".plt" in supported_exts
    assert ".csv" in supported_exts
    # supported 语义不被 planned 混入
    assert supported_exts == {".plt", ".csv"}
    assert planned  # 规划中存在
    planned_exts = {e for entry in planned for e in entry["extensions"]}
    assert not (supported_exts & planned_exts)


def test_stable_across_calls(perception_py):
    assert _call(perception_py) == _call(perception_py)
