#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace perception::core::event {

// 事件类型：core 数据变更的统一通知面。
enum class EventType {
    DataSetChanged,    // 曲线数据集变更（加载 / 增删曲线 / 变换）
    StructureChanged,  // 结构数据变更
    SelectionChanged,  // 选择变化（曲线 / 场选中）
    StatusMessage,     // 状态 / 进度消息
};

// 事件负载：类型化数据指针，消费者自行 downcast；通知型事件通常为 nullptr。
using EventCallback = std::function<void(EventType, const void*)>;
using Token = std::uint64_t;

// 事件总线：发布-订阅。render / ui 只订阅事件更新画面，不直接轮询 core。
// 规则：数据变更必须先 publish 再返回；回调中避免再次发布同类型事件。
class EventBus {
public:
    Token subscribe(EventType type, EventCallback cb);
    void unsubscribe(Token token);
    void publish(EventType type, const void* payload = nullptr) const;

private:
    struct Entry {
        Token token;
        EventCallback cb;
    };

    mutable std::mutex mutex_;
    std::unordered_map<EventType, std::vector<Entry>> subscribers_;
    Token nextToken_ = 1;
};

// RAII 订阅句柄：析构自动退订，防止悬挂回调。
class ScopedSubscription {
public:
    ScopedSubscription(EventBus& bus, Token token) : bus_(&bus), token_(token) {}
    ~ScopedSubscription()
    {
        if (bus_) {
            bus_->unsubscribe(token_);
        }
    }

    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;

    ScopedSubscription(ScopedSubscription&& other) noexcept
        : bus_(other.bus_), token_(other.token_)
    {
        other.bus_ = nullptr;
    }

    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept
    {
        if (this != &other) {
            if (bus_) {
                bus_->unsubscribe(token_);
            }
            bus_ = other.bus_;
            token_ = other.token_;
            other.bus_ = nullptr;
        }
        return *this;
    }

private:
    EventBus* bus_;
    Token token_;
};

} // namespace perception::core::event
