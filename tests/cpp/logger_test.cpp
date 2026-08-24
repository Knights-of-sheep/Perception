// ===== 统一日志子系统单测（core/log）=====
// 覆盖：LogLevel 映射、LogLevelMatrix、FileSink 写入/格式/降级/追加、
//       Logger 广播/过滤/并发、来源宏。全部无 UI 依赖。
#include "core/log/file_sink.h"
#include "core/log/logger.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace perception::core::log;

namespace {

class RecordingSink final : public LogSink {
public:
    void emit(const LogRecord& r) override {
        std::lock_guard<std::mutex> lock(mu_);
        records_.push_back(r);
    }
    const char* name() const noexcept override { return "RecordingSink"; }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return records_.size();
    }
    std::vector<LogRecord> snapshot() const {
        std::lock_guard<std::mutex> lock(mu_);
        return records_;
    }

private:
    mutable std::mutex mu_;
    std::vector<LogRecord> records_;
};

bool fileExists(const std::string& p) { std::ifstream f(p); return f.good(); }

std::string readFile(const std::string& p) {
    std::ifstream f(p);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

std::string tempDir() {
    const char* tmp = std::getenv("TEMP");
    return tmp ? std::string(tmp) : std::string(".");
}

} // namespace

void testLogLevelMapping()
{
    assert(std::string(toString(LogLevel::Debug)) == "DEBUG");
    assert(std::string(toString(LogLevel::Info)) == "INFO");
    assert(std::string(toString(LogLevel::Warn)) == "WARN");
    assert(std::string(toString(LogLevel::Error)) == "ERROR");
    assert(std::string(toString(LogLevel::Fatal)) == "FATAL");

    assert(parseLevel("debug") == LogLevel::Debug);
    assert(parseLevel("DEBUG") == LogLevel::Debug);
    assert(parseLevel("Info") == LogLevel::Info);
    assert(parseLevel("warn") == LogLevel::Warn);
    assert(parseLevel("ERROR") == LogLevel::Error);
    assert(parseLevel("Fatal") == LogLevel::Fatal);

    assert(parseLevel("verbose") == LogLevel::Info);
    assert(parseLevel("") == LogLevel::Info);
    assert(parseLevel("42") == LogLevel::Info);
    std::cout << "log level mapping ok" << std::endl;
}

void testLogLevelMatrix()
{
    LogLevelMatrix m;
    assert(!m.isEnabled(LogLevel::Debug));
    assert(m.isEnabled(LogLevel::Info));
    assert(m.isEnabled(LogLevel::Warn));
    assert(m.isEnabled(LogLevel::Error));
    assert(m.isEnabled(LogLevel::Fatal));

    m.setEnabled(LogLevel::Debug, true);
    assert(m.isEnabled(LogLevel::Debug));
    m.setEnabled(LogLevel::Info, false);
    assert(!m.isEnabled(LogLevel::Info));

    m.setAll(true);
    for (int i = 0; i < 5; ++i)
        assert(m.isEnabled(static_cast<LogLevel>(i)));
    m.setAll(false);
    for (int i = 0; i < 5; ++i)
        assert(!m.isEnabled(static_cast<LogLevel>(i)));
    std::cout << "log level matrix ok" << std::endl;
}

void testFileSinkFormatAndAppend()
{
    const std::string dir = tempDir() + "\\perception_log_test\\sub\\nested";
    const std::string path = dir + "\\app.log";

    {
        FileSink sink(path);
        assert(sink.isWritable());
        sink.emit(LogRecord{std::chrono::system_clock::now(),
                            LogLevel::Info, "core:12", "first"});
        sink.emit(LogRecord{std::chrono::system_clock::now(),
                            LogLevel::Error, "ui/main.cpp:34", "second"});
    }

    assert(fileExists(path));
    const std::string content = readFile(path);
    assert(content.find("INFO [core:12] first") != std::string::npos);
    assert(content.find("ERROR [ui/main.cpp:34] second") != std::string::npos);
    assert(content.find("20") != std::string::npos);  // 年份前缀
    assert(content.find(":") != std::string::npos);   // 时间冒号

    {
        FileSink sink(path);  // 追加写：不覆盖
        assert(sink.isWritable());
        sink.emit(LogRecord{std::chrono::system_clock::now(),
                            LogLevel::Warn, "t:1", "third"});
    }
    const std::string content2 = readFile(path);
    assert(content2.find("third") != std::string::npos);
    assert(content2.find("first") != std::string::npos);
    std::cout << "file sink format/append ok" << std::endl;
}

void testFileSinkDegradation()
{
    FileSink sink("Z:\\no_such_drive_xyz\\a.log");  // 不可写路径
    sink.emit(LogRecord{std::chrono::system_clock::now(),
                        LogLevel::Error, "t:1", "degrade"});
    (void)sink;  // 不抛异常、不崩溃即可（FR-005 降级）
    std::cout << "file sink degradation ok" << std::endl;
}

void testLoggerBroadcastAndFilter()
{
    auto rec = std::make_shared<RecordingSink>();
    Logger& log = Logger::instance();
    log.addSink(rec);

    log.info("broadcast-1");
    log.warn("broadcast-2");
    log.error("broadcast-3");
    assert(rec->size() == 3);

    auto snap = rec->snapshot();
    assert(snap[0].level == LogLevel::Info);
    assert(snap[0].message == "broadcast-1");
    assert(snap[2].level == LogLevel::Error);

    rec->setLevelEnabled(LogLevel::Warn, false);   // 矩阵过滤
    log.warn("should-be-filtered");
    assert(rec->size() == 3);

    rec->setLevelEnabled(LogLevel::Warn, true);
    log.warn("warn-again");
    assert(rec->size() == 4);

    log.removeSink(rec);
    log.warn("after-remove");
    assert(rec->size() == 4);
    std::cout << "logger broadcast/filter ok" << std::endl;
}

void testSourceMacro()
{
    auto rec = std::make_shared<RecordingSink>();
    Logger& log = Logger::instance();
    log.addSink(rec);

    PERCEPTION_LOG_I("macro-info");
    PERCEPTION_LOG_E("macro-error");

    auto snap = rec->snapshot();
    assert(snap.size() >= 2);
    assert(snap[snap.size() - 2].source.find("logger_test.cpp") != std::string::npos);
    assert(snap[snap.size() - 1].source.find("logger_test.cpp") != std::string::npos);
    assert(snap[snap.size() - 1].message == "macro-error");

    log.removeSink(rec);
    std::cout << "source macro ok" << std::endl;
}

// US2：大小轮转（5MB×3；测试用小阈值快速触发）
void testFileRotation()
{
    const std::string dir = tempDir() + "\\perception_log_test\\rotate";
    const std::string path = dir + "\\app.log";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);  // 清空旧轮转文件

    // 小阈值：每条记录约 60B，maxFileSize=200 触发多次轮转
    FileSink sink(path, 200, 3);
    assert(sink.isWritable());

    for (int i = 0; i < 100; ++i) {
        sink.emit(LogRecord{std::chrono::system_clock::now(),
                            LogLevel::Info, "t:1",
                            "rotation-message-" + std::to_string(i)});
    }

    // 主文件存在且不超限
    assert(fileExists(path));
    const auto mainSize = std::filesystem::file_size(path, ec);
    assert(mainSize > 0);
    // 主文件不超阈值（轮转后留一个边界，允许略超一条）
    assert(mainSize <= 200 + 80);

    // 归档链：app.1.log / app.2.log / app.3.log 存在
    int backups = 0;
    for (int i = 1; i <= 3; ++i) {
        const std::string b = dir + "\\app." + std::to_string(i) + ".log";
        if (fileExists(b))
            ++backups;
    }
    assert(backups == 3);  // 100 条远超 3 次轮转，应填满 3 份归档
    std::cout << "file rotation ok" << std::endl;
}

// FR-016：切换日志路径时迁移旧日志（app.log + 归档链）
void testFileSinkMigrate()
{
    const std::string oldDir = tempDir() + "\\perception_log_test\\migrate\\old";
    const std::string newDir = tempDir() + "\\perception_log_test\\migrate\\new";
    const std::string oldPath = oldDir + "\\app.log";
    const std::string newPath = newDir + "\\app.log";
    std::error_code ec;
    std::filesystem::remove_all(tempDir() + "\\perception_log_test\\migrate", ec);

    {
        FileSink sink(oldPath, 200, 2);  // 小阈值触发轮转，生成归档
        assert(sink.isWritable());
        for (int i = 0; i < 60; ++i) {
            sink.emit(LogRecord{std::chrono::system_clock::now(), LogLevel::Info, "t:1",
                                "migrate-msg-" + std::to_string(i)});
        }
        // 归档已生成
        assert(fileExists(oldPath));
        assert(fileExists(oldDir + "\\app.1.log"));

        // 迁移到新路径
        assert(sink.migrateTo(newPath));
        assert(sink.isWritable());
        // 旧目录不再残留日志
        assert(!fileExists(oldPath));
        assert(!fileExists(oldDir + "\\app.1.log"));
        // 新目录已迁移主文件与归档
        assert(fileExists(newPath));
        assert(fileExists(newDir + "\\app.1.log"));
        // 迁移后继续追加
        sink.emit(LogRecord{std::chrono::system_clock::now(), LogLevel::Warn, "t:2",
                            "after-migrate"});
    }
    const std::string content = readFile(newPath);
    assert(content.find("after-migrate") != std::string::npos);
    std::cout << "file sink migrate ok" << std::endl;
}

// FR-017：清除历史日志（删除 app.log + 归档，重建空文件）
void testFileSinkClear()
{
    const std::string dir = tempDir() + "\\perception_log_test\\clear";
    const std::string path = dir + "\\app.log";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

    {
        FileSink sink(path, 200, 2);
        assert(sink.isWritable());
        for (int i = 0; i < 60; ++i) {
            sink.emit(LogRecord{std::chrono::system_clock::now(), LogLevel::Info, "t:1",
                                "clear-msg-" + std::to_string(i)});
        }
        assert(fileExists(path));
        assert(fileExists(dir + "\\app.1.log"));

        assert(sink.clearHistory());
        assert(sink.isWritable());
        // 归档全部删除，主文件重建为空文件
        assert(!fileExists(dir + "\\app.1.log"));
        assert(fileExists(path));
        assert(readFile(path).empty());

        sink.emit(LogRecord{std::chrono::system_clock::now(), LogLevel::Info, "t:2",
                            "after-clear"});
    }
    const std::string content = readFile(path);
    assert(content.find("after-clear") != std::string::npos);
    assert(content.find("clear-msg-0") == std::string::npos);  // 旧内容已清除
    std::cout << "file sink clear ok" << std::endl;
}

void testConcurrentLogging()
{
    auto rec = std::make_shared<RecordingSink>();
    Logger& log = Logger::instance();
    log.addSink(rec);

    constexpr int kThreads = 8;
    constexpr int kPerThread = 250;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&log, t, kPerThread] {
            for (int i = 0; i < kPerThread; ++i)
                log.info("t" + std::to_string(t) + "-" + std::to_string(i));
        });
    }
    for (auto& th : threads)
        th.join();
    assert(rec->size() == static_cast<std::size_t>(kThreads * kPerThread));

    const std::string path = tempDir() + "\\perception_log_test\\concurrent.log";
    std::error_code ec;
    std::filesystem::remove_all(tempDir() + "\\perception_log_test\\concurrent.log", ec);  // 清旧文件（追加模式）
    {
        FileSink sink(path);
        assert(sink.isWritable());
        std::vector<std::thread> wthreads;
        for (int t = 0; t < 4; ++t) {
            wthreads.emplace_back([&sink, t] {
                for (int i = 0; i < 100; ++i)
                    sink.emit(LogRecord{std::chrono::system_clock::now(),
                                        LogLevel::Info, "c", "msg"});
            });
        }
        for (auto& th : wthreads)
            th.join();
    }
    std::ifstream f(path);
    int lines = 0;
    std::string line;
    while (std::getline(f, line))
        ++lines;
    assert(lines == 400);

    log.removeSink(rec);
    std::cout << "concurrent logging ok" << std::endl;
}

int main()
{
    testLogLevelMapping();
    testLogLevelMatrix();
    testFileSinkFormatAndAppend();
    testFileSinkDegradation();
    testLoggerBroadcastAndFilter();
    testSourceMacro();
    testConcurrentLogging();
    testFileRotation();
    testFileSinkMigrate();
    testFileSinkClear();
    std::cout << "ALL logger tests passed" << std::endl;
    return 0;
}
