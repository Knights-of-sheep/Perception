// ===== 文件类型权威目录：宪法「文件格式范围」的唯一事实来源（009）=====
// 设计：
//   - 单一事实源：本文件 kFileTypeCatalog[] 数据表；打开过滤列表 / 文档表格 /
//     运行时可查询结果均由本目录派生（FR-001/FR-005~007），禁止在 UI/文档中手抄
//     格式清单（杜绝「文档说一套、程序里另一套」漂移）。
//   - 覆盖范围：宪法「文件格式范围」4 格式族（VTK 全部类型 / SVisual / HDF5 / 曲线）。
//   - 状态语义（FR-004）：Supported = 有读取能力（当前 .plt/.csv）；Planned = 范围
//     已批准、未实现（.tdr/.dat / VTK 各变体 / HDF5）。过滤列表覆盖全量条目（含
//     Planned，供对话框内发现与核对）；规划中格式选中后按「不支持」提示、不进入
//     读取流程（FR-009 依产品裁决放宽为「展示全部、打开拦截」）。
//   - 一致性（FR-011）：FileTypeCatalog::inconsistencies() 对比目录「已支持」⇄
//     ReaderRegistry 已注册扩展名的对称差，差异可读、可断言。
// 使用：C++ 运行时直读（FileTypeCatalog::*）；scripts/sync_file_types.py 用正则
//       解析同一数据表派生文档（theme_catalog 双消费模式）。
// 修改约定：任何格式清单的变更只改本文件数据表一处，再运行同步/校验脚本。
#pragma once

#include <string>
#include <vector>

namespace perception {
namespace core {
namespace io {

// 文件格式族（宪法「文件格式范围」；VTK 四族同属「VTK 全部类型」）。
enum class FileTypeFamily {
    VtkLegacy,     // VTK 遗留格式（.vtk）
    VtkXml,        // VTK XML 数据格式（.vti/.vtp/.vtu/.vts/.vtr）
    VtkComposite,  // VTK 复合格式（.vtm/.vtmb/.vth/.vto）
    VtkParallel,   // VTK 并行/集合（.pv* 系列 / .pvd）
    SVisual,       // Synopsys SVisual（.plt/.tdr）
    Hdf5,          // HDF5（.h5/.hdf5）
    Curve,         // 通用曲线数据（.csv/.dat）
};

// 数据种类（FR-003/FR-012）：曲线 / 结构 / 双用途。
enum class FileTypeKind { Curve, Structure, Both };

// 支持状态（FR-004）：已支持（有读取能力）/ 规划中（范围已批准、未实现）。
enum class FileTypeStatus { Supported, Planned };

// 权威目录条目（FR-003）：格式名称 / 扩展名 / 格式族 / 数据种类 / 支持状态。
struct FileTypeEntry {
    std::string formatName;               // 格式名称（如 "SVisual 曲线数据"）
    std::vector<std::string> extensions;  // 扩展名集合（小写、含点，如 {".plt"}）
    FileTypeFamily family;
    FileTypeKind kind;
    FileTypeStatus status;
};

// 打开过滤分组（filterGroups 返回）：familyKey 为分组标签（"SVisual"/"Curve Data"...）。
struct FileTypeFilterGroup {
    std::string familyKey;
    std::vector<std::string> patterns;  // 扩展名模式（如 "*.plt"）
};

// 权威目录数据表（单一事实来源，theme_catalog 模式）：宪法「文件格式范围」的全部
// 文件类型。条目顺序即查询返回顺序（稳定，FR-005）。每条目注释标注宪法依据。
inline const FileTypeEntry kFileTypeCatalog[] = {
    // ---- VTK 全部类型（宪法；具体化为 R5 枚举，全部「规划中」，读取器留待后续 feature）----
    {"VTK Legacy",             {".vtk"},
     FileTypeFamily::VtkLegacy, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Image Data",         {".vti"},
     FileTypeFamily::VtkXml, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Polygonal Data",     {".vtp"},
     FileTypeFamily::VtkXml, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Unstructured Grid",  {".vtu"},
     FileTypeFamily::VtkXml, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Structured Grid",    {".vts"},
     FileTypeFamily::VtkXml, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Rectilinear Grid",   {".vtr"},
     FileTypeFamily::VtkXml, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK MultiBlock",         {".vtm", ".vtmb"},
     FileTypeFamily::VtkComposite, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK HyperTreeGrid",      {".vth"},
     FileTypeFamily::VtkComposite, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Overlapping AMR",    {".vto"},
     FileTypeFamily::VtkComposite, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"VTK Parallel XML",       {".pvti", ".pvtp", ".pvtu", ".pvts", ".pvtr", ".pvtm"},
     FileTypeFamily::VtkParallel, FileTypeKind::Structure, FileTypeStatus::Planned},
    {"ParaView Collection",    {".pvd"},
     FileTypeFamily::VtkParallel, FileTypeKind::Structure, FileTypeStatus::Planned},
    // ---- SVisual（宪法：.plt/.tdr）----
    {"SVisual 曲线数据",       {".plt"},
     FileTypeFamily::SVisual, FileTypeKind::Curve, FileTypeStatus::Supported},
    {"SVisual 结构数据",       {".tdr"},
     FileTypeFamily::SVisual, FileTypeKind::Structure, FileTypeStatus::Planned},
    // ---- HDF5（宪法：.h5/.hdf5）----
    {"HDF5 数据",              {".h5", ".hdf5"},
     FileTypeFamily::Hdf5, FileTypeKind::Both, FileTypeStatus::Planned},
    // ---- 曲线（宪法：.csv/.dat）----
    {"CSV 曲线数据",           {".csv"},
     FileTypeFamily::Curve, FileTypeKind::Curve, FileTypeStatus::Supported},
    {"通用曲线数据",           {".dat"},
     FileTypeFamily::Curve, FileTypeKind::Both, FileTypeStatus::Planned},
};

// 目录访问器（静态 API；统一读取 kFileTypeCatalog[]）。
class FileTypeCatalog {
public:
    // 一致性差异报告（FR-011）：两侧差集，可读、可断言。
    struct InconsistencyReport {
        std::vector<std::string> catalogOnly;   // 目录「已支持」有、读取能力无
        std::vector<std::string> registryOnly;  // 读取能力有、目录无
        bool empty() const { return catalogOnly.empty() && registryOnly.empty(); }
    };

    // 全部条目（含 Planned；顺序 = 目录数据表顺序，稳定）。
    static const std::vector<FileTypeEntry>& all();
    // 仅「已支持」条目。
    static std::vector<FileTypeEntry> supported();
    // 大小写不敏感按扩展名查找（FR-010）；未命中返回 nullptr。
    static const FileTypeEntry* findByExtension(const std::string& ext);
    // 打开过滤分组（全量条目含 Planned，按格式族分组；FR-006/FR-009-变更：
    // 过滤覆盖目录全部类型供发现，规划中格式由加载路径按不支持拦截）。
    // 交叉聚合：曲线数据组额外包含其他格式族中 kind==Curve 的条目（SVisual .plt）。
    static std::vector<FileTypeFilterGroup> filterGroups();
    // 一致性校验：目录「已支持」扩展名 ⇄ ReaderRegistry 已注册扩展名对称差
    // （FR-008/FR-011）。
    static InconsistencyReport inconsistencies();
};

}  // namespace io
}  // namespace core
}  // namespace perception
