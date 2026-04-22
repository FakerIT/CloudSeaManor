#pragma once

// ============================================================================
// 【Inventory】玩家背包物品管理系统
// ============================================================================
// 统一管理玩家的物品栏位，支持物品的增加、移除、查询和存档。
//
// 主要职责：
// - 管理玩家持有的物品（ID + 数量）
// - 提供物品数量的查询接口
// - 支持存档保存与恢复
// - 生成调试/显示用的摘要文本
//
// 与其他系统的关系：
// - 依赖：无（纯领域层）
// - 被依赖：GameAppFarming（收获物品）、GameAppNpc（送礼消耗）、
//           GameAppHud（显示物品）、GameAppSave（存档/读档）
//
// 设计说明：
// - 采用简单列表存储（而非固定格子系统），原型期足够轻量
// - 支持后续扩展品质、耐久、标签等属性
// ============================================================================

#include "CloudSeamanor/Result.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【ItemCount】物品数量对
// ============================================================================
// 用于存档和批量操作的简单数据结构。
// 一个物品ID对应一个持有数量。
struct ItemCount {
    std::string item_id;  // 物品唯一标识符
    int count = 0;        // 持有数量

    // 比较操作符，方便排序和查找
    bool operator==(const ItemCount& other) const noexcept {
        return item_id == other.item_id;
    }
    bool operator<(const ItemCount& other) const noexcept {
        return item_id < other.item_id;
    }
};

// ============================================================================
// 【Inventory】玩家背包领域对象
// ============================================================================
// 管理玩家持有的所有物品，提供增删查改和存档接口。
//
// 设计决策：
// - 使用列表而非固定格子，简化原型期开发
// - 物品ID为字符串，支持任意物品类型
// - 数量必须为非负整数，0表示无此物品
//
// 使用示例：
// @code
// Inventory inv;
// inv.AddItem("green_tea", 10);     // 添加10个绿茶
// bool ok = inv.RemoveItem("green_tea", 5);  // 移除5个
// int count = inv.CountOf("green_tea");       // 查询数量
// @endcode
class Inventory {
public:
    // ========================================================================
    // 【AddItem】向背包添加物品
    // ========================================================================
    // @param item_id 物品的唯一标识符
    // @param count 添加的数量（默认为1）
    // @note 如果物品已存在则叠加数量，否则创建新槽位
    // @note count <= 0 的请求会被忽略
    void AddItem(const std::string& item_id, int count = 1);
    [[nodiscard]] Result<int> TryAddItem(const std::string& item_id, int count = 1);

    // ========================================================================
    // 【AddItems】批量添加物品
    // ========================================================================
    // @param items 物品列表
    // @note 用于批量添加多个不同物品（如收获多个作物）
    void AddItems(const std::vector<ItemCount>& items);
    [[nodiscard]] Result<void> TryAddItems(const std::vector<ItemCount>& items);

    // ========================================================================
    // 【RemoveItem】从背包移除物品
    // ========================================================================
    // @param item_id 物品标识符
    // @param count 移除数量（默认为1）
    // @return true 如果移除成功（物品存在且数量足够）
    // @return false 如果物品不存在或数量不足
    // @note 移除后数量为0时，该槽位会被删除
    // @note count <= 0 的请求直接返回false
    [[nodiscard]] bool RemoveItem(const std::string& item_id, int count = 1);

    [[nodiscard]] Result<int> TryRemoveItem(const std::string& item_id, int count = 1);

    // ========================================================================
    // 【RemoveItems】批量移除物品
    // ========================================================================
    // @param items 物品列表
    // @return true 如果全部移除成功
    // @return false 如果任何物品不足或不存在
    // @note 全部成功或全部失败（原子操作）
    [[nodiscard]] bool RemoveItems(const std::vector<ItemCount>& items);
    [[nodiscard]] Result<void> TryRemoveItems(const std::vector<ItemCount>& items);

    // ========================================================================
    // 【Clear】清空背包
    // ========================================================================
    // @note 通常用于读档前重建或测试场景
    void Clear();

    // ========================================================================
    // 【CountOf】查询物品数量
    // ========================================================================
    // @param item_id 物品标识符
    // @return 物品数量，不存在则返回0
    [[nodiscard]] int CountOf(const std::string& item_id) const;

    // ========================================================================
    // 【HasItem】检查是否持有指定物品
    // ========================================================================
    // @param item_id 物品标识符
    // @return true 如果数量 > 0
    [[nodiscard]] bool HasItem(const std::string& item_id) const {
        return CountOf(item_id) > 0;
    }

    // ========================================================================
    // 【HasItems】检查是否持有足够数量的多个物品
    // ========================================================================
    // @param items 物品列表
    // @return true 如果所有物品数量都足够
    [[nodiscard]] bool HasItems(const std::vector<ItemCount>& items) const;

    // ========================================================================
    // 【SlotCount】槽位数量（不同物品种类数）
    // ========================================================================
    [[nodiscard]] std::size_t SlotCount() const noexcept { return slots_.size(); }

    // ========================================================================
    // 【TotalItemCount】所有物品的总数量
    // ========================================================================
    [[nodiscard]] int TotalItemCount() const;

    // ========================================================================
    // 【IsEmpty】背包是否为空
    // ========================================================================
    [[nodiscard]] bool IsEmpty() const noexcept { return slots_.empty(); }

    // ========================================================================
    // 【Slots】获取槽位列表（只读引用）
    // ========================================================================
    // 用于HUD遍历显示或存档系统遍历保存
    [[nodiscard]] const std::vector<ItemCount>& Slots() const noexcept {
        return slots_;
    }

    // ========================================================================
    // 【ToMap】转换为Map形式（用于存档）
    // ========================================================================
    [[nodiscard]] std::unordered_map<std::string, int> ToMap() const;

    // ========================================================================
    // 【FromMap】从Map恢复（用于读档）
    // ========================================================================
    // @param map 存档数据
    // @note 会清空现有数据后重新加载
    void FromMap(const std::unordered_map<std::string, int>& map);

    // ========================================================================
    // 【SummaryText】生成摘要文本
    // ========================================================================
    // @return 格式如 "Inventory: green_tea x10, wood x5"
    // @note 用于调试或最小UI显示
    [[nodiscard]] std::string SummaryText() const;

private:
    // ========================================================================
    // 【FindSlot】查找物品槽位
    // ========================================================================
    // @return 指向对应槽位的迭代器，找不到返回 end()
    [[nodiscard]] std::vector<ItemCount>::iterator FindSlot_(const std::string& item_id);
    [[nodiscard]] std::vector<ItemCount>::const_iterator FindSlot_(const std::string& item_id) const;

    // ========================================================================
    // 成员变量
    // ========================================================================
    std::vector<ItemCount> slots_;  // 物品槽位列表
};

}  // namespace CloudSeamanor::domain
