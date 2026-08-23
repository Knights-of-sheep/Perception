#include "core/io/reader.h"

#include <algorithm>
#include <cctype>

namespace perception::core::io {

ReaderRegistry& ReaderRegistry::instance()
{
    static ReaderRegistry registry;
    return registry;
}

void ReaderRegistry::registerReader(std::shared_ptr<IReader> reader)
{
    std::lock_guard<std::mutex> lock(mutex_);
    readers_.push_back(std::move(reader));
}

std::shared_ptr<IReader> ReaderRegistry::findByPath(const std::string& path) const
{
    // 提取扩展名并小写化，再按注册表匹配（扩展名大小写不敏感）。
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return nullptr;
    }
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& reader : readers_) {
        for (const auto& e : reader->extensions()) {
            if (e == ext) {
                return reader;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<ICurveReader> ReaderRegistry::findCurveReader(
    const std::string& path) const
{
    return std::dynamic_pointer_cast<ICurveReader>(findByPath(path));
}

std::shared_ptr<IStructureReader> ReaderRegistry::findStructureReader(
    const std::string& path) const
{
    return std::dynamic_pointer_cast<IStructureReader>(findByPath(path));
}

} // namespace perception::core::io
