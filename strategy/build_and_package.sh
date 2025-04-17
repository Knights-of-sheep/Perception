#!/bin/bash
# 获取脚本的绝对路径
SCRIPT_PATH=$(realpath "$0")
# 获取脚本所在的目录
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
echo "Script path: $SCRIPT_PATH"
echo "Script directory: $SCRIPT_DIR"
cd  $SCRIPT_DIR
pwd
# 删除旧的 build 文件夹
rm -rf build

# 设置脚本出错时退出
# set -e

# 构建和打包函数
build_and_package() {
    BUILD_TYPE=$1
    BUILD_SUBDIR="build/$BUILD_TYPE"
    OUTPUT_DIR="../output/strategy/$BUILD_TYPE"

    mkdir build
    # 创建构建子目录
    mkdir -p "$BUILD_SUBDIR"
    cd "$BUILD_SUBDIR"

    # 清理构建子目录中的旧文件
    rm -rf *

    # 运行 CMake 配置
    cmake ../.. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

    # 编译项目
    cmake --build . -j32

    # 查找生成的可执行文件（递归搜索）
    EXECUTABLE=$(find . -type f -name "strategy*" | head -n 1)

    if [ -z "$EXECUTABLE" ]; then
        echo "Error: Executable file not found for $BUILD_TYPE build."
        exit 1
    fi

    # 创建输出目录
    mkdir -p "../../$OUTPUT_DIR"

    # 将可执行文件复制到输出目录
    cp "$EXECUTABLE" "../../$OUTPUT_DIR"

    # 返回上级目录
    cd - > /dev/null
}

# 构建和打包 Debug 版本
# build_and_package "Debug"

# 构建和打包 Release 版本
build_and_package "Release"

# 打印完成信息
echo "Build and packaging completed. Output is in output/Debug and output/Release"
