// ===== 命令层骨架实现：CommandServiceImpl（006 US4）=====
// ICommandService 本期实现边界：
//   - createWindow：真实命令（经 IWindowFactory 派发，打通验证）
//   - load/transform/query/export：M5 占位，抛 NotImplementedError
// 真实命令逻辑（数据 CRUD、对标 SVisual）严格留待 M5 命令驱动层。
#pragma once

#include "python/api/i_command_service.h"

#include <exception>
#include <string>

namespace perception {
namespace python {

// 占位命令抛出的 C++ 异常：在 perception_py 模块中用
// py::register_exception<NotImplementedError>(m, "NotImplementedError",
//   PyExc_NotImplementedError) 注册为 Python 内建 NotImplementedError 的子类，
// 使 pytest.raises(NotImplementedError) 可直接捕获。
class NotImplementedError : public std::exception {
public:
    explicit NotImplementedError(std::string msg) : msg_(std::move(msg)) {}
    const char* what() const noexcept override { return msg_.c_str(); }

private:
    std::string msg_;
};

class CommandServiceImpl : public ICommandService {
public:
    std::string createWindow(const std::string& title) override;
    py::object supportedFormats() override;
    py::object load(const std::string& path) override;
    py::object transform(py::object ds, const std::string& name,
                         py::kwargs kwargs) override;
    py::object query(py::object ds, py::kwargs kwargs) override;
    py::object exportData(py::object ds, const std::string& path) override;
};

}  // namespace python
}  // namespace perception
