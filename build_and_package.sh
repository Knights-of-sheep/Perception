#!/bin/bash
# 获取脚本的绝对路径
SCRIPT_PATH=$(realpath "$0")
# 获取脚本所在的目录
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
echo "Script path: $SCRIPT_PATH"
echo "Script directory: $SCRIPT_DIR"
cd  $SCRIPT_DIR
SRC_DIR=$SCRIPT_DIR
pwd
# 删除旧的 build 文件夹
rm -rf build
rm -rf output

# 构建和打包函数
build_and_package() {
    BUILD_TYPE=$1
    BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE"
    OUTPUT_DIR="$SCRIPT_DIR/output"

    # 创建构建子目录
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR

    # 运行 CMake 配置
    cmake -S $SCRIPT_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE

    # 编译项目
    cmake --build . --config $BUILD_TYPE -j32

    # 创建输出目录
    mkdir -p $OUTPUT_DIR
    cp -rf $BUILD_DIR/bin $OUTPUT_DIR
    cp -rf $BUILD_DIR/lib $OUTPUT_DIR
    cp -rf $SCRIPT_DIR/third-party/VTK-9-4-1-install/debug/bin/*.dll $OUTPUT_DIR/bin/Debug
    cp -rf $SCRIPT_DIR/third-party/VTK-9-4-1-install/release/bin/*.dll $OUTPUT_DIR/bin/Release

    # 返回上级目录
    cd - > /dev/null
}
# 构建和打包 Release 版本
build_and_package "Debug"
# 构建和打包 Release 版本
build_and_package "Release"

# 打印完成信息
echo "Build and packaging completed. Output is in output/Debug and output/Release"
