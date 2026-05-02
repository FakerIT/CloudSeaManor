#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <SFML/Graphics/Color.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor {
namespace engine {

// ============================================================================
// 【好感度等级信息】
// ============================================================================
struct HeartLevelInfo {
    int level = 0;           // 等级 0-10
    std::string name;        // 等级名称：薄云/积云/层云/密云/积雨/风暴/霓虹/神圣
    bool unlocked = false;   // 是否已解锁
    int required_favor = 0;  // 达到该等级所需好感度
};

// ============================================================================
// 【NPC社交信息】
// ============================================================================
struct NpcSocialInfo {
    std::string npc_id;
    std::string npc_name;
    int favor = 0;                          // 当前好感度
    int heart_level = 0;                     // 心等级 0-10
    std::vector<HeartLevelInfo> heart_levels; // 所有心等级信息
    bool talked_today = false;               // 今日已对话
    bool gifted_today = false;               // 今日已送礼
    std::string current_location;             // 当前地点
    std::string preferred_gift;               // 喜好礼物
    std::string loved_gifts;                  // 喜爱物品（逗号分隔）
    std::string disliked_gifts;               // 讨厌物品
    std::vector<std::string> triggered_events;  // 已触发的事件列表
    std::string next_event_hint;              // 下一事件提示
    int favor_to_next_level = 0;              // 距离下一级好感度
};

// ============================================================================
// 【送礼反馈信息】
// ============================================================================
struct GiftFeedbackInfo {
    std::string item_name;
    int favor_change = 0;
    std::string feedback_text;        // "喜爱！+30" / "喜欢 +15" / "普通 +1" / "讨厌 -5"
    std::string reaction_emoji;        // "❤️" / "😊" / "😐" / "😞"
};

// ============================================================================
// 【社交面板视图数据】
// ============================================================================
struct SocialPanelViewData {
    std::string title = "社交面板";
    std::vector<NpcSocialInfo> npcs;      // 所有NPC信息
    std::vector<GiftFeedbackInfo> recent_gifts;  // 最近送礼记录
    int total_friends = 0;                // 好友总数
    int total_hearts_10 = 0;              // 满心NPC数量
    std::string filter_mode = "全部";      // 筛选模式：全部/已好友/未对话
    std::string sort_mode = "好感度";      // 排序模式：好感度/姓名/心等级
};

// ============================================================================
// 【PixelSocialPanel】社交系统面板
// ============================================================================
class PixelSocialPanel : public PixelUiPanel {
public:
    PixelSocialPanel();

    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const SocialPanelViewData& data) { data_ = data; }

    // 获取当前选中的NPC索引
    int GetSelectedNpcIndex() const { return selected_npc_index_; }
    void SetSelectedNpcIndex(int index);

    // 选中NPC切换
    void SelectNextNpc();
    void SelectPrevNpc();

    // 获取当前选中的NPC信息
    const NpcSocialInfo* GetSelectedNpc() const;

protected:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

private:
    void RenderNpcList_(sf::RenderWindow& window, const sf::FloatRect& list_area, float start_y);
    void RenderNpcDetail_(sf::RenderWindow& window, const sf::FloatRect& detail_area, const NpcSocialInfo& npc);
    void RenderHeartProgress_(sf::RenderWindow& window, float x, float y, float width, const std::vector<HeartLevelInfo>& levels, int current_level);
    void RenderGiftHistory_(sf::RenderWindow& window, const sf::FloatRect& area, float start_y);

    const PixelFontRenderer* m_font_renderer = nullptr;
    SocialPanelViewData data_{};
    int selected_npc_index_ = 0;
};

}  // namespace CloudSeamanor::engine
