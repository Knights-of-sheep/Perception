// ===== 事件总线单测 =====
// 验证发布-订阅、退订、无订阅者发布的健壮性。
#include "core/event/event_bus.h"

#include <cassert>
#include <iostream>

using namespace perception::core::event;

int main()
{
    EventBus bus;
    int dataEvents = 0;
    int selEvents = 0;

    const Token sub1 = bus.subscribe(EventType::DataSetChanged,
                                     [&](EventType, const void*) { ++dataEvents; });
    const Token sub2 = bus.subscribe(EventType::DataSetChanged,
                                     [&](EventType, const void*) { ++dataEvents; });
    bus.subscribe(EventType::SelectionChanged,
                  [&](EventType, const void*) { ++selEvents; });

    bus.publish(EventType::DataSetChanged);
    assert(dataEvents == 2);
    assert(selEvents == 0);

    bus.publish(EventType::SelectionChanged);
    assert(selEvents == 1);

    bus.unsubscribe(sub1);
    bus.publish(EventType::DataSetChanged);
    assert(dataEvents == 3);  // 仅剩 sub2

    bus.unsubscribe(sub2);
    bus.publish(EventType::DataSetChanged);
    assert(dataEvents == 3);  // 无订阅者，不应崩溃

    // RAII 订阅自动退订
    {
        ScopedSubscription scoped(bus, bus.subscribe(EventType::StatusMessage,
                                                     [&](EventType, const void*) { ++selEvents; }));
        bus.publish(EventType::StatusMessage);
        assert(selEvents == 2);
    }
    bus.publish(EventType::StatusMessage);
    assert(selEvents == 2);  // scoped 已析构退订

    std::cout << "event bus tests passed" << std::endl;
    return 0;
}
