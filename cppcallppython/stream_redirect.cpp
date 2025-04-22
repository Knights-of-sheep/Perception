#include "stream_redirect.h"

namespace python_stream_rd {

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

PyObject* StdStreamFlush(PyObject* self, PyObject* args) {
    // no-op
    return Py_BuildValue("");
}

PyMethodDef StdStreamMethods[] = {
    {"write", StdStreamWrite, METH_VARARGS, "sys.stdout.write"},
    {"flush", StdStreamFlush, METH_VARARGS, "sys.stdout.flush"},
    {0, 0, 0, 0} // sentinel
};

PyTypeObject StdStreamType = {
    PyVarObject_HEAD_INIT(0, 0) "stream_redirect.StdStreamType", /* tp_name */
    sizeof(StdStream), /* tp_basicsize */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    0, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    "stream_redirect.StdStream objects", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    StdStreamMethods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    0, /* tp_init */
    0, /* tp_alloc */
    PyType_GenericNew, /* tp_new */
    0 /* tp_free */
};

PyModuleDef stream_redirect_module = {
    PyModuleDef_HEAD_INIT, "stream_redirect", 0, -1, 0,
};

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

void SetStdOut(stdout_write_type write) {
    PyObject* stdout_old = PySys_GetObject("stdout"); // borrowed
    PyObject* stdout_new = StdStreamType.tp_new(&StdStreamType, 0, 0); // borrowed
    StdStream* impl = reinterpret_cast<StdStream*>(stdout_new);
    impl->write = write;
    PySys_SetObject("stdout", stdout_new);
    Py_XDECREF(stdout_old);
    Py_XDECREF(stdout_new);
}

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