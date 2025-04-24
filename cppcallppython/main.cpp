#include <Python.h>
#include <iostream>
#include <string>
#include <filesystem>
#include "stream_redirect.h"
#include <frameobject.h>
#include <traceback.h>

// 定义颜色代码
const std::string RESET   = "\033[0m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN    = "\033[36m";
const std::string WHITE   = "\033[37m";

std::string PyObjectToStdString(PyObject* pyObj) {
    if (PyUnicode_Check(pyObj)) {
        const char* cStr = PyUnicode_AsUTF8(pyObj);
        if (cStr) {
            return std::string(cStr);
        } else {
            PyErr_Print();
            return std::string();
        }
    }
    PyErr_SetString(PyExc_TypeError, "Expected a string object.");
    return std::string();
}

std::string FetchPyError() {
    std::string error_msg{};
    PyObject *type = NULL, *value = NULL, *traceback = NULL;
    PyErr_Fetch(&type, &value, &traceback);
    if (type) {
        error_msg = std::string(PyExceptionClass_Name(type)) + ", ";
    }
    if (value) {
        PyObject* line = PyObject_Str(value);
        if (line) {
            std::string line_str = PyObjectToStdString(line);
            error_msg += line_str;
        }
    }
    if (traceback) {
        std::cout << "Traceback (most recent call last):" << std::endl;
        PyTracebackObject* tb = (PyTracebackObject*)traceback;
        while (tb) {
            PyCodeObject* code = PyFrame_GetCode(tb->tb_frame);
            error_msg += "\n  File \"" + PyObjectToStdString(code->co_filename) + "\", line " + std::to_string(tb->tb_lineno) + ", in " + PyObjectToStdString(code->co_name);
            Py_DECREF(code);
            tb = tb->tb_next;
        }
    }
    return error_msg;
}

void PyStdErr(const std::string& s) {
    std::cerr << RED << "Traceback (most recent call last):\n" << s << RESET << std::endl;
}

int main(int argc, char* argv[]) {
    PyImport_AppendInittab("stream_redirect", python_stream_rd::PyInit_stream_redirect);
    Py_Initialize();

    if (!Py_IsInitialized()) {
        PyErr_Print();
        std::cerr << "Failed to initialize Python." << std::endl;
        return 1;
    }

    PyObject* mainModule = PyImport_AddModule("__main__");
    PyObject* globalDict = PyModule_GetDict(mainModule);
    PyObject* localDict = PyDict_New();

    std::string applicationDirPath = std::filesystem::current_path().string();
    std::string sanitizedPath = applicationDirPath;
    size_t pos = 0;
    while ((pos = sanitizedPath.find('\\', pos)) != std::string::npos) {
        sanitizedPath.replace(pos, 1, "/");
        pos += 1;
    }

    std::string cmd = "sys.path.append(\"" + sanitizedPath + "\")";
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(cmd.c_str());
    PyRun_SimpleString("from calculatoraaaa import *");

    std::cout << "Python plugin initialized successfully." << std::endl;

    std::string input;
    while (true) {
        std::cout << CYAN << ">>> " << RESET;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        PyErr_Clear();
        PyObject* result = PyRun_String(input.c_str(), Py_single_input, globalDict, localDict);

        if (result == nullptr) {
            if (PyErr_Occurred()) {
                std::string errorMsg = FetchPyError();
                PyStdErr(errorMsg);
                PyErr_Clear();
            }
        } else {
            if (!Py_IsNone(result)) { // 检查结果是否为None
                PyObject* resultStr = PyObject_Str(result);
                if (resultStr) {
                    std::string resultString = PyObjectToStdString(resultStr);
                    if (!resultString.empty()) {
                        std::cout << RESET << resultString << std::endl; // 直接输出结果
                    }
                    Py_DECREF(resultStr);
                }
            }
            Py_DECREF(result);
        }
    }

    Py_Finalize();
    return 0;
}