#include "CloudSeamanor/engine/EventBus.hpp"
#include <algorithm>
namespace CloudSeamanor::engine {
static EventBus g_event_bus;
EventBus& GlobalEventBus() { return g_event_bus; }
std::size_t EventBus::Subscribe(const std::string& event_type, Handler handler) {
    subscriptions_.push_back({next_id_, event_type, std::move(handler)});
    return next_id_++;
}
void EventBus::Emit(const Event& event) {
    for (const auto& sub : subscriptions_) {
        if (sub.type == event.type) sub.handler(event);
    }
}
void EventBus::Unsubscribe(std::size_t subscription_id) {
    subscriptions_.erase(
        std::remove_if(
            subscriptions_.begin(),
            subscriptions_.end(),
            [subscription_id](const Subscription& sub) {
                return sub.id == subscription_id;
            }),
        subscriptions_.end());
}
void EventBus::Clear() { subscriptions_.clear(); }
}  // namespace CloudSeamanor::engine