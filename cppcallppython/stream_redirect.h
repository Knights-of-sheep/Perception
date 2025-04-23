#pragma once
#include "Python.h"
#include <functional>
#include <string>

/**
 * @brief Python标准输出流重定向的命名空间
 * 用于捕获和重定向Python脚本执行时的标准输出和错误输出
 */
namespace python_stream_rd {

/** @brief 定义标准输出写入函数类型，接受string参数 */
typedef std::function<void(std::string)> stdout_write_type;

/**
 * @brief Python输出流对象结构体
 * 继承自Python的对象头部，用于实现自定义的输出流对象
 */
struct StdStream {
    PyObject_HEAD                  // Python对象头部
    stdout_write_type write;       // 写入回调函数
};

/**
 * @brief Python模块初始化函数
 * @return 返回模块初始化状态
 */
PyMODINIT_FUNC PyInit_stream_redirect(void);

/**
 * @brief 设置标准输出的回调函数
 * @param write 处理标准输出的回调函数
 */
void SetStdOut(stdout_write_type write);

/**
 * @brief 设置标准错误输出的回调函数
 * @param write 处理错误输出的回调函数
 */
void SetStdErr(stdout_write_type write);
}
