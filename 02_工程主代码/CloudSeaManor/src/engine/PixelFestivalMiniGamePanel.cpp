#include "CloudSeamanor/engine/PixelFestivalMiniGamePanel.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【构造函数】
// ============================================================================
PixelFestivalMiniGamePanel::PixelFestivalMiniGamePanel()
    : PixelUiPanel({{300.0f, 100.0f}, {640.0f, 480.0f}}, "节日小游戏", false) {
}

// ============================================================================
// 【SetFontRenderer】
// ============================================================================
void PixelFestivalMiniGamePanel::SetFontRenderer(const PixelFontRenderer* renderer) {
    m_font_renderer = renderer;
}

// ============================================================================
// 【StartGame】
// ============================================================================
void PixelFestivalMiniGamePanel::StartGame(domain::MiniGameType type) {
    current_game_ = type;
    domain::FestivalMiniGameSystem::Instance().StartGame(type);
    game_running_ = true;
    game_ended_ = false;
    Open();
}

// ============================================================================
// 【Update】
// ============================================================================
void PixelFestivalMiniGamePanel::Update(float delta_seconds) {
    if (!game_running_) return;

    auto& system = domain::FestivalMiniGameSystem::Instance();
    if (!system.Update(delta_seconds)) {
        // 游戏结束
        result_ = system.EndGame();
        game_running_ = false;
        game_ended_ = true;
    }
}

// ============================================================================
// 【HandleInput】
// ============================================================================
bool PixelFestivalMiniGamePanel::HandleInput(int key) {
    if (!game_running_) return false;

    return domain::FestivalMiniGameSystem::Instance().HandleInput(key);
}

// ============================================================================
// 【HandleClick】
// ============================================================================
bool PixelFestivalMiniGamePanel::HandleClick(float x, float y) {
    if (game_ended_) {
        // 结果界面点击关闭
        auto rect = GetWorldRect();
        float cx = rect.position.x + rect.size.x / 2.0f;
        float cy = rect.position.y + rect.size.y / 2.0f;

        // 检查是否点击继续按钮
        float btn_x = cx - 60.0f;
        float btn_y = cy + 60.0f;
        if (x >= btn_x && x <= btn_x + 120.0f && y >= btn_y && y <= btn_y + 40.0f) {
            Close();
            return true;
        }
        return false;
    }

    if (!game_running_) return false;

    return domain::FestivalMiniGameSystem::Instance().HandleClick(x, y);
}

// ============================================================================
// 【IsGameRunning】
// ============================================================================
bool PixelFestivalMiniGamePanel::IsGameRunning() const {
    return game_running_;
}

// ============================================================================
// 【IsGameEnded】
// ============================================================================
bool PixelFestivalMiniGamePanel::IsGameEnded() const {
    return game_ended_;
}

// ============================================================================
// 【GetResult】
// ============================================================================
const domain::GameResult& PixelFestivalMiniGamePanel::GetResult() const {
    return result_;
}

// ============================================================================
// 【RenderContent】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;

    // 渲染游戏内容
    switch (current_game_) {
        case domain::MiniGameType::CatchMooncake:
            RenderCatchMooncakeGame(window, inner_rect);
            break;
        case domain::MiniGameType::GuessFlower:
            RenderGuessFlowerGame(window, inner_rect);
            break;
        case domain::MiniGameType::FlyKite:
            RenderFlyKiteGame(window, inner_rect);
            break;
        case domain::MiniGameType::HarvestTea:
            RenderHarvestTeaGame(window, inner_rect);
            break;
        case domain::MiniGameType::TeaCeremony:
            RenderTeaCeremonyGame(window, inner_rect);
            break;
        default:
            break;
    }

    // 渲染结果
    if (game_ended_) {
        RenderResult(window, inner_rect);
    }
}

// ============================================================================
// 【RenderGameHeader】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderGameHeader(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect,
    float x, float y
) {
    if (m_font_renderer == nullptr) return;

    std::string game_name;
    switch (current_game_) {
        case domain::MiniGameType::CatchMooncake: game_name = "吃月饼"; break;
        case domain::MiniGameType::GuessFlower: game_name = "猜花"; break;
        case domain::MiniGameType::FlyKite: game_name = "放风筝"; break;
        case domain::MiniGameType::HarvestTea: game_name = "采茶大赛"; break;
        case domain::MiniGameType::TeaCeremony: game_name = "茶艺表演"; break;
        default: game_name = "小游戏"; break;
    }

    m_font_renderer->DrawText(
        window,
        "【" + game_name + "】",
        {x, y},
        TextStyle::PanelTitle()
    );
}

// ============================================================================
// 【RenderTimer】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderTimer(
    sf::RenderWindow& window,
    float x, float y,
    float time_left,
    float max_time
) {
    if (m_font_renderer == nullptr) return;

    // 计时器背景
    sf::RectangleShape timer_bg({200.0f, 24.0f});
    timer_bg.setPosition({x, y});
    timer_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
    timer_bg.setOutlineThickness(1.0f);
    timer_bg.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(timer_bg);

    // 计时器填充
    float ratio = std::clamp(time_left / max_time, 0.0f, 1.0f);
    float fill_width = 196.0f * ratio;

    sf::RectangleShape timer_fill({fill_width, 20.0f});
    timer_fill.setPosition({x + 2.0f, y + 2.0f});

    if (ratio > 0.5f) {
        timer_fill.setFillColor(ColorPalette::FreshGreen);
    } else if (ratio > 0.25f) {
        timer_fill.setFillColor(ColorPalette::FireOrange);
    } else {
        timer_fill.setFillColor(ColorPalette::FireRed);
    }
    window.draw(timer_fill);

    // 计时器文字
    std::string time_text = std::to_string(static_cast<int>(time_left)) + "s";
    TextStyle time_style = TextStyle::Default();
    time_style.fill_color = ColorPalette::DeepBrown;
    m_font_renderer->DrawText(window, time_text, {x + 80.0f, y + 2.0f}, time_style);
}

// ============================================================================
// 【RenderScore】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderScore(
    sf::RenderWindow& window,
    float x, float y,
    int score
) {
    if (m_font_renderer == nullptr) return;

    std::string score_text = "分数: " + std::to_string(score);
    m_font_renderer->DrawText(window, score_text, {x, y}, TextStyle::CoinText());
}

// ============================================================================
// 【RenderCatchMooncakeGame】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderCatchMooncakeGame(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float x = inner_rect.position.x + kGameAreaPadding;
    float y = inner_rect.position.y + kGameAreaPadding;

    auto& system = domain::FestivalMiniGameSystem::Instance();

    // 标题
    RenderGameHeader(window, inner_rect, x, y);
    y += 30.0f;

    // 计时器和分数
    RenderTimer(window, x, y, system.GetTimeLeft(), 30.0f);
    RenderScore(window, x + 220.0f, y, system.GetScore());
    y += 40.0f;

    // 游戏区域背景
    sf::RectangleShape game_area({
        inner_rect.size.x - kGameAreaPadding * 2,
        inner_rect.size.y - kInfoHeight - 60.0f
    });
    game_area.setPosition({x, y});
    game_area.setFillColor(ColorPalette::Lighten(ColorPalette::SkyBlue, 30));
    game_area.setOutlineThickness(2.0f);
    game_area.setOutlineColor(ColorPalette::OceanBlue);
    window.draw(game_area);

    // 渲染落下的月饼
    for (const auto& mooncake : system.GetFallingMooncakes()) {
        sf::CircleShape shape(15.0f);
        shape.setPosition({
            x + mooncake.first * game_area.getSize().x - 15.0f,
            y + mooncake.second * game_area.getSize().y
        });
        shape.setFillColor(ColorPalette::FireOrange);
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(ColorPalette::DeepBrown);
        window.draw(shape);
    }

    // 渲染篮子
    float basket_x = x + system.GetBasketX() * game_area.getSize().x - 30.0f;
    sf::RectangleShape basket({60.0f, 30.0f});
    basket.setPosition({basket_x, y + game_area.getSize().y - 35.0f});
    basket.setFillColor(ColorPalette::WoodBrown);
    basket.setOutlineThickness(2.0f);
    basket.setOutlineColor(ColorPalette::DeepBrown);
    window.draw(basket);

    // 提示
    y += game_area.getSize().y + 10.0f;
    if (m_font_renderer) {
        m_font_renderer->DrawText(
            window,
            "点击落下的月饼！",
            {x, y},
            TextStyle::HotkeyHint()
        );
    }
}

// ============================================================================
// 【RenderGuessFlowerGame】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderGuessFlowerGame(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float x = inner_rect.position.x + kGameAreaPadding;
    float y = inner_rect.position.y + kGameAreaPadding;

    auto& system = domain::FestivalMiniGameSystem::Instance();
    const auto& state = system.GetFlowerState();

    // 标题
    RenderGameHeader(window, inner_rect, x, y);
    y += 30.0f;

    // 计时器
    RenderTimer(window, x, y, system.GetTimeLeft(), 30.0f);
    y += 40.0f;

    // 显示当前轮次
    if (m_font_renderer) {
        std::string round_text = "第 " + std::to_string(state.current_round + 1) +
                                 " / " + std::to_string(state.round_limit) + " 轮";
        m_font_renderer->DrawText(window, round_text, {x, y}, TextStyle::PanelTitle());
    }
    y += 40.0f;

    // 5盆花
    const char* flower_colors[] = {"红", "橙", "黄", "绿", "紫"};
    sf::Color flower_color_list[] = {
        ColorPalette::FireRed,
        ColorPalette::FireOrange,
        ColorPalette::CoinGold,
        ColorPalette::FreshGreen,
        ColorPalette::RoyalPurple
    };

    float flower_width = (inner_rect.size.x - kGameAreaPadding * 2 - 40.0f) / 5.0f;
    for (int i = 0; i < 5; ++i) {
        float fx = x + 20.0f + i * flower_width;

        // 花盆
        sf::RectangleShape pot({flower_width - 10.0f, 60.0f});
        pot.setPosition({fx, y + 100.0f});
        pot.setFillColor(ColorPalette::WoodBrown);
        pot.setOutlineThickness(2.0f);
        pot.setOutlineColor(ColorPalette::DeepBrown);
        window.draw(pot);

        // 花朵
        sf::CircleShape flower({25.0f});
        flower.setPosition({fx + flower_width / 2.0f - 25.0f, y + 50.0f});
        flower.setFillColor(flower_color_list[i]);
        window.draw(flower);

        // 编号
        if (m_font_renderer) {
            m_font_renderer->DrawText(
                window,
                std::to_string(i + 1),
                {fx + flower_width / 2.0f - 8.0f, y + 110.0f},
                TextStyle::Default()
            );
        }
    }

    // 提示
    y += 180.0f;
    if (m_font_renderer) {
        m_font_renderer->DrawText(
            window,
            "记住花朵顺序，点击对应编号！",
            {x, y},
            TextStyle::HotkeyHint()
        );
    }
}

// ============================================================================
// 【RenderFlyKiteGame】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderFlyKiteGame(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float x = inner_rect.position.x + kGameAreaPadding;
    float y = inner_rect.position.y + kGameAreaPadding;

    auto& system = domain::FestivalMiniGameSystem::Instance();
    const auto& state = system.GetKiteState();

    // 标题
    RenderGameHeader(window, inner_rect, x, y);
    y += 30.0f;

    // 计时器
    RenderTimer(window, x, y, system.GetTimeLeft(), 20.0f);

    // 平衡度
    if (m_font_renderer) {
        std::string balance_text = "平衡: " + std::to_string(static_cast<int>(state.balance * 100)) + "%";
        m_font_renderer->DrawText(window, balance_text, {x + 220.0f, y}, TextStyle::Default());
    }
    y += 40.0f;

    // 游戏区域
    sf::RectangleShape game_area({
        inner_rect.size.x - kGameAreaPadding * 2,
        inner_rect.size.y - kInfoHeight - 60.0f
    });
    game_area.setPosition({x, y});
    game_area.setFillColor(ColorPalette::Lighten(ColorPalette::SkyBlue, 20));
    window.draw(game_area);

    // 背景云朵（装饰）
    for (int i = 0; i < 5; ++i) {
        sf::CircleShape cloud({30.0f + i * 5});
        cloud.setPosition({
            x + 50.0f + i * 100.0f,
            y + 20.0f + (i % 3) * 80.0f
        });
        cloud.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 10));
        window.draw(cloud);
    }

    // 风筝
    float kite_x = x + system.GetKiteX() * game_area.getSize().x;
    float kite_y = y + state.kite_y * game_area.getSize().y;

    sf::CircleShape kite({20.0f});
    kite.setPosition({kite_x - 20.0f, kite_y - 20.0f});
    kite.setFillColor(ColorPalette::FireRed);
    kite.setOutlineThickness(2.0f);
    kite.setOutlineColor(ColorPalette::DeepBrown);
    window.draw(kite);

    // 风筝线
    sf::VertexArray line(sf::LineStrip, 3);
    line[0].position = {x + inner_rect.size.x / 2.0f, y + game_area.getSize().y};
    line[1].position = {kite_x, kite_y + 20.0f};
    line[2].position = {kite_x, kite_y};
    line[0].color = sf::Color(100, 100, 100);
    line[1].color = sf::Color(100, 100, 100);
    line[2].color = ColorPalette::DeepBrown;
    window.draw(line);

    // 提示
    y += game_area.getSize().y + 10.0f;
    if (m_font_renderer) {
        m_font_renderer->DrawText(
            window,
            "使用 ← → 键控制风筝！",
            {x, y},
            TextStyle::HotkeyHint()
        );
    }
}

// ============================================================================
// 【RenderHarvestTeaGame】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderHarvestTeaGame(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float x = inner_rect.position.x + kGameAreaPadding;
    float y = inner_rect.position.y + kGameAreaPadding;

    auto& system = domain::FestivalMiniGameSystem::Instance();
    const auto& state = system.GetHarvestState();

    // 标题
    RenderGameHeader(window, inner_rect, x, y);
    y += 30.0f;

    // 计时器
    RenderTimer(window, x, y, system.GetTimeLeft(), 60.0f);
    y += 40.0f;

    // 进度条
    sf::RectangleShape progress_bg({300.0f, 24.0f});
    progress_bg.setPosition({x, y});
    progress_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
    progress_bg.setOutlineThickness(1.0f);
    progress_bg.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(progress_bg);

    float progress_fill = 296.0f * state.progress;
    sf::RectangleShape progress({progress_fill, 20.0f});
    progress.setPosition({x + 2.0f, y + 2.0f});
    progress.setFillColor(ColorPalette::FreshGreen);
    window.draw(progress);

    // 进度文字
    if (m_font_renderer) {
        std::string progress_text = std::to_string(state.tea_harvested) + " / " + std::to_string(state.target_tea);
        m_font_renderer->DrawText(window, progress_text, {x + 110.0f, y + 2.0f}, TextStyle::Default());
    }
    y += 40.0f;

    // 茶园区域
    sf::RectangleShape garden({
        inner_rect.size.x - kGameAreaPadding * 2,
        inner_rect.size.y - kInfoHeight - 80.0f
    });
    garden.setPosition({x, y});
    garden.setFillColor(ColorPalette::Lighten(ColorPalette::ForestGreen, 20));
    garden.setOutlineThickness(2.0f);
    garden.setOutlineColor(ColorPalette::ForestGreen);
    window.draw(garden);

    // 茶叶（随机分布）
    std::mt19937 rng(static_cast<unsigned>(state.tea_harvested + state.current_target));
    for (int i = 0; i < 15; ++i) {
        std::uniform_real_distribution<float> dist(0.1f, 0.9f);
        float tx = x + dist(rng) * garden.getSize().x;
        float ty = y + dist(rng) * garden.getSize().y;

        sf::CircleShape tea({12.0f});
        tea.setPosition({tx - 12.0f, ty - 12.0f});
        tea.setFillColor(ColorPalette::FreshGreen);
        tea.setOutlineThickness(1.0f);
        tea.setOutlineColor(ColorPalette::ForestGreen);
        window.draw(tea);
    }

    // 提示
    y += garden.getSize().y + 10.0f;
    if (m_font_renderer) {
        m_font_renderer->DrawText(
            window,
            "点击茶树采集茶叶！",
            {x, y},
            TextStyle::HotkeyHint()
        );
    }
}

// ============================================================================
// 【RenderTeaCeremonyGame】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderTeaCeremonyGame(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float x = inner_rect.position.x + kGameAreaPadding;
    float y = inner_rect.position.y + kGameAreaPadding;

    auto& system = domain::FestivalMiniGameSystem::Instance();
    const auto& state = system.GetCeremonyState();

    // 标题
    RenderGameHeader(window, inner_rect, x, y);
    y += 30.0f;

    // 计时器
    RenderTimer(window, x, y, system.GetTimeLeft(), 45.0f);
    y += 40.0f;

    // 当前步骤
    if (m_font_renderer) {
        std::string step_text = "步骤: " + std::to_string(state.current_step + 1) + " / 5";
        m_font_renderer->DrawText(window, step_text, {x, y}, TextStyle::PanelTitle());
    }
    y += 30.0f;

    // 茶具选项
    const char* tea_items[] = {"茶壶", "茶杯", "茶盘", "茶匙", "茶罐"};
    float item_width = (inner_rect.size.x - kGameAreaPadding * 2 - 40.0f) / 5.0f;

    for (int i = 0; i < 5; ++i) {
        float ix = x + 20.0f + i * item_width;

        // 茶具框
        sf::RectangleShape item_bg({item_width - 10.0f, 80.0f});
        item_bg.setPosition({ix, y + 80.0f});
        item_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
        item_bg.setOutlineThickness(2.0f);
        item_bg.setOutlineColor(ColorPalette::BrownOutline);
        window.draw(item_bg);

        // 编号
        if (m_font_renderer) {
            m_font_renderer->DrawText(
                window,
                std::to_string(i + 1) + ". " + tea_items[i],
                {ix + 10.0f, y + 100.0f},
                TextStyle::Default()
            );
        }
    }

    // 提示
    y += 180.0f;
    if (m_font_renderer) {
        m_font_renderer->DrawText(
            window,
            "按正确顺序选择茶具！",
            {x, y},
            TextStyle::HotkeyHint()
        );
    }
}

// ============================================================================
// 【RenderResult】
// ============================================================================
void PixelFestivalMiniGamePanel::RenderResult(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    float cx = inner_rect.position.x + inner_rect.size.x / 2.0f;
    float cy = inner_rect.position.y + inner_rect.size.y / 2.0f;

    // 半透明遮罩
    sf::RectangleShape overlay({inner_rect.size.x, inner_rect.size.y});
    overlay.setPosition(inner_rect.position);
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);

    // 结果面板
    sf::RectangleShape result_panel({kResultPanelWidth, kResultPanelHeight});
    result_panel.setPosition({cx - kResultPanelWidth / 2.0f, cy - kResultPanelHeight / 2.0f});
    result_panel.setFillColor(ColorPalette::Cream);
    result_panel.setOutlineThickness(3.0f);
    result_panel.setOutlineColor(ColorPalette::CoinGold);
    window.draw(result_panel);

    if (m_font_renderer == nullptr) return;

    float ry = cy - kResultPanelHeight / 2.0f + 20.0f;

    // 评级
    TextStyle grade_style = TextStyle::PanelTitle();
    if (result_.rank == 1) {
        grade_style.fill_color = ColorPalette::CoinGold;
    } else if (result_.rank == 2) {
        grade_style.fill_color = ColorPalette::FreshGreen;
    } else if (result_.rank == 3) {
        grade_style.fill_color = ColorPalette::OceanBlue;
    } else {
        grade_style.fill_color = ColorPalette::StoneGray;
    }
    m_font_renderer->DrawText(window, "评级: " + result_.rank_text, cx - 60.0f, ry, grade_style);
    ry += 40.0f;

    // 分数
    m_font_renderer->DrawText(
        window,
        "分数: " + std::to_string(result_.score),
        cx - 60.0f, ry,
        TextStyle::Default()
    );
    ry += 30.0f;

    // 奖励
    m_font_renderer->DrawText(
        window,
        "奖励: " + std::to_string(result_.gold_reward) + " 金 + " + std::to_string(result_.item_count) + " 物品",
        cx - 100.0f, ry,
        TextStyle::CoinText()
    );
    ry += 50.0f;

    // 继续按钮
    float btn_x = cx - 60.0f;
    float btn_y = cy + 60.0f;
    sf::RectangleShape btn({120.0f, 40.0f});
    btn.setPosition({btn_x, btn_y});
    btn.setFillColor(ColorPalette::Lighten(ColorPalette::FreshGreen, 10));
    btn.setOutlineThickness(2.0f);
    btn.setOutlineColor(ColorPalette::FreshGreen);
    window.draw(btn);

    m_font_renderer->DrawText(window, "[ 继续 ]", {btn_x + 20.0f, btn_y + 8.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
