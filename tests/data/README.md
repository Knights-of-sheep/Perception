# 测试样例数据（tests/data）

对应权威文件类型目录 `src/core/io/file_type_catalog.h`（4 格式族 / 16 条目 / 23 扩展名）。
按格式族分类存放，与打开对话框过滤分组一致（VTK / SVisual / HDF5 / Curve）。

> 来源：真实文件下载自 [lorensen/VTKExamples](https://github.com/lorensen/VTKExamples)；
> 其余由 `scripts/make_test_data.py` 用 VTK 9.5.2 / h5py 生成（可重复执行重建）。

## vtk/ — VTK 全类型

| 文件 | 扩展名 | 目录条目 | 来源 |
|---|---|---|---|
| legacy.vtk | .vtk | VTK Legacy | 下载（uGridEx.vtk） |
| image.vti | .vti | VTK Image Data | 下载（vase.vti） |
| polygonal.vtp | .vtp | VTK Polygonal Data | 下载（Bunny.vtp） |
| unstructured.vtu | .vtu | VTK Unstructured Grid | 下载（Hexahedron.vtu） |
| structured_grid.vts | .vts | VTK Structured Grid | 下载（StructuredGrid.vts） |
| rectilinear.vtr | .vtr | VTK Rectilinear Grid | 下载（RectilinearGrid.vtr） |
| multiblock.vtm / .vtmb | .vtm .vtmb | VTK MultiBlock | 生成（image + sphere 两 block） |
| hypertreegrid.vth | .vth | VTK HyperTreeGrid | 生成（最小骨架，见下注） |
| overlapping_amr.vto | .vto | VTK Overlapping AMR | 生成（单层 8³ box） |
| parallel_*.pvti/.pvtp/.pvtu/.pvts/.pvtr | .pv* | VTK Parallel XML | 生成（由对应 piece 并行写出） |
| parallel_multiblock.pvtm | .pvtm | VTK Parallel XML | 生成（引用 multiblock.vtm） |
| collection.pvd | .pvd | ParaView Collection | 生成（引用 unstructured.vtu 两个时间步） |

## svisual/ — SVisual

| 文件 | 扩展名 | 目录条目 | 来源 |
|---|---|---|---|
| curve.plt | .plt | SVisual 曲线数据（已支持） | 生成（title/xlabel/ylabel + 数值列） |
| structure.tdr | .tdr | SVisual 结构数据 | **占位**（专有二进制格式，公开源无样例） |

## hdf5/ — HDF5

| 文件 | 扩展名 | 目录条目 | 来源 |
|---|---|---|---|
| data.h5 | .h5 | HDF5 数据（双用途） | 生成（h5py：curve + structure 两组） |
| data.hdf5 | .hdf5 | HDF5 数据（双用途） | 生成（同上） |

## curve/ — 曲线数据

| 文件 | 扩展名 | 目录条目 | 来源 |
|---|---|---|---|
| data.csv | .csv | CSV 曲线数据（已支持） | 生成（表头 + 两列） |
| data.dat | .dat | 通用曲线数据（双用途） | 生成（注释 + 两列） |

## 备注

- **`_0` 片段 / 子目录**：`parallel_*_0.*`、`multiblock/`、`overlapping_amr/` 是
  VTK 并行 / 复合 writer 的标准产物（master 引用外部 piece/block 文件），
  删除会导致 master 无法加载。
- **`.vth` 骨架**：VTK 9.5.2 的 Python 绑定无法构造含实际叶节点的 HyperTreeGrid，
  当前为结构合法的最小骨架；实现 HTG 读取器时可补充真实样例。
- **`.tdr` 占位**：SVisual TDR 为 Synopsys 专有二进制格式，无公开规范与样例，
  此文件仅作占位，便于打开过滤/文档核对 `.tdr` 扩展名。
