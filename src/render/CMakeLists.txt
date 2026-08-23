# ===== src/render：渲染层（M3 启用）=====
# 由顶层 CMakeLists 在 PERCEPTION_BUILD_GUI=ON 时 add_subdirectory。
# 职责：VTK Charts 2D 曲线（vtkChartXY）+ 色板。
# 预期 target：perception_render（STATIC，链接 VTK::ChartsCore + perception_core）。
# 接入点：顶层 find_package(VTK ... COMPONENTS ChartsCore ...) 已就绪。
