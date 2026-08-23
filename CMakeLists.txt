cmake_minimum_required(VERSION 3.16)
project(Perception VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ==== Third-party / system dependencies (M1+ 逐步接入) ====
# find_package(Qt5 COMPONENTS Widgets REQUIRED)   # Qt 5.12.12 (C:\Qt\Qt5.12.12)
# find_package(VTK REQUIRED)                      # VTK 9.4.1 (本地安装目录)
# find_package(pybind11 CONFIG REQUIRED)          # pybind11 (M5)

# ==== Targets（M1+ 逐个打开） ====
add_executable(perception src/app/main.cpp)
target_include_directories(perception PRIVATE src)
# target_link_libraries(perception PRIVATE Qt5::Widgets VTK::ChartsCore)  # M3/M4
