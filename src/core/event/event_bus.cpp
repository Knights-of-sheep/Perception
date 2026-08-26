#include "core/event/event_bus.h"

#include <algorithm>
#include <mutex>

namespace perception::core::event {

Token EventBus::subscribe(EventType type, EventCallback cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const Token token = nextToken_++;
    subscribers_[type].push_back(Entry{token, std::move(cb)});
    return token;
}

void EventBus::unsubscribe(Token token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [type, entries] : subscribers_) {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [token](const Entry& e) { return e.token == token; }),
                      entries.end());
    }
}

void EventBus::publish(EventType type, const void* payload) const
{
    // 拷贝订阅列表后再回调，允许回调中 unsubscribe 而不破坏迭代器。
    std::vector<Entry> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = subscribers_.find(type);
        if (it != subscribers_.end()) {
            snapshot = it->second;
        }
    }
    for (const auto& entry : snapshot) {
        entry.cb(type, payload);
    }
}

} // namespace perception::core::event
