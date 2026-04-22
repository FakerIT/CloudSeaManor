#pragma once
// 【EventBus】事件总线
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace CloudSeamanor::engine {
struct Event {
    std::string type;
    std::unordered_map<std::string, std::string> data;
    template <typename T>
    [[nodiscard]] T GetAs(const std::string& key, T fallback) const {
        const auto it = data.find(key);
        if (it == data.end()) return fallback;
        try {
            if constexpr (std::is_same_v<T, int>) return std::stoi(it->second);
            if constexpr (std::is_same_v<T, float>) return std::stof(it->second);
            if constexpr (std::is_same_v<T, bool>) return it->second == "true";
            return static_cast<T>(it->second);
        } catch (...) { return fallback; }
    }
};
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;
    [[nodiscard]] std::size_t Subscribe(const std::string& event_type, Handler handler);
    void Emit(const Event& event);
    void Unsubscribe(std::size_t subscription_id);
    void Clear();
    [[nodiscard]] std::size_t Count() const { return subscriptions_.size(); }
private:
    struct Subscription {
        std::size_t id;
        std::string type;
        Handler handler;
    };
    std::vector<Subscription> subscriptions_;
    std::size_t next_id_ = 0;
};
EventBus& GlobalEventBus();
}  // namespace CloudSeamanor::engine
