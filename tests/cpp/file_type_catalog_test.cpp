// ===== 文件类型权威目录单测（009-supported-file-types）=====
// 覆盖 data-model.md「验证规则」（映射自 spec FR）：
//   - 覆盖宪法 4 格式族（FR-002 / SC-001）
//   - 每条目 5 字段完整非空、扩展名全库唯一（FR-003）
//   - 过滤分组覆盖全量条目（含规划中；FR-006 / FR-009-变更：展示全部、打开拦截）
//   - 大小写不敏感查找（FR-010）、.dat 双用途（FR-012）
//   - 「已支持」⇄ ReaderRegistry 一致（FR-008 / SC-002）与对称差检测（FR-011）
//
// 一致性用例依赖 ReaderRegistry 单例（进程内累积、不可重置），故按文件顺序执行：
//   1) test_consistency_catalog_only_diff  —— 只注册 .plt → 目录多出 .csv（catalogOnly）
//   2) test_consistency_matches_registry    —— 注册 .csv → 两侧一致（绿）
//   3) test_consistency_registry_only_diff  —— 额外注册 .zzz → 注册表多出 .zzz（registryOnly）
#include "core/io/csv_reader.h"
#include "core/io/file_type_catalog.h"
#include "core/io/plt_reader.h"
#include "core/io/reader.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace perception::core::io;

namespace {

std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

// 测试专用读取器（.zzz）：模拟「注册表有、目录无」的漂移方向。
class ZzzReader : public ICurveReader {
public:
    std::string formatName() const override { return "zzz"; }
    DataKind kind() const override { return DataKind::Curve; }
    std::vector<std::string> extensions() const override { return {".zzz"}; }
    std::shared_ptr<perception::core::model::IDataSet> readCurves(
        const std::string& /*path*/, const ReadOptions& /*opts*/) override {
        return nullptr;
    }
};

bool contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

}  // namespace

// 目录覆盖宪法「文件格式范围」4 格式族（FR-002 / SC-001）。
void test_catalog_covers_all_families() {
    const auto& all = FileTypeCatalog::all();
    assert(!all.empty());
    assert(all.size() >= 12);  // 契约：条目 ≥12，覆盖 4 格式族
    std::set<FileTypeFamily> families;
    for (const auto& e : all) families.insert(e.family);
    assert(families.count(FileTypeFamily::VtkLegacy) == 1);
    assert(families.count(FileTypeFamily::VtkXml) == 1);
    assert(families.count(FileTypeFamily::VtkComposite) == 1);
    assert(families.count(FileTypeFamily::VtkParallel) == 1);
    assert(families.count(FileTypeFamily::SVisual) == 1);
    assert(families.count(FileTypeFamily::Hdf5) == 1);
    assert(families.count(FileTypeFamily::Curve) == 1);
}

// 每条目 5 字段完整且非空（FR-003）。
void test_entries_have_all_fields() {
    for (const auto& e : FileTypeCatalog::all()) {
        assert(!e.formatName.empty());
        assert(!e.extensions.empty());
        for (const auto& ext : e.extensions) {
            assert(!ext.empty());
            assert(ext.front() == '.');
            assert(toLower(ext) == ext);  // 目录扩展名统一小写
        }
    }
}

// 扩展名全库唯一（FR-003）。
void test_extensions_unique() {
    std::set<std::string> seen;
    for (const auto& e : FileTypeCatalog::all()) {
        for (const auto& ext : e.extensions) {
            assert(seen.insert(toLower(ext)).second);
        }
    }
}

// 过滤分组覆盖目录全量条目（含 Planned）、按格式族分组（FR-006 / FR-009-变更：
// 展示全部类型供发现，规划中格式由加载路径按不支持拦截）；曲线数据组交叉聚合
// 其他格式族中 kind==Curve 的条目（SVisual .plt）。
void test_filter_groups_all_entries() {
    const auto groups = FileTypeCatalog::filterGroups();
    assert(groups.size() == 4);  // VTK / SVisual / HDF5 / Curve Data 全量分组
    // 目录全部扩展名 → "*"+ext 模式集合，作为全量比对基准（16 条目共 23 个扩展名）。
    std::set<std::string> expectedPatterns;
    for (const auto& e : FileTypeCatalog::all()) {
        for (const auto& ext : e.extensions) {
            expectedPatterns.insert("*" + ext);
        }
    }
    std::set<std::string> actualPatterns;
    for (const auto& g : groups) {
        actualPatterns.insert(g.patterns.begin(), g.patterns.end());
        if (g.familyKey == "SVisual") {
            assert(g.patterns == std::vector<std::string>({"*.plt", "*.tdr"}));
        } else if (g.familyKey == "Curve Data") {
            // 曲线数据组 = Curve 族（.csv/.dat）+ 其他族 kind==Curve 交叉（SVisual .plt）
            assert(g.patterns == std::vector<std::string>({"*.csv", "*.dat", "*.plt"}));
        } else if (g.familyKey == "HDF5") {
            assert(g.patterns == std::vector<std::string>({"*.h5", "*.hdf5"}));
        } else if (g.familyKey == "VTK") {
            // VTK 全族（含规划中变体）聚合到 VTK 组
            assert(g.patterns.size() == 17);
            assert(std::find(g.patterns.begin(), g.patterns.end(), "*.vtk") != g.patterns.end());
            assert(std::find(g.patterns.begin(), g.patterns.end(), "*.pvd") != g.patterns.end());
        } else {
            assert(false);  // 不允许出现其他分组
        }
    }
    // 分组模式与目录全量扩展名一一对应（无缺失、无多余）
    assert(actualPatterns == expectedPatterns);
    // 规划中扩展名出现在过滤分组（供发现），同时状态仍为 Planned（打开拦截依据）
    assert(FileTypeCatalog::findByExtension(".tdr")->status == FileTypeStatus::Planned);
    assert(FileTypeCatalog::findByExtension(".vtk")->status == FileTypeStatus::Planned);
    assert(FileTypeCatalog::findByExtension(".dat")->status == FileTypeStatus::Planned);
    assert(FileTypeCatalog::findByExtension(".h5")->status == FileTypeStatus::Planned);
}

// supported() 恰为当前可打开格式（.plt/.csv）（FR-008 的目录侧）。
void test_supported_set() {
    const auto sup = FileTypeCatalog::supported();
    assert(sup.size() == 2);
    std::set<std::string> exts;
    for (const auto& e : sup) exts.insert(e.extensions.begin(), e.extensions.end());
    assert(exts == std::set<std::string>({".plt", ".csv"}));
}

// 大小写不敏感查找（FR-010）。
void test_find_by_extension_case_insensitive() {
    const auto* plt = FileTypeCatalog::findByExtension(".PLT");
    assert(plt != nullptr);
    assert(plt->formatName == "SVisual 曲线数据");
    assert(plt->status == FileTypeStatus::Supported);
    assert(FileTypeCatalog::findByExtension(".Plt") == plt);
    assert(FileTypeCatalog::findByExtension(".plt") == plt);
    assert(FileTypeCatalog::findByExtension(".xyz") == nullptr);
}

// 多用途扩展名：.dat 标记为 Both（FR-012）。
void test_dat_is_both() {
    const auto* dat = FileTypeCatalog::findByExtension(".dat");
    assert(dat != nullptr);
    assert(dat->kind == FileTypeKind::Both);
    assert(dat->status == FileTypeStatus::Planned);
}

// 一致性（方向一 catalogOnly）：目录「已支持」有、注册表无。
// 本用例先于全量注册用例执行：此时仅注册 .plt，目录多出的 .csv 应被报告。
void test_consistency_catalog_only_diff() {
    registerPltReader();
    const auto report = FileTypeCatalog::inconsistencies();
    assert(contains(report.catalogOnly, ".csv"));
    assert(report.registryOnly.empty());
}

// 一致性（绿）：目录「已支持」⇄ ReaderRegistry 一一对应（FR-008 / SC-002）。
void test_consistency_matches_registry() {
    registerCsvReader();  // 注册表 = {.plt, .csv}，与目录一致
    const auto report = FileTypeCatalog::inconsistencies();
    assert(report.catalogOnly.empty());
    assert(report.registryOnly.empty());
    assert(report.empty());
}

// 一致性（方向二 registryOnly）：注册表有、目录无（FR-011）。
void test_consistency_registry_only_diff() {
    ReaderRegistry::instance().registerReader(std::make_shared<ZzzReader>());
    const auto report = FileTypeCatalog::inconsistencies();
    assert(contains(report.registryOnly, ".zzz"));
    assert(report.catalogOnly.empty());  // .plt/.csv 均已注册
}

int main() {
    test_catalog_covers_all_families();
    test_entries_have_all_fields();
    test_extensions_unique();
    test_filter_groups_all_entries();
    test_supported_set();
    test_find_by_extension_case_insensitive();
    test_dat_is_both();
    test_consistency_catalog_only_diff();
    test_consistency_matches_registry();
    test_consistency_registry_only_diff();
    return 0;
}
