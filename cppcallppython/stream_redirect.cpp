#include "stream_redirect.h"

/**
 * @brief Python标准输出流重定向的实现
 * 该模块实现了Python标准输出和错误输出的重定向功能
 */
namespace python_stream_rd {

/**
 * @brief 处理Python标准输出的写入操作
 * @param self Python对象指针
 * @param args Python参数元组
 * @return 返回写入的字符数量
 */
PyObject* StdStreamWrite(PyObject* self, PyObject* args) {
    std::size_t written(0);
    StdStream* selfImpl = reinterpret_cast<StdStream*>(self);
    if (selfImpl->write) {
        char* data;
        if (!PyArg_ParseTuple(args, "s", &data)) {
            return 0;
        }
        std::string str(data);
        selfImpl->write(str);
        written = str.size();
    }
    return PyLong_FromSize_t(written);
}

/**
 * @brief 处理Python标准输出的刷新操作
 * @param self Python对象指针
 * @param args Python参数元组
 * @return 空Python对象
 */
PyObject* StdStreamFlush(PyObject* self, PyObject* args) {
    // no-op
    return Py_BuildValue("");
}

/** @brief Python方法定义表，定义了可供Python调用的方法 */
PyMethodDef StdStreamMethods[] = {
    {"write", StdStreamWrite, METH_VARARGS, "sys.stdout.write"},
    {"flush", StdStreamFlush, METH_VARARGS, "sys.stdout.flush"},
    {0, 0, 0, 0} // sentinel
};

/**
 * @brief Python类型对象定义
 * 定义了一个自定义的Python类型，用于替代标准输出流
 */
PyTypeObject StdStreamType = {
    PyVarObject_HEAD_INIT(0, 0) "stream_redirect.StdStreamType", /* 类型名称 */
    sizeof(StdStream),     /* 对象基本大小 */
    0,                     /* 元素大小(不适用) */
    0,                     /* 析构函数 */
    0,                     /* 打印函数(已废弃) */
    0,                     /* 获取属性 */
    0,                     /* 设置属性 */
    0,                     /* 保留字段 */
    0,                     /* 表示函数 */
    0,                     /* 数值操作 */
    0,                     /* 序列操作 */
    0,                     /* 映射操作 */
    0,                     /* 哈希函数 */
    0,                     /* 调用函数 */
    0,                     /* 字符串转换 */
    0,                     /* 获取属性 */
    0,                     /* 设置属性 */
    0,                     /* 缓冲区操作 */
    Py_TPFLAGS_DEFAULT,    /* 类型标志 */
    "stream_redirect.StdStream objects", /* 类型文档 */
    0,                     /* 遍历函数 */
    0,                     /* 清理函数 */
    0,                     /* 富比较函数 */
    0,                     /* 弱引用列表偏移 */
    0,                     /* 迭代器函数 */
    0,                     /* 下一个迭代器函数 */
    StdStreamMethods,      /* 方法定义 */
    0,                     /* 成员定义 */
    0,                     /* 获取设置定义 */
    0,                     /* 基类 */
    0,                     /* 字典 */
    0,                     /* 描述符获取 */
    0,                     /* 描述符设置 */
    0,                     /* 字典偏移 */
    0,                     /* 初始化函数 */
    0,                     /* 分配函数 */
    PyType_GenericNew,     /* 新建函数 */
    0                      /* 释放函数 */
};

/**
 * @brief Python模块定义
 * 定义了模块的基本信息和配置
 */
PyModuleDef stream_redirect_module = {
    PyModuleDef_HEAD_INIT, "stream_redirect", 0, -1, 0,
};

/**
 * @brief 模块初始化函数
 * 完成Python类型的注册和模块的创建
 */
PyMODINIT_FUNC PyInit_stream_redirect(void) {
    if (PyType_Ready(&StdStreamType) < 0) {
        return NULL;
    }
    PyObject* m = PyModule_Create(&stream_redirect_module);
    if (m) {
        Py_INCREF(&StdStreamType);
        PyModule_AddObject(m, "stdout", reinterpret_cast<PyObject*>(&StdStreamType));
    }
    return m;
}

/**
 * @brief 设置Python的标准输出重定向
 * @param write 输出处理回调函数
 */
void SetStdOut(stdout_write_type write) {
    PyObject* stdout_old = PySys_GetObject("stdout"); // borrowed
    PyObject* stdout_new = StdStreamType.tp_new(&StdStreamType, 0, 0); // borrowed
    StdStream* impl = reinterpret_cast<StdStream*>(stdout_new);
    impl->write = write;
    PySys_SetObject("stdout", stdout_new);
    Py_XDECREF(stdout_old);
    Py_XDECREF(stdout_new);
}

/**
 * @brief 设置Python的标准错误输出重定向
 * @param write 错误输出处理回调函数
 */
void SetStdErr(stdout_write_type write) {
    PyObject* stdout_old = PySys_GetObject("stderr"); // borrowed
    PyObject* stdout_new = StdStreamType.tp_new(&StdStreamType, 0, 0); // borrowed
    StdStream* impl = reinterpret_cast<StdStream*>(stdout_new);
    impl->write = write;
    PySys_SetObject("stderr", stdout_new);
    Py_XDECREF(stdout_old);
    Py_XDECREF(stdout_new);
}

}  // namespace python_stream_rd