#include "CloudSeamanor/engine/BattleUI.hpp"

#include "CloudSeamanor/CloudSystem.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// 【Initialize】初始化UI
// ============================================================================
void BattleUI::Initialize(
    const sf::Font& font,
    unsigned int window_width,
    unsigned int window_height
) {
    font_ = &font;
    window_width_ = window_width;
    window_height_ = window_height;

    // ---- 顶部状态栏 ----
    top_bar_bg_.setSize({static_cast<float>(window_width), 36.0f});
    top_bar_bg_.setFillColor({0, 0, 0, 180});

    weather_text_.setFont(font);
    weather_text_.setCharacterSize(16);
    weather_text_.setFillColor({220, 220, 255});
    weather_text_.setPosition(16.0f, 8.0f);

    battle_time_text_.setFont(font);
    battle_time_text_.setCharacterSize(16);
    battle_time_text_.setFillColor({220, 220, 255});
    battle_time_text_.setPosition(200.0f, 8.0f);

    pause_button_.setSize({60.0f, 24.0f});
    pause_button_.setPosition({static_cast<float>(window_width) - 130.0f, 6.0f});
    pause_button_.setFillColor({80, 80, 120});

    retreat_button_.setSize({60.0f, 24.0f});
    retreat_button_.setPosition({static_cast<float>(window_width) - 60.0f, 6.0f});
    retreat_button_.setFillColor({160, 60, 60});

    // ---- 能量条 ----
    energy_bar_bg_.setSize({300.0f, 20.0f});
    energy_bar_bg_.setPosition({20.0f, static_cast<float>(window_height) - 80.0f});
    energy_bar_bg_.setFillColor({40, 40, 60});

    energy_bar_fill_.setSize({300.0f, 20.0f});
    energy_bar_fill_.setPosition({20.0f, static_cast<float>(window_height) - 80.0f});
    energy_bar_fill_.setFillColor({60, 180, 255});

    energy_text_.setFont(font);
    energy_text_.setCharacterSize(14);
    energy_text_.setFillColor({255, 255, 255});
    energy_text_.setPosition(24.0f, static_cast<float>(window_height) - 77.0f);

    // ---- 技能按钮（4个） ----
    skill_buttons_.resize(kSkillButtonCount);
    for (int i = 0; i < kSkillButtonCount; ++i) {
        auto& btn = skill_buttons_[i];
        btn.background.setSize({70.0f, 70.0f});
        btn.background.setPosition({
            340.0f + i * 80.0f,
            static_cast<float>(window_height) - 80.0f
        });
        btn.background.setFillColor({60, 60, 100});
        btn.background.setOutlineThickness(2.0f);
        btn.background.setOutlineColor({120, 120, 200});

        btn.cooldown_overlay.setSize({70.0f, 70.0f});
        btn.cooldown_overlay.setFillColor({0, 0, 0, 180});

        btn.key_label.setFont(font);
        btn.key_label.setCharacterSize(14);
        btn.key_label.setFillColor({255, 255, 255});
        const char* keys[] = {"Q", "W", "E", "R"};
        btn.key_label.setString(keys[i]);
        btn.key_label.setPosition({
            350.0f + i * 80.0f,
            static_cast<float>(window_height) - 72.0f
        });

        btn.cooldown_label.setFont(font);
        btn.cooldown_label.setCharacterSize(12);
        btn.cooldown_label.setFillColor({200, 200, 200});
        btn.cooldown_label.setPosition({
            350.0f + i * 80.0f,
            static_cast<float>(window_height) - 55.0f
        });

        btn.name_label.setFont(font);
        btn.name_label.setCharacterSize(11);
        btn.name_label.setFillColor({200, 200, 255});
        btn.name_label.setPosition({
            340.0f + i * 80.0f,
            static_cast<float>(window_height) - 42.0f
        });

        btn.energy_cost_label.setFont(font);
        btn.energy_cost_label.setCharacterSize(11);
        btn.energy_cost_label.setFillColor({180, 180, 100});
        btn.energy_cost_label.setPosition({
            350.0f + i * 80.0f,
            static_cast<float>(window_height) - 30.0f
        });

        btn.slot_index = i;
        btn.is_ready = false;
        btn.is_locked = false;
    }

    // ---- 战斗日志 ----
    log_panel_bg_.setSize({320.0f, 150.0f});
    log_panel_bg_.setPosition({20.0f, 50.0f});
    log_panel_bg_.setFillColor({0, 0, 0, 150});

    log_lines_.resize(kMaxLogLines);
    for (int i = 0; i < kMaxLogLines; ++i) {
        log_lines_[i].setFont(font);
        log_lines_[i].setCharacterSize(13);
        log_lines_[i].setFillColor({200, 200, 200});
        log_lines_[i].setPosition(28.0f, 54.0f + i * 22.0f);
    }

    // ---- 暂停菜单 ----
    pause_menu_visible_ = false;
    pause_menu_bg_.setSize({400.0f, 300.0f});
    pause_menu_bg_.setPosition({440.0f, 210.0f});
    pause_menu_bg_.setFillColor({20, 20, 40, 230});

    pause_title_.setFont(font);
    pause_title_.setCharacterSize(28);
    pause_title_.setFillColor({255, 255, 255});
    pause_title_.setString("战斗暂停");
    pause_title_.setPosition(560.0f, 230.0f);

    resume_button_.setSize({160.0f, 40.0f});
    resume_button_.setPosition({560.0f, 290.0f});
    resume_button_.setFillColor({60, 100, 180});

    resume_text_.setFont(font);
    resume_text_.setCharacterSize(18);
    resume_text_.setFillColor({255, 255, 255});
    resume_text_.setString("继续战斗");
    resume_text_.setPosition(580.0f, 298.0f);

    quit_button_.setSize({160.0f, 40.0f});
    quit_button_.setPosition({560.0f, 350.0f});
    quit_button_.setFillColor({160, 60, 60});

    quit_text_.setFont(font);
    quit_text_.setCharacterSize(18);
    quit_text_.setFillColor({255, 255, 255});
    quit_text_.setString("暂避锋芒");
    quit_text_.setPosition(580.0f, 358.0f);

    // ---- 结算面板 ----
    result_panel_visible_ = false;
    result_panel_bg_.setSize({500.0f, 400.0f});
    result_panel_bg_.setPosition({390.0f, 160.0f});
    result_panel_bg_.setFillColor({10, 10, 30, 240});

    result_title_.setFont(font);
    result_title_.setCharacterSize(36);
    result_title_.setFillColor({255, 215, 100});
    result_title_.setPosition(550.0f, 180.0f);

    result_lines_.reserve(8);
}

// ============================================================================
// 【Update】每帧更新
// ============================================================================
void BattleUI::Update(float delta_seconds) {
    UpdateEnergyBar_();
    UpdateSkillButtons_();

    if (result_panel_visible_) {
        UpdateResultPanel_(delta_seconds);
    }
}

// ============================================================================
// 【Draw】渲染
// ============================================================================
void BattleUI::Draw(sf::RenderWindow& window) const {
    DrawTopBar_(window);
    DrawHealthBars_(window);
    DrawEnergyBar_(window);
    DrawSkillButtons_(window);
    DrawPartnerBars_(window);
    DrawLogPanel_(window);

    if (pause_menu_visible_) {
        DrawPauseMenu_(window);
    }
    if (result_panel_visible_) {
        DrawResultPanel_(window);
    }
}

// ============================================================================
// 【SyncFromField】从战场同步数据
// ============================================================================
void BattleUI::SyncFromField(const BattleField& field) {
    // 同步天气文字
    weather_text_.setString("天气：" + GetWeatherText_(field.GetWeatherMultiplier() > 1.2f
        ? CloudSeamanor::domain::CloudState::Mist
        : CloudSeamanor::domain::CloudState::Clear)
        + " x" + std::to_string(field.GetWeatherMultiplier()));

    // 同步战斗时间
    int seconds = static_cast<int>(field.ElapsedTime());
    int ms = static_cast<int>((field.ElapsedTime() - seconds) * 10);
    battle_time_text_.setString(
        "战斗时间：" + std::to_string(seconds / 60) + ":" +
        (seconds % 60 < 10 ? "0" : "") + std::to_string(seconds % 60)
    );

    // 同步能量条
    const auto& ps = field.GetPlayerState();
    float ratio = ps.current_energy / ps.max_energy;
    energy_bar_fill_.setSize({300.0f * ratio, 20.0f});
    energy_text_.setString(
        std::to_string(static_cast<int>(ps.current_energy)) + "/" +
        std::to_string(static_cast<int>(ps.max_energy)) + " 能量"
    );

    // 同步灵体血条
    health_bars_.clear();
    for (const auto& spirit : field.GetSpirits()) {
        if (spirit.is_defeated) continue;
        SpiritHealthBar bar;
        bar.spirit_id = spirit.id;
        bar.background.setSize({120.0f, 10.0f});
        bar.background.setFillColor({40, 40, 60});
        bar.background.setPosition({spirit.pos_x - 60.0f, spirit.pos_y - 50.0f});

        float p_ratio = spirit.current_pollution / spirit.max_pollution;
        bar.pollution_fill.setSize({120.0f * p_ratio, 10.0f});
        bar.pollution_fill.setFillColor({180, 40, 40});
        bar.pollution_fill.setPosition({spirit.pos_x - 60.0f, spirit.pos_y - 50.0f});

        bar.name_label.setFont(*font_);
        bar.name_label.setCharacterSize(12);
        bar.name_label.setFillColor({255, 200, 200});
        bar.name_label.setString(spirit.name);
        bar.name_label.setPosition({spirit.pos_x - 60.0f, spirit.pos_y - 68.0f});

        health_bars_.push_back(bar);
    }

    // 同步日志
    const auto& logs = field.GetLog();
    for (int i = 0; i < kMaxLogLines && i < static_cast<int>(logs.size()); ++i) {
        const auto& log = logs[logs.size() - kMaxLogLines + i];
        log_lines_[i].setString(log.message);
        log_lines_[i].setFillColor(log.is_important
            ? sf::Color(255, 215, 100)
            : (log.is_player_action ? sf::Color(200, 220, 255) : sf::Color(200, 200, 200)));
    }
}

// ============================================================================
// 【AddLogEntry】添加日志
// ============================================================================
void BattleUI::AddLogEntry(const std::string& message, bool is_player_action) {
    (void)is_player_action;
    // 日志由 SyncFromField 从 field 同步，此方法仅用于离线添加
}

// ============================================================================
// 【ShowResultPanel】显示结算面板
// ============================================================================
void BattleUI::ShowResultPanel(const BattleResult& result, bool victory) {
    result_panel_visible_ = true;
    cached_result_ = result;
    cached_victory_ = victory;
    result_display_timer_ = 0.0f;

    result_lines_.clear();
    result_title_.setString(victory ? "★★ 战斗胜利！★★" : "暂避锋芒");

    auto addLine = [&](const std::string& text, unsigned char r, unsigned char g, unsigned char b) {
        sf::Text t;
        t.setFont(*font_);
        t.setCharacterSize(18);
        t.setFillColor({r, g, b});
        t.setString(text);
        result_lines_.push_back(t);
    };

    addLine("净化灵体：" + std::to_string(result.spirits_purified) + "/" + std::to_string(result.spirits_total), 200, 220, 255);
    addLine("战斗时长：" + std::to_string(static_cast<int>(result.battle_duration)) + "秒", 200, 220, 255);
    addLine("净化经验：+" + std::to_string(static_cast<int>(result.total_exp_gained)), 100, 255, 150);

    if (!result.items_gained.empty()) {
        addLine("获得物品：" + std::to_string(result.items_gained.size()) + "件", 255, 200, 100);
    }
}

// ============================================================================
// 【SetPauseMenuVisible】暂停菜单
// ============================================================================
void BattleUI::SetPauseMenuVisible(bool visible) {
    pause_menu_visible_ = visible;
}

// ============================================================================
// 【OnMouseMove】鼠标移动
// ============================================================================
void BattleUI::OnMouseMove(float mouse_x, float mouse_y) {
    for (auto& btn : skill_buttons_) {
        btn.is_hovered = btn.background.getGlobalBounds().contains(mouse_x, mouse_y);
    }
}

// ============================================================================
// 【OnMouseClick】鼠标点击
// ============================================================================
int BattleUI::OnMouseClick(float mouse_x, float mouse_y) {
    for (const auto& btn : skill_buttons_) {
        if (btn.background.getGlobalBounds().contains(mouse_x, mouse_y)) {
            if (btn.is_ready && !btn.is_locked) {
                return btn.slot_index;
            }
        }
    }
    return -1;
}

// ============================================================================
// 【OnKeyPressed】按键按下
// ============================================================================
int BattleUI::OnKeyPressed(int key_code) {
    // Q=0, W=1, E=2, R=3 对应 sf::Keyboard::Q/W/E/R
    if (key_code >= 0 && key_code < kSkillButtonCount) {
        const auto& btn = skill_buttons_[key_code];
        if (btn.is_ready && !btn.is_locked) {
            return key_code;
        }
    }
    return -1;
}

// ============================================================================
// 私有方法
// ============================================================================

void BattleUI::UpdateEnergyBar_() {
    // 能量条颜色渐变：满=蓝，耗尽=红
}

void BattleUI::UpdateSkillButtons_() {
    // 更新冷却遮罩和文字
    for (auto& btn : skill_buttons_) {
        if (btn.cooldown_remaining > 0.0f) {
            btn.is_ready = false;
            float ratio = btn.cooldown_remaining / std::max(btn.cooldown_total, 1.0f);
            btn.cooldown_overlay.setSize({70.0f * ratio, 70.0f});
            btn.cooldown_label.setString(std::to_string(static_cast<int>(btn.cooldown_remaining)) + "s");
            btn.background.setFillColor({40, 40, 70});
        } else {
            btn.is_ready = !btn.is_locked;
            btn.cooldown_overlay.setSize({0.0f, 70.0f});
            btn.cooldown_label.setString("");
            btn.background.setFillColor(btn.is_ready ? sf::Color(60, 60, 100) : sf::Color(30, 30, 50));
        }
    }
}

void BattleUI::UpdateResultPanel_(float delta_seconds) {
    result_display_timer_ += delta_seconds;
}

void BattleUI::DrawTopBar_(sf::RenderWindow& window) const {
    window.draw(top_bar_bg_);
    window.draw(weather_text_);
    window.draw(battle_time_text_);
    window.draw(pause_button_);
    window.draw(retreat_button_);
}

void BattleUI::DrawHealthBars_(sf::RenderWindow& window) const {
    for (const auto& bar : health_bars_) {
        window.draw(bar.background);
        window.draw(bar.pollution_fill);
        window.draw(bar.name_label);
    }
}

void BattleUI::DrawEnergyBar_(sf::RenderWindow& window) const {
    window.draw(energy_bar_bg_);
    window.draw(energy_bar_fill_);
    window.draw(energy_text_);
}

void BattleUI::DrawSkillButtons_(sf::RenderWindow& window) const {
    for (const auto& btn : skill_buttons_) {
        window.draw(btn.background);
        window.draw(btn.cooldown_overlay);
        window.draw(btn.key_label);
        window.draw(btn.cooldown_label);
        window.draw(btn.name_label);
        window.draw(btn.energy_cost_label);
    }
}

void BattleUI::DrawPartnerBars_(sf::RenderWindow& window) const {
    // 灵兽伙伴状态栏（屏幕左侧）
}

void BattleUI::DrawLogPanel_(sf::RenderWindow& window) const {
    window.draw(log_panel_bg_);
    for (const auto& line : log_lines_) {
        window.draw(line);
    }
}

void BattleUI::DrawPauseMenu_(sf::RenderWindow& window) const {
    window.draw(pause_menu_bg_);
    window.draw(pause_title_);
    window.draw(resume_button_);
    window.draw(resume_text_);
    window.draw(quit_button_);
    window.draw(quit_text_);
}

void BattleUI::DrawResultPanel_(sf::RenderWindow& window) const {
    window.draw(result_panel_bg_);
    window.draw(result_title_);
    for (size_t i = 0; i < result_lines_.size(); ++i) {
        result_lines_[i].setPosition(410.0f, 240.0f + i * 35.0f);
        window.draw(result_lines_[i]);
    }
}

sf::Color BattleUI::GetHealthBarColor_(float pollution_ratio) const {
    if (pollution_ratio > 0.75f) return {200, 40, 40};
    if (pollution_ratio > 0.5f) return {200, 120, 40};
    if (pollution_ratio > 0.25f) return {200, 200, 40};
    return {100, 220, 100};
}

std::string BattleUI::GetWeatherText_(CloudSeamanor::domain::CloudState state) const {
    switch (state) {
        case CloudSeamanor::domain::CloudState::Clear: return "晴";
        case CloudSeamanor::domain::CloudState::Mist: return "薄雾";
        case CloudSeamanor::domain::CloudState::DenseCloud: return "浓云海";
        case CloudSeamanor::domain::CloudState::Tide: return "大潮";
    }
    return "晴";
}

}  // namespace CloudSeamanor::engine
