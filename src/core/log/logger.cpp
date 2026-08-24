#include "core/log/logger.h"

#include "core/log/file_sink.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>

namespace perception::core::log {

struct Logger::Impl {
    std::mutex mutex;
    std::vector<LogSinkPtr> sinks;
    LogLevelMatrix defaultMatrix;
    std::string filePath;
    FileSink* fileSink = nullptr;

    void broadcast(const LogRecord& record)
    {
        std::vector<LogSinkPtr> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex);
            snapshot = sinks;
        }
        for (const auto& sink : snapshot) {
            if (sink->isLevelEnabled(record.level))
                sink->emit(record);
        }
    }
};

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::configure(const Config& cfg)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->defaultMatrix = cfg.levelMatrix;

    if (impl_->filePath != cfg.filePath) {
        impl_->filePath = cfg.filePath;
        if (impl_->fileSink) {
            const auto it = std::find_if(impl_->sinks.begin(), impl_->sinks.end(),
                                         [this](const LogSinkPtr& s) { return s.get() == impl_->fileSink; });
            if (it != impl_->sinks.end())
                impl_->sinks.erase(it);
            impl_->fileSink = nullptr;
        }
        if (!cfg.filePath.empty()) {
            auto sink = std::make_shared<FileSink>(cfg.filePath, cfg.maxFileSize, cfg.maxBackups);
            sink->setLevelMatrix(cfg.levelMatrix);
            impl_->fileSink = sink.get();
            impl_->sinks.push_back(std::move(sink));
        }
    }
}

const LogLevelMatrix& Logger::defaultMatrix() const noexcept
{
    return impl_->defaultMatrix;
}

bool Logger::setFilePath(const std::string& newPath)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->fileSink)
        return false;
    const bool ok = impl_->fileSink->migrateTo(newPath);  // 迁移旧日志并重建
    impl_->filePath = newPath;
    return ok;
}

bool Logger::clearLogFiles()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->fileSink)
        return false;
    return impl_->fileSink->clearHistory();
}

const std::string& Logger::filePath() const noexcept
{
    return impl_->filePath;
}

void Logger::log(LogLevel level, const std::string& msg)
{
    LogRecord record{std::chrono::system_clock::now(), level, "core", msg};
    impl_->broadcast(record);
}

void Logger::log(LogLevel level, const std::string& source, const std::string& msg)
{
    LogRecord record{std::chrono::system_clock::now(), level, source, msg};
    impl_->broadcast(record);
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& msg)
{
    const char* base = file ? strrchr(file, '/') : nullptr;
    const char* baseWin = file ? strrchr(file, '\\') : nullptr;
    const char* name = file ? (baseWin && baseWin > base ? baseWin + 1
                             : (base ? base + 1 : file)) : "core";
    const std::string source = std::string(name) + ":" + std::to_string(line);

    LogRecord record{std::chrono::system_clock::now(), level, source, msg};
    impl_->broadcast(record);
}

void Logger::debug(const std::string& msg) { log(LogLevel::Debug, msg); }
void Logger::info(const std::string& msg)  { log(LogLevel::Info, msg); }
void Logger::warn(const std::string& msg)  { log(LogLevel::Warn, msg); }
void Logger::error(const std::string& msg) { log(LogLevel::Error, msg); }
void Logger::fatal(const std::string& msg) { log(LogLevel::Fatal, msg); }

void Logger::debugAt(const char* file, int line, const std::string& msg) { log(LogLevel::Debug, file, line, msg); }
void Logger::infoAt(const char* file, int line, const std::string& msg)  { log(LogLevel::Info, file, line, msg); }
void Logger::warnAt(const char* file, int line, const std::string& msg)  { log(LogLevel::Warn, file, line, msg); }
void Logger::errorAt(const char* file, int line, const std::string& msg) { log(LogLevel::Error, file, line, msg); }
void Logger::fatalAt(const char* file, int line, const std::string& msg) { log(LogLevel::Fatal, file, line, msg); }

void Logger::addSink(LogSinkPtr sink)
{
    if (!sink)
        return;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    const auto it = std::find(impl_->sinks.begin(), impl_->sinks.end(), sink);
    if (it == impl_->sinks.end())
        impl_->sinks.push_back(std::move(sink));
}

void Logger::removeSink(LogSinkPtr sink)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->sinks.erase(std::remove(impl_->sinks.begin(), impl_->sinks.end(), sink),
                       impl_->sinks.end());
    if (impl_->fileSink == sink.get())
        impl_->fileSink = nullptr;
}

LogSinkPtr Logger::findSink(const char* name) const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (const auto& sink : impl_->sinks) {
        if (std::strcmp(sink->name(), name) == 0)
            return sink;
    }
    return nullptr;
}

std::size_t Logger::sinkCount() const noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->sinks.size();
}

std::vector<LogSinkPtr> Logger::sinks() const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->sinks;
}

Logger::Logger()
    : impl_(std::make_unique<Impl>())
{
}

Logger::~Logger() = default;

} // namespace perception::core::log
