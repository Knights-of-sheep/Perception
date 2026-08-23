# ===== src/ui：界面层（M4 启用）=====
# 由顶层 CMakeLists 在 PERCEPTION_BUILD_GUI=ON 时 add_subdirectory。
# 职责：Qt5 Widgets + Dock + QSS 深色主题；只持有 ICurveChart 抽象，不直接操作 VTK。
# 预期 target：perception_ui（STATIC，链接 Qt5::Widgets + perception_render + perception_core）。
# 界面事实源：docs/design/mockups/。
