#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    int subtract(int a, int b) {
        return a - b;
    }

    int multiply(int a, int b) {
        return a * b;
    }

    int divide(int a, int b) {
        if (b == 0) throw std::invalid_argument("Division by zero");
        return a / b;
    }
};

PYBIND11_MODULE(calculatoraaaa, m) {
    py::class_<Calculator>(m, "Calculator")
        .def(py::init<>())
        .def("add", &Calculator::add)
        .def("subtract", &Calculator::subtract)
        .def("multiply", &Calculator::multiply)
        .def("divide", &Calculator::divide);
}