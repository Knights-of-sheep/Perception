#!/usr/bin/env python3
"""生成 tests/data 下各格式样例（009 文件类型目录的测试夹具）。

单一事实来源：src/core/io/file_type_catalog.h（4 格式族 / 16 条目 / 23 扩展名）。
真实文件（legacy.vtk / image.vti / polygonal.vtp / unstructured.vtu /
structured_grid.vts / rectilinear.vtr）已从 lorensen/VTKExamples 下载，
本脚本生成无法从公开源下载的格式并统一校验。

目录结构（按格式族分类，与 file_type_catalog filterGroups 一致）：
    tests/data/
      vtk/        .vtk .vti .vtp .vtu .vts .vtr .vtm .vtmb .vth .vto .pv* .pvd
      svisual/    .plt .tdr
      hdf5/       .h5 .hdf5
      curve/      .csv .dat

用法：python scripts/make_test_data.py
"""
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "tests" / "data"

RESULTS = []


def note(ok, msg):
    RESULTS.append((ok, msg))
    print(("[OK]  " if ok else "[WARN]") + " " + msg, flush=True)


def write_text(path, text):
    path.write_text(text, encoding="utf-8", newline="\n")
    return path


# ---------------------------------------------------------------- VTK 生成
def gen_vtk():
    import vtk

    vtkdir = DATA / "vtk"
    vtkdir.mkdir(parents=True, exist_ok=True)

    # --- 复合数据 .vtm / .vtmb（MultiBlock：image + polydata 两个块）---
    img = vtk.vtkImageData()
    img.SetDimensions([5, 5, 5])
    img.SetOrigin([0.0, 0.0, 0.0])
    img.SetSpacing([1.0, 1.0, 1.0])
    sph = vtk.vtkSphereSource()
    sph.SetThetaResolution(16)
    sph.SetPhiResolution(16)
    sph.Update()
    mb = vtk.vtkMultiBlockDataSet()
    mb.SetNumberOfBlocks(2)
    mb.SetBlock(0, img)
    mb.SetBlock(1, sph.GetOutput())
    for ext in (".vtm", ".vtmb"):
        w = vtk.vtkXMLMultiBlockDataWriter()
        w.SetFileName(str(vtkdir / ("multiblock" + ext)))
        w.SetInputData(mb)
        note(bool(w.Write()), f"vtk/multiblock{ext}")

    # --- HyperTreeGrid .vth（VTK 9.5.2 Python 无法构造非空 HTG，输出合法骨架）---
    try:
        htg = vtk.vtkHyperTreeGrid()
        htg.SetBranchFactor(2)
        htg.SetTransposedRootIndexing(False)
        for name, vals in (("x", (0.0, 1.0, 2.0)), ("y", (0.0, 1.0, 2.0)), ("z", (0.0, 1.0))):
            a = vtk.vtkDoubleArray()
            a.SetName(name)
            for v in vals:
                a.InsertNextValue(v)
            getattr(htg, "Set%sCoordinates" % name.capitalize())(a)
        mask = vtk.vtkBitArray()
        mask.SetNumberOfValues(4)
        for i in range(4):
            mask.SetValue(i, 1)
        htg.SetMask(mask)
        htg.Initialize()
        w = vtk.vtkXMLHyperTreeGridWriter()
        w.SetFileName(str(vtkdir / "hypertreegrid.vth"))
        w.SetInputData(htg)
        note(bool(w.Write()), "vtk/hypertreegrid.vth")
    except Exception as e:
        note(False, f"vtk/hypertreegrid.vth 生成失败：{e}")

    # --- Overlapping AMR .vto（单层单 box 的 vtkUniformGrid）---
    try:
        amr = vtk.vtkOverlappingAMR()
        amr.Initialize(1, [1])
        ug = vtk.vtkUniformGrid()
        ug.SetDimensions([8, 8, 8])
        ug.SetOrigin([0.0, 0.0, 0.0])
        ug.SetSpacing([1.0, 1.0, 1.0])
        arr = vtk.vtkFloatArray()
        arr.SetName("value")
        arr.SetNumberOfTuples(8 * 8 * 8)
        for i in range(arr.GetNumberOfTuples()):
            arr.SetValue(i, float(i))
        ug.GetPointData().SetScalars(arr)
        amr.SetOrigin([0.0, 0.0, 0.0])
        amr.SetDataSet(0, 0, ug)
        amr.SetRefinementRatio(0, 2)
        w = vtk.vtkXMLUniformGridAMRWriter()
        w.SetFileName(str(vtkdir / "overlapping_amr.vto"))
        w.SetInputData(amr)
        note(bool(w.Write()), "vtk/overlapping_amr.vto")
    except Exception as e:
        note(False, f"vtk/overlapping_amr.vto 生成失败：{e}")

    # --- 并行 XML .pv*（用真实 writer 生成 master + piece，保证可读）---
    par_inputs = [
        ("parallel_image.pvti", "image.vti", vtk.vtkXMLImageDataReader, vtk.vtkXMLPImageDataWriter),
        ("parallel_polydata.pvtp", "polygonal.vtp", vtk.vtkXMLPolyDataReader, vtk.vtkXMLPPolyDataWriter),
        ("parallel_unstructured.pvtu", "unstructured.vtu", vtk.vtkXMLUnstructuredGridReader, vtk.vtkXMLPUnstructuredGridWriter),
        ("parallel_structured.pvts", "structured_grid.vts", vtk.vtkXMLStructuredGridReader, vtk.vtkXMLPStructuredGridWriter),
        ("parallel_rectilinear.pvtr", "rectilinear.vtr", vtk.vtkXMLRectilinearGridReader, vtk.vtkXMLPRectilinearGridWriter),
    ]
    for fname, src, rcls, wcls in par_inputs:
        try:
            rd = rcls()
            rd.SetFileName(str(vtkdir / src))
            rd.Update()
            w = wcls()
            w.SetFileName(str(vtkdir / fname))
            w.SetInputData(rd.GetOutput())
            w.SetNumberOfPieces(1)
            note(bool(w.Write()), f"vtk/{fname} <- {src}")
        except Exception as e:
            note(False, f"vtk/{fname} 生成失败：{e}")

    # --- 并行 MultiBlock .pvtm（master 引用 multiblock.vtm，避免 writer 崩溃）---
    pvtm = (
        '<?xml version="1.0"?>\n'
        '<VTKFile type="vtkMultiBlockDataSet" version="1.0" byte_order="LittleEndian">\n'
        '  <vtkMultiBlockDataSet>\n'
        '    <DataSet index="0" file="multiblock.vtm"/>\n'
        '  </vtkMultiBlockDataSet>\n'
        '</VTKFile>\n'
    )
    write_text(vtkdir / "parallel_multiblock.pvtm", pvtm)
    note(True, "vtk/parallel_multiblock.pvtm -> multiblock.vtm")

    # --- ParaView Collection .pvd（引用同一 piece 的两个时间步）---
    pvd = (
        '<?xml version="1.0"?>\n'
        '<VTKFile type="Collection" version="0.1" byte_order="LittleEndian">\n'
        '  <Collection>\n'
        '    <DataSet timestep="0" group="" part="0" file="unstructured.vtu"/>\n'
        '    <DataSet timestep="1" group="" part="0" file="unstructured.vtu"/>\n'
        '  </Collection>\n'
        '</VTKFile>\n'
    )
    write_text(vtkdir / "collection.pvd", pvd)
    note(True, "vtk/collection.pvd -> unstructured.vtu")


# ------------------------------------------------------------ HDF5 生成
def gen_hdf5():
    import numpy as np
    import h5py

    d = DATA / "hdf5"
    d.mkdir(parents=True, exist_ok=True)
    t = np.linspace(0.0, 10.0, 101)
    v = np.sin(2.0 * np.pi * t / 5.0)
    pts = np.random.default_rng(0).random((12, 3)).astype(np.float32)
    conn = np.arange(24).reshape(8, 3).astype(np.int32)
    for fname in ("data.h5", "data.hdf5"):
        try:
            with h5py.File(d / fname, "w") as f:
                f.attrs["sample"] = "curve + structure (kind=Both)"
                g = f.create_group("curve")
                g.create_dataset("time", data=t)
                g.create_dataset("voltage", data=v)
                s = f.create_group("structure")
                s.create_dataset("points", data=pts)
                s.create_dataset("connectivity", data=conn)
            note(True, f"hdf5/{fname}")
        except Exception as e:
            note(False, f"hdf5/{fname} 生成失败：{e}")


# ------------------------------------------------------- SVisual / Curve 生成
def gen_text_formats():
    plt = (
        'title = "nmos IdVg transfer"\n'
        'xlabel = "Vgs (V)"\n'
        'ylabel = "Id (A)"\n'
        '0.0 0.0\n'
        '0.2 0.0\n'
        '0.4 0.010\n'
        '0.6 0.048\n'
        '0.8 0.121\n'
        '1.0 0.246\n'
    )
    write_text(DATA / "svisual" / "curve.plt", plt)
    note(True, "svisual/curve.plt")

    # .tdr：SVisual 结构数据为专有二进制格式，公开源无样例；此处为占位
    (DATA / "svisual" / "structure.tdr").write_bytes(
        b"SVisual TDR placeholder - proprietary binary, no public sample\n" + b"\x00" * 96
    )
    note(True, "svisual/structure.tdr (占位)")

    csv_lines = ["time,voltage"] + ["%.1f,%.3f" % (i * 0.1, round(0.246 * min(i, 10) / 10, 3)) for i in range(11)]
    write_text(DATA / "curve" / "data.csv", "\n".join(csv_lines) + "\n")
    note(True, "curve/data.csv")

    dat_lines = ["# time(s) voltage(V)"] + ["%.1f %.3f" % (i * 0.1, round(0.246 * min(i, 10) / 10, 3)) for i in range(11)]
    write_text(DATA / "curve" / "data.dat", "\n".join(dat_lines) + "\n")
    note(True, "curve/data.dat")


# ------------------------------------------------------------ 校验回读
def verify():
    import vtk
    import h5py

    vtkdir = DATA / "vtk"
    vtk_cases = [
        ("legacy.vtk", vtk.vtkDataSetReader),
        ("image.vti", vtk.vtkXMLImageDataReader),
        ("polygonal.vtp", vtk.vtkXMLPolyDataReader),
        ("unstructured.vtu", vtk.vtkXMLUnstructuredGridReader),
        ("structured_grid.vts", vtk.vtkXMLStructuredGridReader),
        ("rectilinear.vtr", vtk.vtkXMLRectilinearGridReader),
        ("multiblock.vtm", vtk.vtkXMLMultiBlockDataReader),
        ("multiblock.vtmb", vtk.vtkXMLMultiBlockDataReader),
        ("hypertreegrid.vth", vtk.vtkXMLHyperTreeGridReader),
        ("overlapping_amr.vto", vtk.vtkXMLUniformGridAMRReader),
        ("parallel_image.pvti", vtk.vtkXMLPImageDataReader),
        ("parallel_polydata.pvtp", vtk.vtkXMLPPolyDataReader),
        ("parallel_unstructured.pvtu", vtk.vtkXMLPUnstructuredGridReader),
        ("parallel_structured.pvts", vtk.vtkXMLPStructuredGridReader),
        ("parallel_rectilinear.pvtr", vtk.vtkXMLPRectilinearGridReader),
    ]
    for fname, rcls in vtk_cases:
        path = vtkdir / fname
        if not path.exists():
            note(False, f"校验 {fname}：文件缺失")
            continue
        try:
            r = rcls()
            if hasattr(r, "SetFileName"):
                r.SetFileName(str(path))
            r.Update()
            note(True, f"校验 {fname} 可读")
        except Exception as e:
            note(False, f"校验 {fname} 读取异常：{e}")

    # .pvd / .pvtm 无独立 reader（vtkXMLCollectionReader 已移除），只校验 XML 良构
    for xmlf in ("collection.pvd", "parallel_multiblock.pvtm"):
        try:
            ET.parse(vtkdir / xmlf)
            note(True, f"校验 {xmlf} XML 良构")
        except Exception as e:
            note(False, f"校验 {xmlf} 失败：{e}")

    for fname in ("data.h5", "data.hdf5"):
        path = DATA / "hdf5" / fname
        if not path.exists():
            note(False, f"校验 {fname}：文件缺失")
            continue
        try:
            with h5py.File(path, "r") as f:
                note(True, f"校验 {fname} 可读（groups: {sorted(f.keys())}）")
        except Exception as e:
            note(False, f"校验 {fname} 失败：{e}")


def main():
    for d in ("vtk", "svisual", "hdf5", "curve"):
        (DATA / d).mkdir(parents=True, exist_ok=True)
    gen_vtk()
    gen_hdf5()
    gen_text_formats()
    verify()
    bad = [r for r in RESULTS if not r[0]]
    print("\n=== 汇总：%d 项，%d 警告 ===" % (len(RESULTS), len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
