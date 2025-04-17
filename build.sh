#!/bin/bash
# 获取脚本的绝对路径
SCRIPT_PATH=$(realpath "$0")
# 获取脚本所在的目录
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
echo "Script path: $SCRIPT_PATH"
echo "Script directory: $SCRIPT_DIR"
cd  $SCRIPT_DIR
pwd

# 删除旧的 output 文件夹
rm -rf $SCRIPT_DIR/output
mkdir $SCRIPT_DIR/output

sh ./strategy/build_and_package.sh