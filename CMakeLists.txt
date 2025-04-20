cmake_minimum_required(VERSION 3.10)
project(Perception)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(SRC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# 添加子目录
add_subdirectory(strategy)
add_subdirectory(pybind)
add_subdirectory(cppcallppython)
