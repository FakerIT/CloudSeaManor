#include "CloudSeamanor/engine/systems/InnSystem.hpp"

#include <algorithm>
#include <array>

namespace CloudSeamanor::engine {

namespace {

std::size_t StableHash_(const std::string& text) {
    std::size_t h = 1469598103934665603ull;
    for (unsigned char c : text) {
        h ^= static_cast<std::size_t>(c);
        h *= 1099511628211ull;
    }
    return h;
}

float DecorationInnVisitMultiplier_(int decoration_score) {
    // 0..60 -> 1.00..1.30 (cap 30%)
    return 1.0f + std::clamp(static_cast<float>(decoration_score), 0.0f, 60.0f) * 0.005f;
}

float NpcVisitWeightFromOrder_(
    const InnOrderEntry& order,
    int house_level,
    int reputation,
    float avg_favor,
    int decoration_score) {
    const float house = 1.0f + std::max(0, house_level - 1) * 0.18f;
    const float rep = 1.0f + std::clamp(static_cast<float>(reputation), -20.0f, 80.0f) * 0.01f;
    const float social = 1.0f + std::clamp(avg_favor / 2000.0f, 0.0f, 0.6f);
    const float decor = DecorationInnVisitMultiplier_(decoration_score);
    return std::clamp(order.npc_visit_weight * house * rep * social * decor, 0.4f, 3.6f);
}

}  // namespace

void InnSystem::RefreshOrderPool_(GameWorldState& world_state, int house_level) const {
    auto& orders = world_state.MutableInnOrders();
    if (!orders.empty()) {
        return;
    }

    static const std::array<const char*, 6> kOrderItems = {
        "TeaPack", "TeaLeaf", "Turnip", "Egg", "Milk", "spirit_dust"};
    const int day = world_state.GetClock().Day();
    const int count = std::clamp(2 + std::max(0, house_level - 1), 2, 5);
    for (int i = 0; i < count; ++i) {
        const std::size_t idx = (static_cast<std::size_t>(day) + static_cast<std::size_t>(i) * 3u) % kOrderItems.size();
        InnOrderEntry entry;
        entry.order_id = "inn_" + std::to_string(day) + "_" + std::to_string(i + 1);
        entry.item_id = kOrderItems[idx];
        entry.required_count = (entry.item_id == "TeaLeaf" || entry.item_id == "Turnip") ? 3 : 1;
        entry.reward_gold = 45 + house_level * 18 + i * 6;
        entry.npc_visit_weight = 0.9f + static_cast<float>((StableHash_(entry.order_id) % 8u)) * 0.1f;
        entry.fulfilled = false;
        orders.push_back(std::move(entry));
    }
}

void InnSystem::DailySettlement(GameWorldState& world_state, int house_level) {
    RefreshOrderPool_(world_state, house_level);
    auto& orders = world_state.MutableInnOrders();

    const int level = std::max(1, house_level);
    float avg_favor = 0.0f;
    if (!world_state.GetNpcs().empty()) {
        int total = 0;
        for (const auto& npc : world_state.GetNpcs()) {
            total += npc.favor;
        }
        avg_favor = static_cast<float>(total) / static_cast<float>(world_state.GetNpcs().size());
    }

    int fulfilled_count = 0;
    int order_income = 0;
    float visit_weight_sum = 0.0f;
    for (auto& order : orders) {
        const float weight = NpcVisitWeightFromOrder_(
            order, level, world_state.GetInnReputation(), avg_favor, world_state.GetDecorationScore());
        visit_weight_sum += weight;
        if (!order.fulfilled && world_state.MutableInventory().CountOf(order.item_id) >= order.required_count) {
            (void)world_state.MutableInventory().TryRemoveItem(order.item_id, order.required_count);
            order.fulfilled = true;
            ++fulfilled_count;
            order_income += order.reward_gold;
        }
    }

    const int visitors = std::max(0, static_cast<int>(visit_weight_sum));
    const int walk_in_income = visitors * (12 + level * 4);
    const int upkeep = 12 + level * 6;
    const int total_income = std::max(0, walk_in_income + order_income - upkeep);
    world_state.MutableInnVisitorsToday() = visitors;
    world_state.MutableInnIncomeToday() = total_income;
    world_state.MutableInnReputation() = std::clamp(
        world_state.GetInnReputation() + fulfilled_count * 2 - std::max(0, static_cast<int>(orders.size()) - fulfilled_count),
        -20,
        120);

    world_state.MutableInnGoldReserve() += total_income;
    world_state.MutableGold() += total_income;
    orders.clear();
}

} // namespace CloudSeamanor::engine
