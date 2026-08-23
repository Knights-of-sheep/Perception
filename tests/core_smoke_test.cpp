// ===== core 层冒烟测试 =====
// 骨架阶段：验证数据模型接口与 PltParser 占位行为。
// M1 起在此补充 .plt 解析用例。
#include "dataset.h"
#include "plt_parser.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace perception::core;

int main()
{
    // DataSet / Curve 基本行为
    DataSet ds("smoke");
    ds.addCurve(Curve{"id_vs_vds", {0.0, 1.0, 2.0}, {0.1, 0.5, 1.2}});
    assert(ds.name() == "smoke");
    assert(ds.curves().size() == 1);
    assert(ds.curves().front().size() == 3);
    assert(!ds.curves().front().empty());

    // PltParser 占位：文件不存在应抛异常
    bool threw = false;
    try {
        PltParser::parse("no_such_file_should_exist.plt");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    std::cout << "core smoke tests passed" << std::endl;
    return 0;
}
