cmake_minimum_required(VERSION 3.10)
project(Perception)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/toolchain.cmake)

set(SRC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# 添加子目录
add_subdirectory(strategy)
add_subdirectory(pybind)
add_subdirectory(cppcallppython)
<<<<<<< Updated upstream
add_subdirectory(vtktest)
=======
add_subdirectory(vtk_qt_test)
>>>>>>> Stashed changes
