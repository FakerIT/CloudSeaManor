#include "CloudSeamanor/engine/BattleUI.hpp"

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// 【Helper】将 UTF-8 std::string 转换为 sf::String
// ============================================================================
static inline sf::String ToSfString(const std::string& str) {
    return CloudSeamanor::rendering::toSfString(str);
}

namespace detail {
const sf::Font& BattleUiFallbackFont() {
    static const sf::Font fallback_font = [] {
        sf::Font f;
        const std::array<std::string, 2> fallback_paths = {
            "assets/fonts/NotoSansSC-Regular.ttf",
            "assets/fonts/fallback.ttf"};
        for (const auto& path : fallback_paths) {
            if (f.openFromFile(path)) {
                break;
            }
        }
        return f;
    }();
    return fallback_font;
}
} // namespace detail

namespace {
sf::Color SegmentColor_(ElementType e, bool active) {
    sf::Color c{100, 100, 110};
    switch (e) {
    case ElementType::Metal: c = sf::Color(210, 230, 255); break; // cloud
    case ElementType::Earth: c = sf::Color(160, 220, 190); break; // wind
    case ElementType::Wood:  c = sf::Color(150, 240, 220); break; // dew
    case ElementType::Fire:  c = sf::Color(255, 170, 120); break; // glow
    case ElementType::Water: c = sf::Color(130, 170, 255); break; // tide
    case ElementType::Light: c = sf::Color(255, 240, 170); break;
    case ElementType::Dark:  c = sf::Color(170, 130, 220); break;
    default: break;
    }
    if (!active) {
        c.a = 90;
    }
    return c;
}

ElementType CounterElement_(ElementType target) {
    switch (target) {
    case ElementType::Metal: return ElementType::Earth; // wind > cloud
    case ElementType::Earth: return ElementType::Water; // tide > wind
    case ElementType::Wood:  return ElementType::Metal; // cloud > dew
    case ElementType::Fire:  return ElementType::Wood;  // dew > glow
    case ElementType::Water: return ElementType::Fire;  // glow > tide
    case ElementType::Light: return ElementType::Dark;
    case ElementType::Dark:  return ElementType::Light;
    default:                 return ElementType::Neutral;
    }
}

std::string ElementText_(ElementType e) {
    switch (e) {
    case ElementType::Metal: return "云";
    case ElementType::Earth: return "风";
    case ElementType::Wood:  return "露";
    case ElementType::Fire:  return "霞";
    case ElementType::Water: return "潮";
    case ElementType::Light: return "光";
    case ElementType::Dark:  return "暗";
    default:                 return "中性";
    }
}
} // namespace

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
    weather_text_.setPosition({16.0f, 8.0f});

    battle_time_text_.setFont(font);
    battle_time_text_.setCharacterSize(16);
    battle_time_text_.setFillColor({220, 220, 255});
    battle_time_text_.setPosition({200.0f, 8.0f});

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
    energy_text_.setPosition({24.0f, static_cast<float>(window_height) - 77.0f});

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
        btn.icon_placeholder.setSize({26.0f, 26.0f});
        btn.icon_placeholder.setPosition({
            362.0f + i * 80.0f,
            static_cast<float>(window_height) - 68.0f
        });
        btn.icon_placeholder.setFillColor({120, 190, 255});

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

    log_lines_.clear();
    log_lines_.reserve(kMaxLogLines);
    for (int i = 0; i < kMaxLogLines; ++i) {
        log_lines_.emplace_back(font);
        log_lines_.back().setCharacterSize(13);
        log_lines_.back().setFillColor({200, 200, 200});
        log_lines_.back().setPosition({28.0f, 54.0f + i * 22.0f});
    }

    // ---- 暂停菜单 ----
    pause_menu_visible_ = false;
    pause_menu_bg_.setSize({400.0f, 300.0f});
    pause_menu_bg_.setPosition({440.0f, 210.0f});
    pause_menu_bg_.setFillColor({20, 20, 40, 230});

    pause_title_.setFont(font);
    pause_title_.setCharacterSize(28);
    pause_title_.setFillColor({255, 255, 255});
    pause_title_.setString(ToSfString("战斗暂停"));
    pause_title_.setPosition({560.0f, 230.0f});

    resume_button_.setSize({160.0f, 40.0f});
    resume_button_.setPosition({560.0f, 290.0f});
    resume_button_.setFillColor({60, 100, 180});

    resume_text_.setFont(font);
    resume_text_.setCharacterSize(18);
    resume_text_.setFillColor({255, 255, 255});
    resume_text_.setString(ToSfString("继续战斗"));
    resume_text_.setPosition({580.0f, 298.0f});

    quit_button_.setSize({160.0f, 40.0f});
    quit_button_.setPosition({560.0f, 350.0f});
    quit_button_.setFillColor({160, 60, 60});

    quit_text_.setFont(font);
    quit_text_.setCharacterSize(18);
    quit_text_.setFillColor({255, 255, 255});
    quit_text_.setString(ToSfString("暂避锋芒"));
    quit_text_.setPosition({580.0f, 358.0f});

    // ---- 结算面板 ----
    result_panel_visible_ = false;
    result_panel_bg_.setSize({500.0f, 400.0f});
    result_panel_bg_.setPosition({390.0f, 160.0f});
    result_panel_bg_.setFillColor({10, 10, 30, 240});

    result_title_.setFont(font);
    result_title_.setCharacterSize(36);
    result_title_.setFillColor({255, 215, 100});
    result_title_.setPosition({550.0f, 180.0f});

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
    weather_text_.setString(ToSfString("天气：" + GetWeatherText_(field.GetWeatherMultiplier() > 1.2f
        ? CloudSeamanor::domain::CloudState::Mist
        : CloudSeamanor::domain::CloudState::Clear)
        + " x" + std::to_string(field.GetWeatherMultiplier())));

    // 同步战斗时间
    int seconds = static_cast<int>(field.ElapsedTime());
    battle_time_text_.setString(
        ToSfString("战斗时间：" + std::to_string(seconds / 60) + ":" +
        (seconds % 60 < 10 ? "0" : "") + std::to_string(seconds % 60))
    );

    // 同步能量条
    const auto& ps = field.GetPlayerState();
    float ratio = ps.current_energy / ps.max_energy;
    energy_bar_fill_.setSize({300.0f * ratio, 20.0f});
    energy_text_.setString(
        ToSfString(std::to_string(static_cast<int>(ps.current_energy)) + "/" +
        std::to_string(static_cast<int>(ps.max_energy)) + " 能量")
    );

    // 同步技能栏
    for (std::size_t i = 0; i < skill_buttons_.size(); ++i) {
        auto& btn = skill_buttons_[i];
        if (i < ps.unlocked_skill_ids.size()) {
            btn.skill_id = ps.unlocked_skill_ids[i];
            btn.is_locked = false;
            btn.cooldown_remaining = i < ps.cooldown_remaining.size() ? ps.cooldown_remaining[i] : 0.0f;
            btn.cooldown_total = i < ps.cooldown_total.size() ? std::max(0.1f, ps.cooldown_total[i]) : 0.1f;
            btn.name_label.setString(ToSfString(ps.unlocked_skill_ids[i]));
            btn.energy_cost_label.setString(ToSfString("READY"));
            if (btn.skill_id.find("shadow") != std::string::npos) {
                btn.icon_placeholder.setFillColor({170, 120, 230});
            } else if (btn.skill_id.find("wind") != std::string::npos) {
                btn.icon_placeholder.setFillColor({110, 230, 210});
            } else if (btn.skill_id.find("stone") != std::string::npos) {
                btn.icon_placeholder.setFillColor({220, 170, 120});
            } else {
                btn.icon_placeholder.setFillColor({120, 190, 255});
            }
        } else {
            btn.skill_id.clear();
            btn.is_locked = true;
            btn.cooldown_remaining = 0.0f;
            btn.cooldown_total = 0.1f;
            btn.name_label.setString(ToSfString("LOCK"));
            btn.energy_cost_label.setString(ToSfString("--"));
            btn.icon_placeholder.setFillColor({55, 55, 70});
        }
    }

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
        bar.name_label.setString(ToSfString(spirit.name));
        bar.name_label.setPosition({spirit.pos_x - 60.0f, spirit.pos_y - 68.0f});
        bar.recommend_label.setFont(*font_);
        bar.recommend_label.setCharacterSize(11);
        bar.recommend_label.setFillColor({210, 235, 255});
        bar.recommend_label.setPosition({spirit.pos_x - 60.0f, spirit.pos_y - 28.0f});

        bar.imbalance_segments.clear();
        if (!spirit.imbalance_segments.empty()) {
            const float seg_y = spirit.pos_y - 36.0f;
            const float seg_w = 20.0f;
            const float seg_h = 6.0f;
            const float seg_gap = 3.0f;
            const float total_w = static_cast<float>(spirit.imbalance_segments.size()) * seg_w
                + static_cast<float>(std::max<std::size_t>(1, spirit.imbalance_segments.size()) - 1) * seg_gap;
            float seg_x = spirit.pos_x - total_w * 0.5f;
            for (const auto& seg : spirit.imbalance_segments) {
                sf::RectangleShape seg_rect;
                seg_rect.setPosition({seg_x, seg_y});
                seg_rect.setSize({seg_w, seg_h});
                seg_rect.setFillColor(SegmentColor_(seg.element, seg.is_active));
                seg_rect.setOutlineThickness(1.0f);
                seg_rect.setOutlineColor(seg.is_active ? sf::Color(245, 245, 250) : sf::Color(90, 90, 110));
                bar.imbalance_segments.push_back(seg_rect);
                seg_x += seg_w + seg_gap;
            }

            std::optional<ElementType> first_active;
            for (const auto& seg : spirit.imbalance_segments) {
                if (seg.is_active) {
                    first_active = seg.element;
                    break;
                }
            }
            if (first_active.has_value()) {
                const ElementType counter = CounterElement_(*first_active);
                bar.recommend_label.setString(
                    ToSfString("建议: " + ElementText_(counter) + " 调和 " + ElementText_(*first_active)));
                bar.recommend_label.setFillColor(SegmentColor_(counter, true));
            } else {
                bar.recommend_label.setString(ToSfString("失衡已清零"));
                bar.recommend_label.setFillColor({140, 220, 140});
            }
        } else {
            bar.recommend_label.setString(ToSfString(""));
        }

        health_bars_.push_back(bar);
    }

    // 同步日志
    const auto& logs = field.GetLog();
    const int begin = std::max(0, static_cast<int>(logs.size()) - kMaxLogLines);
    for (int i = 0; i < kMaxLogLines; ++i) {
        if ((begin + i) >= static_cast<int>(logs.size())) {
            log_lines_[i].setString(ToSfString(""));
            continue;
        }
        const auto& log = logs[static_cast<std::size_t>(begin + i)];
        log_lines_[i].setString(ToSfString(log.message));
        log_lines_[i].setFillColor(log.is_important
            ? sf::Color(255, 215, 100)
            : (log.is_player_action ? sf::Color(200, 220, 255) : sf::Color(200, 200, 200)));
    }

    // 同步灵兽伙伴状态栏
    partner_bars_.clear();
    partner_names_.clear();
    partner_heart_labels_.clear();
    for (std::size_t i = 0; i < field.GetPartners().size(); ++i) {
        const auto& partner = field.GetPartners()[i];
        sf::RectangleShape bar;
        bar.setSize({180.0f, 16.0f});
        bar.setPosition({20.0f, 220.0f + static_cast<float>(i) * 34.0f});
        bar.setFillColor({40, 70, 100, 210});
        partner_bars_.push_back(bar);

        sf::Text name{*font_};
        name.setCharacterSize(13);
        name.setFillColor({210, 235, 255});
        name.setPosition({26.0f, 218.0f + static_cast<float>(i) * 34.0f});
        name.setString(ToSfString(partner.name.empty() ? partner.spirit_beast_id : partner.name));
        partner_names_.push_back(name);

        sf::Text heart{*font_};
        heart.setCharacterSize(12);
        heart.setFillColor({255, 180, 210});
        heart.setPosition({26.0f, 234.0f + static_cast<float>(i) * 34.0f});
        heart.setString(ToSfString("羁绊 Lv." + std::to_string(partner.heart_level)));
        partner_heart_labels_.push_back(heart);
    }
}

// ============================================================================
// 【AddLogEntry】添加日志
// ============================================================================
void BattleUI::AddLogEntry(const std::string& message, bool is_player_action) {
    (void)message;
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
    result_title_.setString(ToSfString(victory ? "★★ 战斗胜利！★★" : "暂避锋芒"));

    auto addLine = [&](const std::string& text, unsigned char r, unsigned char g, unsigned char b) {
        result_lines_.emplace_back(*font_);
        auto& t = result_lines_.back();
        t.setCharacterSize(18);
        t.setFillColor({r, g, b});
        t.setString(ToSfString(text));
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
        btn.is_hovered = btn.background.getGlobalBounds().contains({mouse_x, mouse_y});
    }
}

// ============================================================================
// 【OnMouseClick】鼠标点击
// ============================================================================
int BattleUI::OnMouseClick(float mouse_x, float mouse_y) {
    for (const auto& btn : skill_buttons_) {
        if (btn.background.getGlobalBounds().contains({mouse_x, mouse_y})) {
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
    const float max_width = std::max(1.0f, energy_bar_bg_.getSize().x);
    const float ratio = std::clamp(energy_bar_fill_.getSize().x / max_width, 0.0f, 1.0f);

    // 能量条颜色渐变：满=蓝，耗尽=红。
    const std::uint8_t r = static_cast<std::uint8_t>(220.0f - 160.0f * ratio);
    const std::uint8_t g = static_cast<std::uint8_t>(90.0f + 90.0f * ratio);
    const std::uint8_t b = static_cast<std::uint8_t>(80.0f + 175.0f * ratio);
    energy_bar_fill_.setFillColor({r, g, b});
    energy_text_.setFillColor(
        ratio < 0.25f ? sf::Color(255, 180, 180) : sf::Color(255, 255, 255));
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
            if (btn.is_hovered && btn.is_ready) {
                btn.background.setFillColor({90, 90, 140});
            } else {
                btn.background.setFillColor(btn.is_ready ? sf::Color(60, 60, 100) : sf::Color(30, 30, 50));
            }
        }
    }
}

void BattleUI::UpdateResultPanel_(float delta_seconds) {
    result_display_timer_ += delta_seconds;
    const float alpha_ratio = std::clamp(result_display_timer_ / 0.3f, 0.0f, 1.0f);
    const std::uint8_t panel_alpha = static_cast<std::uint8_t>(240.0f * alpha_ratio);
    result_panel_bg_.setFillColor({10, 10, 30, panel_alpha});
    result_title_.setFillColor({255, 215, 100, static_cast<std::uint8_t>(255.0f * alpha_ratio)});
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
        for (const auto& seg_rect : bar.imbalance_segments) {
            window.draw(seg_rect);
        }
        window.draw(bar.name_label);
        window.draw(bar.recommend_label);
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
        window.draw(btn.icon_placeholder);
        window.draw(btn.cooldown_overlay);
        window.draw(btn.key_label);
        window.draw(btn.cooldown_label);
        window.draw(btn.name_label);
        window.draw(btn.energy_cost_label);
    }
}

void BattleUI::DrawPartnerBars_(sf::RenderWindow& window) const {
    for (const auto& bar : partner_bars_) {
        window.draw(bar);
    }
    for (const auto& txt : partner_names_) {
        window.draw(txt);
    }
    for (const auto& txt : partner_heart_labels_) {
        window.draw(txt);
    }
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
        sf::Text line = result_lines_[i];
        line.setPosition({410.0f, 240.0f + static_cast<float>(i) * 35.0f});
        const float start = 0.1f + static_cast<float>(i) * 0.1f;
        const float reveal = std::clamp((result_display_timer_ - start) / 0.2f, 0.0f, 1.0f);
        auto c = line.getFillColor();
        line.setFillColor({c.r, c.g, c.b, static_cast<std::uint8_t>(255.0f * reveal)});
        window.draw(line);
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
