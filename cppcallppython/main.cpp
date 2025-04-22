// #include "D:/work/Perception/third-party/python3.13.3/include/Python.h"
// #include <iostream>
// #include <string>
// #include <filesystem>

// int main(int argc, char* argv[]) {
//     // PyImport_AppendInittab("stream_redirect", QX::Ipy::stream::PyInit_stream_redirect);
//     Py_Initialize();
//     PyObject* pMainModule = nullptr;
//     PyObject* GlobalDict = nullptr;
//     if (!Py_IsInitialized()) {
//         PyErr_Print();
//         std::cerr << "initialize python plugin failed." << std::endl;
//     } else {
//         PyRun_SimpleString("import sys");
//         std::string applicationDirPath = std::filesystem::current_path().string();
//         std::string cmd = "sys.path.append(\"" + applicationDirPath + "\")";
//         PyRun_SimpleString(cmd.c_str());
//         PyRun_SimpleString("from calculatoraaaa import *");

//         pMainModule = PyImport_AddModule("__main__");

//         // 重定向模块
//         // PyImport_ImportModule("stream_redirect");
//         // 获取上下文Global字典
//         GlobalDict = PyModule_GetDict(pMainModule);
//         // 创建上下文Local字典
//         // PyObject* LocalDict = PyDict_New();

//         // writer_ = [&](std::string s) {
//         //     // 重定向 DoCmdRunLine 标准输出
//         //     // 注意：Python3，PyRun_SimpleString 的默认调用不会再输出到标准输出
//         //     PyStdOut(s);
//         // };
//         // error_ = [&](std::string s) {
//         //     // 意图是重定向 DoCmdRunLine 的错误输出
//         //     // 但是发现在Python3里，错误输出也是由执行结果一起返回了，但是这里依然做一次重定向，以防万一
//         //     PyStdErr(s);
//         // };
//         // qx::stream::setStdout(writer_);
//         // qx::stream::setStdErr(error_);
//     }
//     while (true)
//     {
//         /* code */
//         std::string input;
//         std::getline(std::cin, input);
//         PyRun_String(input.c_str(), Py_file_input, GlobalDict, GlobalDict);
//     }
    
//     return 0;
// }

#include <Python.h>
#include <iostream>

int main() {
    // 初始化 Python 解释器
    std::cout << "Initializing Python interpreter..." << std::endl;
    Py_Initialize();
    if (Py_IsInitialized()) {
        // 执行 Python 代码
        PyRun_SimpleString("print('Hello from embedded Python!')");
        // 关闭 Python 解释器
        Py_Finalize();
    }
    std::cout << "Python interpreter finalized." << std::endl;
    return 0;
}