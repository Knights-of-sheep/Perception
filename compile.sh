#!/bin/bash
# 获取脚本的绝对路径
SCRIPT_PATH=$(realpath "$0")
# 获取脚本所在的目录
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
echo "Script path: $SCRIPT_PATH"
echo "Script directory: $SCRIPT_DIR"

BUILD_TYPE="Debug"
# 检查是否有输入参数
if [ "$#" -eq 1 ]; then
    # 检查输入参数是否合法
    if [ "$1" == "Release" ]; then
        BUILD_TYPE="Release"
    elif [ "$1" == "Debug" ]; then
        BUILD_TYPE="Debug"
    else
        echo "Invalid build type: $1. Using default build type: Debug"
    fi
fi


cd  $SCRIPT_DIR
SRC_DIR=$SCRIPT_DIR
pwd
# 删除旧的 build 文件夹
rm -rf build

# 构建和打包函数
build_and_package() {
    BUILD_TYPE_NOW=$1
    BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE_NOW"

    # 创建构建子目录
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR

    # 运行 CMake 配置
    cmake -S $SCRIPT_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE_NOW

    # 编译项目
    cmake --build . --config $BUILD_TYPE_NOW -j32
    # 返回上级目录
    cd - > /dev/null
}

# 构建版本
build_and_package $BUILD_TYPE

# 打印完成信息
echo "Build and packaging completed. Output is in output/Debug and output/Release"
