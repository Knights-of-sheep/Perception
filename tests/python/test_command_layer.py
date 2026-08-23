"""命令层主链路测试（占位）。

链路：加载 → 变换 → 查询 → 导出。
M5 之前 perception_py 未构建，全部 skip；M5 后逐项补断言。
"""

import pytest

pytestmark = pytest.mark.skip(reason="M5: perception_py 未构建")


def test_load_plt_returns_dataset(perception_py, sample_plt):
    ds = perception_py.load_plt(sample_plt)
    assert ds.name == "sample"
    assert len(ds.curves) >= 1


def test_transform_unit_scale(perception_py, sample_plt):
    ds = perception_py.load_plt(sample_plt)
    scaled = perception_py.transform(ds, "unit_scale", factor=1000.0)
    assert scaled.curves[0].y[0] == pytest.approx(100.0)


def test_query_curve(perception_py, sample_plt):
    ds = perception_py.load_plt(sample_plt)
    curve = perception_py.query(ds, curve=0)
    assert len(curve.x) == len(curve.y) == 3


def test_export_csv(perception_py, sample_plt, tmp_path):
    ds = perception_py.load_plt(sample_plt)
    out = tmp_path / "out.csv"
    perception_py.export(ds, str(out))
    assert out.exists()
