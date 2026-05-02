#include "CloudSeamanor/engine/PixelSocialPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【心等级名称映射】
// ============================================================================
static const std::vector<std::pair<int, std::string>> kHeartLevelNames = {
    {0, "陌生"},
    {1, "初识"},
    {2, "相识"},
    {3, "熟识"},
    {4, "好感"},
    {5, "亲近"},
    {6, "亲密"},
    {7, "挚友"},
    {8, "知心"},
    {9, "心许"},
    {10, "永结"}
};

// ============================================================================
// 【心等级颜色】
// ============================================================================
static const sf::Color kHeartColors[] = {
    sf::Color(150, 150, 150, 255),  // 陌生 - 灰色
    sf::Color(200, 200, 200, 255),  // 初识 - 浅灰
    sf::Color(100, 200, 100, 255),  // 相识 - 浅绿
    sf::Color(100, 180, 100, 255),  // 熟识 - 绿
    sf::Color(100, 160, 255, 255),  // 好感 - 蓝
    sf::Color(150, 120, 255, 255),  // 亲近 - 紫
    sf::Color(200, 100, 255, 255),  // 亲密 - 粉紫
    sf::Color(255, 100, 150, 255),  // 挚友 - 粉红
    sf::Color(255, 150, 100, 255),  // 知心 - 橙
    sf::Color(255, 200, 50, 255),   // 心许 - 金
    sf::Color(255, 215, 0, 255)      // 永结 - 深金
};

// ============================================================================
// 【心图标符号】
// ============================================================================
static const char* kHeartFilled = "\u2665";   // ♥
static const char* kHeartEmpty = "\u2661";    // ♡
static const char* kStarFilled = "\u2605";     // ★
static const char* kStarEmpty = "\u2606";      // ☆

// ============================================================================
// 【默认心等级信息】
// ============================================================================
static std::vector<HeartLevelInfo> CreateDefaultHeartLevels() {
    std::vector<HeartLevelInfo> levels;
    levels.reserve(11);
    // 每级所需好感度递增
    int favor_threshold = 0;
    for (int i = 0; i <= 10; ++i) {
        HeartLevelInfo info;
        info.level = i;
        info.name = (i < static_cast<int>(kHeartLevelNames.size())) 
            ? kHeartLevelNames[i].second 
            : "未知";
        info.unlocked = (i == 0);
        info.required_favor = favor_threshold;
        levels.push_back(info);
        // 每级增加所需好感度
        if (i < 4) {
            favor_threshold += 20;  // 前4级每级20
        } else if (i < 7) {
            favor_threshold += 50;  // 中间3级每级50
        } else if (i < 10) {
            favor_threshold += 100; // 高等级每级100
        } else {
            favor_threshold += 200; // 满级200
        }
    }
    return levels;
}

PixelSocialPanel::PixelSocialPanel()
    : PixelUiPanel({{280.0f, 80.0f}, {900.0f, 600.0f}}, "社交面板", true) {
    SetRect({{280.0f, 80.0f}, {900.0f, 600.0f}});
}

void PixelSocialPanel::SetSelectedNpcIndex(int index) {
    if (index >= 0 && index < static_cast<int>(data_.npcs.size())) {
        selected_npc_index_ = index;
    }
}

void PixelSocialPanel::SelectNextNpc() {
    if (!data_.npcs.empty()) {
        selected_npc_index_ = (selected_npc_index_ + 1) % static_cast<int>(data_.npcs.size());
    }
}

void PixelSocialPanel::SelectPrevNpc() {
    if (!data_.npcs.empty()) {
        selected_npc_index_ = (selected_npc_index_ - 1 + static_cast<int>(data_.npcs.size())) 
            % static_cast<int>(data_.npcs.size());
    }
}

const NpcSocialInfo* PixelSocialPanel::GetSelectedNpc() const {
    if (selected_npc_index_ >= 0 && selected_npc_index_ < static_cast<int>(data_.npcs.size())) {
        return &data_.npcs[selected_npc_index_];
    }
    return nullptr;
}

void PixelSocialPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;

    const float x = inner_rect.position.x + 8.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 20.0f;

    // 分割区域：左侧NPC列表(40%)，右侧详情(60%)
    const float list_width = inner_rect.size.x * 0.38f;
    const float detail_width = inner_rect.size.x * 0.58f;
    const float gap = 12.0f;

    sf::FloatRect list_area({x, y}, {list_width, inner_rect.size.y - 50.0f});
    sf::FloatRect detail_area({x + list_width + gap, y}, {detail_width, inner_rect.size.y - 50.0f});

    // 渲染NPC列表
    RenderNpcList_(window, list_area, y);

    // 渲染右侧详情
    if (const NpcSocialInfo* npc = GetSelectedNpc()) {
        RenderNpcDetail_(window, detail_area, *npc);
    }

    // 底部：筛选和统计信息
    const float bottom_y = y + inner_rect.size.y - 45.0f;
    m_font_renderer->DrawText(window, 
        "筛选: [" + data_.filter_mode + "]  排序: [" + data_.sort_mode + "]  |  好友: " + 
        std::to_string(data_.total_friends) + "  满心: " + std::to_string(data_.total_hearts_10),
        {x, bottom_y}, TextStyle::TopRightInfo());
}

void PixelSocialPanel::RenderNpcList_(sf::RenderWindow& window, const sf::FloatRect& list_area, float start_y) {
    const float x = list_area.position.x;
    const float y = list_area.position.y;
    const float row_h = 28.0f;
    const float icon_size = 18.0f;

    // 绘制背景
    sf::RectangleShape list_bg(list_area.size);
    list_bg.setPosition(list_area.position);
    list_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 3));
    list_bg.setOutlineThickness(1.0f);
    list_bg.setOutlineColor(ColorPalette::LightBrown);
    window.draw(list_bg);

    // 绘制NPC列表
    int visible_count = static_cast<int>(data_.npcs.size());
    float current_y = y + 6.0f;

    for (int i = 0; i < visible_count && current_y < y + list_area.size.y - 20.0f; ++i) {
        const NpcSocialInfo& npc = data_.npcs[i];
        bool is_selected = (i == selected_npc_index_);

        // 选中背景
        if (is_selected) {
            sf::RectangleShape select_bg({list_area.size.x - 4.0f, row_h});
            select_bg.setPosition({x + 2.0f, current_y});
            select_bg.setFillColor(ColorPalette::Lighten(ColorPalette::ActiveGreen, 2));
            window.draw(select_bg);
        }

        // 心图标（显示心等级）
        std::string heart_display;
        int filled_hearts = npc.heart_level;
        for (int h = 0; h < 10; ++h) {
            if (h < filled_hearts) {
                heart_display += kHeartFilled;
            } else {
                heart_display += kHeartEmpty;
            }
        }

        // NPC名称和心等级
        sf::Color name_color = is_selected ? sf::Color(50, 50, 50, 255) : sf::Color(80, 60, 40, 255);
        TextStyle npc_style = TextStyle::Default();
        npc_style.fill_color = name_color;

        if (is_selected) {
            npc_style.fill_color = sf::Color(30, 80, 30, 255);
            npc_style.outline_color = sf::Color(100, 180, 100, 255);
            npc_style.outline_thickness = 1.0f;
        }

        m_font_renderer->DrawText(window, heart_display, {x + 6.0f, current_y + 4.0f}, 
            TextStyle::Default());
        m_font_renderer->DrawText(window, npc.npc_name, {x + 210.0f, current_y + 4.0f}, npc_style);

        // 今日互动状态图标
        std::string status_icons;
        if (npc.talked_today) status_icons += "[聊]";
        if (npc.gifted_today) status_icons += "[礼]";

        if (!status_icons.empty()) {
            TextStyle status_style = TextStyle::TopRightInfo();
            status_style.fill_color = is_selected 
                ? sf::Color(50, 120, 50, 255) 
                : sf::Color(100, 100, 100, 255);
            m_font_renderer->DrawText(window, status_icons, 
                {x + list_area.size.x - 90.0f, current_y + 4.0f}, status_style);
        }

        current_y += row_h;
    }
}

void PixelSocialPanel::RenderNpcDetail_(sf::RenderWindow& window, const sf::FloatRect& detail_area, const NpcSocialInfo& npc) {
    const float x = detail_area.position.x;
    const float y = detail_area.position.y;
    const float row_h = 24.0f;
    const float content_width = detail_area.size.x - 16.0f;

    // 绘制背景
    sf::RectangleShape detail_bg(detail_area.size);
    detail_bg.setPosition(detail_area.position);
    detail_bg.setFillColor(ColorPalette::Cream);
    detail_bg.setOutlineThickness(1.0f);
    detail_bg.setOutlineColor(ColorPalette::Brown);
    window.draw(detail_bg);

    float current_y = y + 8.0f;

    // NPC名称和心等级名称
    TextStyle title_style = TextStyle::PanelTitle();
    title_style.fill_color = kHeartColors[std::min(npc.heart_level, 10)];
    m_font_renderer->DrawText(window, npc.npc_name, {x + 8.0f, current_y}, title_style);

    // 好感度数值
    current_y += row_h * 0.9f;
    m_font_renderer->DrawText(window, 
        "好感度: " + std::to_string(npc.favor) + 
        " (距离下一级: " + std::to_string(npc.favor_to_next_level) + ")",
        {x + 8.0f, current_y}, TextStyle::Default());

    // 心进度条
    current_y += row_h * 1.2f;
    m_font_renderer->DrawText(window, "心进度:", {x + 8.0f, current_y}, TextStyle::Default());
    RenderHeartProgress_(window, x + 8.0f, current_y + 22.0f, content_width, npc.heart_levels, npc.heart_level);

    // 当前地点
    current_y += row_h * 1.8f;
    m_font_renderer->DrawText(window, 
        "当前位置: " + npc.current_location,
        {x + 8.0f, current_y}, TextStyle::TopRightInfo());

    // 今日互动状态
    current_y += row_h * 1.1f;
    std::string interaction_status = "今日: ";
    interaction_status += npc.talked_today ? "[已对话]" : "[未对话]";
    interaction_status += " ";
    interaction_status += npc.gifted_today ? "[已送礼]" : "[可送礼]";
    m_font_renderer->DrawText(window, interaction_status, {x + 8.0f, current_y}, TextStyle::Default());

    // 喜好物品
    current_y += row_h * 1.3f;
    if (!npc.loved_gifts.empty()) {
        sf::RectangleShape gift_bg({content_width, 36.0f});
        gift_bg.setPosition({x + 8.0f, current_y});
        gift_bg.setFillColor(ColorPalette::Lighten(ColorPalette::LightPink, 2));
        gift_bg.setOutlineThickness(1.0f);
        gift_bg.setOutlineColor(sf::Color(255, 150, 150, 180));
        window.draw(gift_bg);

        m_font_renderer->DrawText(window, "喜爱物品: " + npc.loved_gifts, 
            {x + 12.0f, current_y + 8.0f}, TextStyle::HotkeyHint());
    }

    // 讨厌物品
    if (!npc.disliked_gifts.empty()) {
        current_y += 40.0f;
        sf::RectangleShape dislike_bg({content_width, 28.0f});
        dislike_bg.setPosition({x + 8.0f, current_y});
        dislike_bg.setFillColor(ColorPalette::Lighten(ColorPalette::LightGray, 2));
        dislike_bg.setOutlineThickness(1.0f);
        dislike_bg.setOutlineColor(sf::Color(180, 180, 180, 180));
        window.draw(dislike_bg);

        m_font_renderer->DrawText(window, "讨厌物品: " + npc.disliked_gifts, 
            {x + 12.0f, current_y + 5.0f}, TextStyle::Default());
    }

    // 已触发事件列表
    current_y += row_h * 1.5f;
    m_font_renderer->DrawText(window, "已触发事件:", {x + 8.0f, current_y}, TextStyle::Default());

    current_y += row_h * 0.9f;
    float event_y = current_y;
    int event_col = 0;
    const float event_col_width = 140.0f;

    for (const auto& event : npc.triggered_events) {
        if (event_col >= 4 || event_y + row_h > detail_area.position.y + detail_area.size.y - 10.0f) {
            break;
        }
        sf::RectangleShape event_chip({event_col_width - 8.0f, 20.0f});
        event_chip.setPosition({x + 8.0f + event_col * event_col_width, event_y});
        event_chip.setFillColor(ColorPalette::Lighten(ColorPalette::ActiveGreen, 3));
        event_chip.setOutlineThickness(1.0f);
        event_chip.setOutlineColor(ColorPalette::ActiveGreen);
        window.draw(event_chip);

        m_font_renderer->DrawText(window, kStarFilled + event, 
            {x + 12.0f + event_col * event_col_width, event_y + 3.0f}, 
            TextStyle::TopRightInfo());

        event_col++;
        if (event_col >= 4) {
            event_col = 0;
            event_y += 24.0f;
        }
    }

    // 下一事件提示
    if (!npc.next_event_hint.empty()) {
        float hint_y = detail_area.position.y + detail_area.size.y - 40.0f;
        sf::RectangleShape hint_bg({content_width, 28.0f});
        hint_bg.setPosition({x + 8.0f, hint_y});
        hint_bg.setFillColor(ColorPalette::Lighten(ColorPalette::LightBlue, 2));
        hint_bg.setOutlineThickness(1.0f);
        hint_bg.setOutlineColor(ColorPalette::LightBlue);
        window.draw(hint_bg);

        m_font_renderer->DrawText(window, "下一事件: " + npc.next_event_hint, 
            {x + 12.0f, hint_y + 5.0f}, TextStyle::HotkeyHint());
    }
}

void PixelSocialPanel::RenderHeartProgress_(sf::RenderWindow& window, float x, float y, float width,
                                           const std::vector<HeartLevelInfo>& levels, int current_level) {
    const float heart_width = 22.0f;
    const float heart_height = 20.0f;
    const float spacing = 2.0f;
    const int total_hearts = 10;

    float current_x = x;
    for (int i = 0; i < total_hearts; ++i) {
        bool filled = (i < current_level);
        bool current = (i == current_level - 1);

        // 绘制心形背景
        sf::RectangleShape heart_bg({heart_width, heart_height});
        heart_bg.setPosition({current_x, y});
        
        if (filled) {
            heart_bg.setFillColor(kHeartColors[std::min(i + 1, 10)]);
        } else {
            heart_bg.setFillColor(ColorPalette::LightGray);
        }
        
        if (current) {
            heart_bg.setOutlineThickness(2.0f);
            heart_bg.setOutlineColor(sf::Color(255, 255, 100, 255));
        } else {
            heart_bg.setOutlineThickness(1.0f);
            heart_bg.setOutlineColor(ColorPalette::BrownOutline);
        }
        window.draw(heart_bg);

        // 绘制心图标
        TextStyle heart_style = TextStyle::Default();
        heart_style.fill_color = filled 
            ? sf::Color(255, 255, 255, 255) 
            : ColorPalette::Gray;
        heart_style.character_size = 14;
        
        m_font_renderer->DrawText(window, filled ? kHeartFilled : kHeartEmpty, 
            {current_x + 5.0f, y + 3.0f}, heart_style);

        current_x += heart_width + spacing;
    }

    // 等级名称
    TextStyle level_style = TextStyle::Default();
    level_style.fill_color = kHeartColors[std::min(current_level, 10)];
    level_style.character_size = 12;
    
    std::string level_name;
    if (current_level < static_cast<int>(kHeartLevelNames.size())) {
        level_name = kHeartLevelNames[current_level].second;
    }
    m_font_renderer->DrawText(window, "Lv." + std::to_string(current_level) + " " + level_name, 
        {x + width - 100.0f, y}, level_style);
}

}  // namespace CloudSeamanor::engine
