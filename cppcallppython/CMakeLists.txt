cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(cppcallppython)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${SRC_ROOT_DIR})
include_directories(${SRC_ROOT_DIR}/third-party/python3.13.3/include)
link_directories(${SRC_ROOT_DIR}/third-party/python3.13.3/libs)
link_directories(${SRC_ROOT_DIR}/third-party/python3.13.3)

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
message(STATUS "Compiling Source files: ${SRC_FILES}")
message(STATUS "Current Compiling directory: ${CMAKE_CURRENT_SOURCE_DIR}")

# Sets Python library and DLL paths based on build configuration
# For Debug builds:
#   - Libraries: python313_d.lib, python3_d.lib, python3t_d.lib, python313t_d.lib 
#   - DLLs: python313_d.dll, python3t_d.dll
# For Release builds:
#   - Libraries: python313.lib, python3.lib, python3t.lib, python313t.lib
#   - DLLs: python313.dll, python3t.dll
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PYTHON_LIBS python313_d.lib python3_d.lib python3t_d.lib python313t_d.lib)
    set(PYTHON_DLL python313_d.dll python3t_d.dll)
else()
    set(PYTHON_LIBS python313.lib python3.lib python3t.lib python313t.lib)
    set(PYTHON_DLL python313.dll python3t.dll)
endif()

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})
# 链接外部库
target_link_libraries(${PROJECT_NAME} PRIVATE ${PYTHON_DLL} ${PYTHON_LIBS})