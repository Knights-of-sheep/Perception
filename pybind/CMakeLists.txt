cmake_minimum_required(VERSION 3.10)
project(pybind_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${SRC_ROOT_DIR})

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
message(STATUS "Compiling Source files: ${SRC_FILES}")
message(STATUS "Current Compiling directory: ${CMAKE_CURRENT_SOURCE_DIR}")

# 添加pybind11路径
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third-party/pybind11-2.13.6 pybind11_build)

# 查找Python包
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# 创建Python模块
pybind11_add_module(calculatoraaaa SHARED ${SRC_FILES})

# 设置后缀名
set_target_properties(calculatoraaaa PROPERTIES SUFFIX ".pyd")