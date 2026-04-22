#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/Inventory.hpp"

#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【AddItem】向背包添加物品
// ============================================================================
void Inventory::AddItem(const std::string& item_id, int count) {
    (void)TryAddItem(item_id, count);
}

Result<int> Inventory::TryAddItem(const std::string& item_id, int count) {
    if (count <= 0 || item_id.empty()) {
        return Result<int>("无效的添加请求：物品ID为空或数量<=0");
    }

    auto it = FindSlot_(item_id);
    if (it != slots_.end()) {
        it->count += count;
        return it->count;
    }

    slots_.push_back({item_id, count});
    return count;
}

// ============================================================================
// 【AddItems】批量添加物品
// ============================================================================
void Inventory::AddItems(const std::vector<ItemCount>& items) {
    (void)TryAddItems(items);
}

Result<void> Inventory::TryAddItems(const std::vector<ItemCount>& items) {
    for (const auto& item : items) {
        auto result = TryAddItem(item.item_id, item.count);
        if (!result) {
            return Result<void>(result.Error());
        }
    }
    return Result<void>();
}

// ============================================================================
// 【RemoveItem】从背包移除物品
// ============================================================================
bool Inventory::RemoveItem(const std::string& item_id, int count) {
    if (count <= 0 || item_id.empty()) {
        return false;
    }

    auto it = FindSlot_(item_id);
    if (it == slots_.end()) {
        return false;
    }

    if (it->count < count) {
        return false;
    }

    it->count -= count;
    if (it->count == 0) {
        slots_.erase(it);
    }
    return true;
}

Result<int> Inventory::TryRemoveItem(const std::string& item_id, int count) {
    if (count <= 0 || item_id.empty()) {
        return Result<int>("无效的移除请求：数量=" + std::to_string(count));
    }

    auto it = FindSlot_(item_id);
    if (it == slots_.end()) {
        return Result<int>("物品不存在：" + item_id);
    }

    if (it->count < count) {
        return Result<int>("物品数量不足：" + item_id + "（需要=" + std::to_string(count) + "，持有=" + std::to_string(it->count) + "）");
    }

    const int remaining = it->count - count;
    it->count = remaining;
    if (remaining == 0) {
        slots_.erase(it);
    }
    return remaining;
}

// ============================================================================
// 【RemoveItems】批量移除物品（原子操作）
// ============================================================================
bool Inventory::RemoveItems(const std::vector<ItemCount>& items) {
    return TryRemoveItems(items).Ok();
}

Result<void> Inventory::TryRemoveItems(const std::vector<ItemCount>& items) {
    // 先检查所有物品是否足够
    for (const auto& item : items) {
        if (item.item_id.empty() || item.count <= 0) {
            return Result<void>("批量移除参数无效");
        }
        if (CountOf(item.item_id) < item.count) {
            return Result<void>("批量移除失败：物品数量不足 - " + item.item_id);
        }
    }

    // 所有物品足够，执行批量移除
    for (const auto& item : items) {
        auto result = TryRemoveItem(item.item_id, item.count);
        if (!result) {
            return Result<void>("批量移除失败（执行阶段）：" + result.Error());
        }
    }
    return Result<void>();
}

// ============================================================================
// 【Clear】清空背包
// ============================================================================
void Inventory::Clear() {
    slots_.clear();
}

// ============================================================================
// 【CountOf】查询物品数量
// ============================================================================
int Inventory::CountOf(const std::string& item_id) const {
    auto it = FindSlot_(item_id);
    if (it != slots_.end()) {
        return it->count;
    }
    return 0;
}

// ============================================================================
// 【HasItems】检查是否持有足够数量的多个物品
// ============================================================================
bool Inventory::HasItems(const std::vector<ItemCount>& items) const {
    for (const auto& item : items) {
        if (CountOf(item.item_id) < item.count) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// 【TotalItemCount】所有物品的总数量
// ============================================================================
int Inventory::TotalItemCount() const {
    int total = 0;
    for (const auto& slot : slots_) {
        total += slot.count;
    }
    return total;
}

// ============================================================================
// 【ToMap】转换为Map形式（用于存档）
// ============================================================================
std::unordered_map<std::string, int> Inventory::ToMap() const {
    std::unordered_map<std::string, int> result;
    for (const auto& slot : slots_) {
        result[slot.item_id] = slot.count;
    }
    return result;
}

// ============================================================================
// 【FromMap】从Map恢复（用于读档）
// ============================================================================
void Inventory::FromMap(const std::unordered_map<std::string, int>& map) {
    Clear();
    slots_.reserve(map.size());
    for (const auto& [item_id, count] : map) {
        if (!item_id.empty() && count > 0) {
            slots_.push_back({item_id, count});
        }
    }
}

// ============================================================================
// 【SummaryText】生成摘要文本
// ============================================================================
std::string Inventory::SummaryText() const {
    if (slots_.empty()) {
        return "背包：空";
    }

    std::ostringstream oss;
    oss << "背包：";
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (i > 0) {
            oss << "，";
        }
        oss << slots_[i].item_id << " x" << slots_[i].count;
    }
    return oss.str();
}

// ============================================================================
// 【FindSlot_】查找物品槽位（内部）
// ============================================================================
std::vector<ItemCount>::iterator Inventory::FindSlot_(const std::string& item_id) {
    for (auto it = slots_.begin(); it != slots_.end(); ++it) {
        if (it->item_id == item_id) {
            return it;
        }
    }
    return slots_.end();
}

std::vector<ItemCount>::const_iterator Inventory::FindSlot_(const std::string& item_id) const {
    for (auto it = slots_.begin(); it != slots_.end(); ++it) {
        if (it->item_id == item_id) {
            return it;
        }
    }
    return slots_.end();
}

}  // namespace CloudSeamanor::domain
