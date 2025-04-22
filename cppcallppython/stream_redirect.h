#pragma once
#include "Python.h"
#include <functional>
#include <string>

namespace python_stream_rd {
typedef std::function<void(std::string)> stdout_write_type;

struct StdStream {
    PyObject_HEAD
    stdout_write_type write;
};


PyMODINIT_FUNC PyInit_stream_redirect(void);

void SetStdOut(stdout_write_type write);
void SetStdErr(stdout_write_type write);
}
