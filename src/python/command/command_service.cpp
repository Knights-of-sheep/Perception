// ===== 命令层骨架实现（006 US4；009 新增 supportedFormats 只读查询）=====
// 占位命令一律抛 NotImplementedError（模块内经 register_exception 注册为
// Python 内建 NotImplementedError 子类），接口签名即 M5 契约；
// createWindow 真实命令经 IWindowFactory 注册表派发；
// supportedFormats 只读查询经文件类型权威目录派生（契约 python-supported-formats.md）。
#include "command_service.h"

#include "core/io/file_type_catalog.h"

#include "registry.h"

namespace perception {
namespace python {

namespace {

// 枚举 → 契约值域字符串（python-supported-formats.md）。
using core::io::FileTypeFamily;
using core::io::FileTypeKind;
using core::io::FileTypeStatus;

const char* familyValue(FileTypeFamily family) {
    switch (family) {
        case FileTypeFamily::VtkLegacy:    return "vtk-legacy";
        case FileTypeFamily::VtkXml:       return "vtk-xml";
        case FileTypeFamily::VtkComposite: return "vtk-composite";
        case FileTypeFamily::VtkParallel:  return "vtk-parallel";
        case FileTypeFamily::SVisual:      return "svisual";
        case FileTypeFamily::Hdf5:         return "hdf5";
        case FileTypeFamily::Curve:        return "curve";
    }
    return "";
}

const char* kindValue(FileTypeKind kind) {
    switch (kind) {
        case FileTypeKind::Curve:     return "curve";
        case FileTypeKind::Structure: return "structure";
        case FileTypeKind::Both:      return "both";
    }
    return "";
}

const char* statusValue(FileTypeStatus status) {
    switch (status) {
        case FileTypeStatus::Supported: return "supported";
        case FileTypeStatus::Planned:   return "planned";
    }
    return "";
}

}  // namespace

std::string CommandServiceImpl::createWindow(const std::string& title) {
    IWindowFactory* factory = getWindowFactory();
    return factory ? factory->createWindow(title) : std::string();
}

py::object CommandServiceImpl::supportedFormats() {
    // 只读查询：遍历权威目录数据表（含 supported + planned，顺序稳定），
    // 按契约逐条组装 dict；无副作用，不依赖宿主窗口。
    py::list result;
    for (const auto& e : core::io::FileTypeCatalog::all()) {
        py::dict entry;
        py::list exts;
        for (const auto& ext : e.extensions) {
            exts.append(py::str(ext));
        }
        entry["extensions"] = exts;
        entry["format_name"] = py::str(e.formatName);
        entry["family"] = py::str(familyValue(e.family));
        entry["kind"] = py::str(kindValue(e.kind));
        entry["status"] = py::str(statusValue(e.status));
        result.append(entry);
    }
    return result;
}

py::object CommandServiceImpl::load(const std::string& /*path*/) {
    throw NotImplementedError("load() not implemented yet (M5)");  // 数据加载（load_plt 等）
}

py::object CommandServiceImpl::transform(py::object /*ds*/,
                                         const std::string& /*name*/,
                                         py::kwargs /*kwargs*/) {
    throw NotImplementedError("transform() not implemented yet (M5)");  // 数据变换
}

py::object CommandServiceImpl::query(py::object /*ds*/, py::kwargs /*kwargs*/) {
    throw NotImplementedError("query() not implemented yet (M5)");  // 数据查询
}

py::object CommandServiceImpl::exportData(py::object /*ds*/,
                                          const std::string& /*path*/) {
    throw NotImplementedError("export() not implemented yet (M5)");  // 数据导出
}

}  // namespace python
}  // namespace perception
