cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(cppcallppython)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${SRC_ROOT_DIR})
include_directories(${SRC_ROOT_DIR}/third-party/Python313/include)
# link_directories(D:/work/Perception/build/bin/Release)

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
message(STATUS "Compiling Source files: ${SRC_FILES}")
message(STATUS "Current Compiling directory: ${CMAKE_CURRENT_SOURCE_DIR}")

# 添加可执行文件
add_executable(cppcallppython ${SRC_FILES})
