#include <iostream>
#include <variant>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <stdexcept>

// 定义一个泛型变量类型，可以存储 double、bool、int、std::string 四种类型
using genericVar = std::variant<double, bool, int, std::string>;

class TdrParser
{
public:
    // 打印 genericVar 的值，根据实际类型分支
    bool printGenericVar(genericVar x)
    {
        std::cout << "printGenericVar called " << std::endl;
        if (std::holds_alternative<std::string>(x))
        {
            std::cout << "printGenericVar called string with: " << std::get<std::string>(x) << std::endl;
        }
        if (std::holds_alternative<double>(x))
        {
            std::cout << "printGenericVar called double with: " << std::get<double>(x) << std::endl;
        }
        if (std::holds_alternative<int>(x))
        {
            std::cout << "printGenericVar called int with: " << std::get<int>(x) << std::endl;
        }
        if (std::holds_alternative<bool>(x))
        {
            std::cout << "printGenericVar called bool with: " << std::get<bool>(x) << std::endl;
        }
        return true;
    }
    // 打印 int 类型
    bool printInt(int x)
    {
        std::cout << "printInt called with: " << x << std::endl;
        return true;
    }

    // 打印字符串
    bool printString(const std::string &str)
    {
        std::cout << "printString called with: " << str << std::endl;
        return true;
    }

    // 打印 int 和 string
    bool printTwo(int a, const std::string &str)
    {
        std::cout << "printTwo called with: " << a << std::endl;
        std::cout << "printTwo called with: " << str << std::endl;
        return true;
    }
};

// 操作类型枚举
enum operateType
{
    TYPE_A = 0,
    TYPE_B,
    TYPE_C,
    TYPE_D
};

// 定义一个泛型函数类型，可以存储四种不同签名的 std::function
using GenericFunction = std::variant<
    std::function<bool(TdrParser *, int)>,
    std::function<bool(TdrParser *, genericVar)>,
    std::function<bool(TdrParser *, int, std::string)>,
    std::function<bool(TdrParser *, const std::string &)>>;

class LinkOperate
{
public:
    LinkOperate()
    {
        // 将不同签名的 lambda 包装为 std::function 并存入 operatemap
        operatemap[TYPE_D] = std::function<bool(TdrParser *, genericVar)>{[](TdrParser *p, genericVar x)
                                                                          {
                                                                              return p->printGenericVar(x);
                                                                          }};
        operatemap[TYPE_A] = std::function<bool(TdrParser *, int)>{[](TdrParser *p, int x)
                                                                   {
                                                                       return p->printInt(x);
                                                                   }};
        operatemap[TYPE_B] = std::function<bool(TdrParser *, const std::string &)>{[](TdrParser *p, const std::string &str)
                                                                                   {
                                                                                       return p->printString(str);
                                                                                   }};
        operatemap[TYPE_C] = std::function<bool(TdrParser *, int, std::string)>{[](TdrParser *p, int a, const std::string &str)
                                                                                {
                                                                                    return p->printTwo(a, str);
                                                                                }};
    }

    // executeFunction 是一个可变参数模板函数，可以接受任意参数
    template <typename... Args>
    bool executeFunction(operateType type, Args &&...args)
    {
        // 检查操作类型是否存在于 operatemap
        if (!operatemap.count(type))
        {
            return false; // 不存在则返回 false
        }
        // 获取对应的泛型函数
        auto func = operatemap[type];

        try
        {
            // 使用 std::visit 调用 variant 中存储的实际函数
            auto result = std::visit(
                [&](auto &&f) // lambda 捕获实际的 std::function
                {
                    using FuncType = std::decay_t<decltype(f)>; // 获取函数类型
                    // 判断该函数能否用传入参数调用
                    if constexpr (std::is_invocable_v<FuncType, Args...>)
                    {
                        // 如果可以调用，则转发参数并调用
                        return f(std::forward<Args>(args)...);
                    }
                    else
                    {
                        // 如果不能调用，返回 false
                        return false;
                    }
                },
                func);     // func 是一个 std::variant
            return result; // 返回调用结果
        }
        catch (const std::exception &e)
        {
            // 捕获并打印异常信息
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

private:
    // 操作类型到泛型函数的映射表
    std::unordered_map<operateType, GenericFunction> operatemap;
};

int main()
{
    // 创建 TdrParser 实例
    TdrParser *unit = new TdrParser();
    // 创建 LinkOperate 实例
    LinkOperate *funcexecutor = new LinkOperate();

    // 调用 TYPE_A 类型函数，参数为 unit 和 int
    bool res = funcexecutor->executeFunction(TYPE_A, unit, 42);
    // 调用 TYPE_B 类型函数，参数为 unit 和 string
    bool res2 = funcexecutor->executeFunction(TYPE_B, unit, std::string("Hello, World!"));
    // 调用 TYPE_C 类型函数，参数为 unit、int 和 string
    bool res3 = funcexecutor->executeFunction(TYPE_C, unit, 55, std::string("hhhhhh"));
    // 调用 TYPE_D 类型函数，参数为 unit 和 genericVar
    bool res4 = funcexecutor->executeFunction(TYPE_D, unit, genericVar(42));

    // 打印所有调用结果
    std::cout << "res: " << res << ", res2: " << res2 << ", res3: " << res3 << ", res4: " << res4 << std::endl;

    // 释放内存
    delete unit;
    delete funcexecutor;

    return 0;
}