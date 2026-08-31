// ===== 文件类型权威目录访问器实现（009）=====
// 纯函数、无 UI/VTK 依赖（Layered Core）：过滤分组 / 一致性校验均可无头单测。
#include "core/io/file_type_catalog.h"

#include "core/io/reader.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <set>
#include <string>

namespace perception {
namespace core {
namespace io {

namespace {

// 扩展名小写化（目录统一小写，匹配不区分大小写，FR-010）。
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// 格式族 → 打开过滤分组键（data-model：VTK 四族→"VTK"；SVisual→"SVisual"；
// Hdf5→"HDF5"；Curve→"Curve Data"）。
const char* familyKey(FileTypeFamily family) {
    switch (family) {
        case FileTypeFamily::VtkLegacy:
        case FileTypeFamily::VtkXml:
        case FileTypeFamily::VtkComposite:
        case FileTypeFamily::VtkParallel:
            return "VTK";
        case FileTypeFamily::SVisual:
            return "SVisual";
        case FileTypeFamily::Hdf5:
            return "HDF5";
        case FileTypeFamily::Curve:
            return "Curve Data";
    }
    return "";
}

}  // namespace

const std::vector<FileTypeEntry>& FileTypeCatalog::all() {
    static const std::vector<FileTypeEntry> catalog(
        std::begin(kFileTypeCatalog), std::end(kFileTypeCatalog));
    return catalog;
}

std::vector<FileTypeEntry> FileTypeCatalog::supported() {
    std::vector<FileTypeEntry> result;
    for (const auto& e : all()) {
        if (e.status == FileTypeStatus::Supported) {
            result.push_back(e);
        }
    }
    return result;
}

const FileTypeEntry* FileTypeCatalog::findByExtension(const std::string& ext) {
    const std::string key = toLower(ext);
    for (const auto& e : all()) {
        for (const auto& candidate : e.extensions) {
            if (candidate == key) {
                return &e;
            }
        }
    }
    return nullptr;
}

std::vector<FileTypeFilterGroup> FileTypeCatalog::filterGroups() {
    // 全量条目（含 Planned）：过滤覆盖程序支持范围内的全部类型（产品裁决覆盖
    // FR-009 原文），供对话框内发现与核对；规划中格式选中后由加载路径按
    // 「不支持」提示，不进入读取流程。
    std::vector<FileTypeFilterGroup> groups;
    for (const auto& e : all()) {
        const char* key = familyKey(e.family);
        auto it = std::find_if(groups.begin(), groups.end(),
                               [&](const FileTypeFilterGroup& g) {
                                   return g.familyKey == key;
                               });
        if (it == groups.end()) {
            groups.push_back(FileTypeFilterGroup{key, {}});
            it = std::prev(groups.end());
        }
        for (const auto& ext : e.extensions) {
            it->patterns.push_back("*" + ext);
        }
    }
    // 交叉聚合：曲线数据组追加其他格式族中数据种类为「曲线」的条目
    // （当前为 SVisual .plt），便于按数据种类检索曲线文件（FR-003 数据种类维度）。
    // 单一来源：仍由目录派生，禁止手抄；Curve 族自身条目（.csv/.dat）已在组内。
    for (auto& g : groups) {
        if (g.familyKey != "Curve Data") {
            continue;
        }
        for (const auto& e : all()) {
            if (e.family == FileTypeFamily::Curve || e.kind != FileTypeKind::Curve) {
                continue;
            }
            for (const auto& ext : e.extensions) {
                const std::string pattern = "*" + ext;
                if (std::find(g.patterns.begin(), g.patterns.end(), pattern) ==
                    g.patterns.end()) {
                    g.patterns.push_back(pattern);
                }
            }
        }
    }
    return groups;
}

FileTypeCatalog::InconsistencyReport FileTypeCatalog::inconsistencies() {
    // 目录侧集合：目录「已支持」扩展名。
    std::set<std::string> catalogExts;
    for (const auto& e : supported()) {
        catalogExts.insert(e.extensions.begin(), e.extensions.end());
    }
    // 读取能力侧集合：ReaderRegistry 已注册扩展名（大小写归一）。
    std::set<std::string> registryExts;
    for (const auto& ext : ReaderRegistry::instance().registeredExtensions()) {
        registryExts.insert(toLower(ext));
    }

    // 对称差：两侧分别报告（FR-011），不静默。
    InconsistencyReport report;
    std::set_difference(catalogExts.begin(), catalogExts.end(),
                        registryExts.begin(), registryExts.end(),
                        std::back_inserter(report.catalogOnly));
    std::set_difference(registryExts.begin(), registryExts.end(),
                        catalogExts.begin(), catalogExts.end(),
                        std::back_inserter(report.registryOnly));
    return report;
}

}  // namespace io
}  // namespace core
}  // namespace perception
