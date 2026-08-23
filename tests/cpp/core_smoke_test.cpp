// ===== core 层冒烟测试 =====
// 骨架阶段：验证数据模型接口、读取器注册表分派、占位实现行为。
// M1 起在此补充 .plt 解析用例。
#include "core/io/csv_reader.h"
#include "core/io/plt_reader.h"
#include "core/io/reader.h"
#include "core/model/dataset.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace perception::core;

int main()
{
    // model: DataSet / Curve 基本行为
    model::DataSet ds("smoke");
    ds.addCurve(model::Curve{"id_vs_vds", {0.0, 1.0, 2.0}, {0.1, 0.5, 1.2}});
    assert(ds.name() == "smoke");
    assert(ds.curves().size() == 1);
    assert(ds.curves().front().size() == 3);
    assert(!ds.curves().front().empty());

    // io: 注册表按扩展名分派（多格式扩展路径）
    io::registerPltReader();
    io::registerCsvReader();
    assert(io::ReaderRegistry::instance().findCurveReader("a.plt") != nullptr);
    assert(io::ReaderRegistry::instance().findCurveReader("b.csv") != nullptr);
    assert(io::ReaderRegistry::instance().findCurveReader("c.tdr") == nullptr);
    assert(io::ReaderRegistry::instance().findStructureReader("c.tdr") == nullptr);

    // io: 不存在的文件应抛异常（占位实现的安全校验）
    bool threw = false;
    try {
        io::ReaderRegistry::instance()
            .findCurveReader("no_such.plt")
            ->readCurves("no_such_file_should_exist.plt");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    std::cout << "core smoke tests passed" << std::endl;
    return 0;
}
