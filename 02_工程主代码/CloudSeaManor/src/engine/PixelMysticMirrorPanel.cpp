#include "CloudSeamanor/engine/PixelMysticMirrorPanel.hpp"
#include "CloudSeamanor/MysticMirrorSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"

namespace CloudSeamanor::engine {

using namespace CloudSeamanor::domain;

// ============================================================================
// MysticMirrorPanel 实现
// ============================================================================
PixelMysticMirrorPanel::PixelMysticMirrorPanel()
    : x_(0), y_(0), is_visible_(false), is_closing_(false), close_animation_(0.0f) {
}

void PixelMysticMirrorPanel::Initialize() {
    // 默认居中
    const auto& window = GameConstants::GetWindowSize();
    x_ = (window.x - kWidth) / 2.0f;
    y_ = (window.y - kHeight) / 2.0f;
}

void PixelMysticMirrorPanel::Show() {
    is_visible_ = true;
    is_closing_ = false;
    close_animation_ = 0.0f;
}

void PixelMysticMirrorPanel::Hide() {
    is_closing_ = true;
}

void PixelMysticMirrorPanel::Update(float delta_seconds) {
    if (is_closing_) {
        close_animation_ += delta_seconds * 3.0f;
        if (close_animation_ >= 1.0f) {
            is_visible_ = false;
            is_closing_ = false;
        }
    }
}

void PixelMysticMirrorPanel::Render(sf::RenderTarget& target) {
    if (!is_visible_ && !is_closing_) return;

    float alpha = is_closing_ ? (1.0f - close_animation_) : 1.0f;

    // 保存原始alpha并应用动画
    sf::RenderStates states;
    sf::FloatRect clipRect(x_, y_, kWidth, kHeight * alpha);
    target.setView(sf::View(clipRect));

    // 渲染面板
    RenderBackground(target);
    RenderTitle(target);

    // 获取观云镜数据
    auto& mirror = MysticMirrorSystem::Instance();
    auto view_data = mirror.GetViewData();

    float current_y = y_ + 70.0f;

    // 今日运势
    RenderFortuneSection(target, view_data.fortune_text);
    current_y += 90.0f;

    // 天气预报
    RenderWeatherSection(target, view_data.weather_today, view_data.weather_tomorrow, view_data.tide_day_of_month);
    current_y += 90.0f;

    // 食谱提示
    RenderRecipeSection(target, view_data.recipe_text);
    current_y += 90.0f;

    // 云海观测
    RenderAuraSection(target, view_data.cloud_density, view_data.aura_status);
    current_y += 80.0f;

    // 关闭按钮
    RenderCloseButton(target);

    // 恢复视图
    target.setView(sf::View(sf::FloatRect(0, 0, GameConstants::GetWindowSize().x, GameConstants::GetWindowSize().y)));
}

bool PixelMysticMirrorPanel::HandleInput(const sf::Event& event) {
    if (!is_visible_) return false;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::E) {
            Hide();
            return true;
        }
    }

    return false;
}

void PixelMysticMirrorPanel::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void PixelMysticMirrorPanel::RenderBackground(sf::RenderTarget& target) {
    // 主面板背景
    sf::RectangleShape bg;
    bg.setPosition({x_, y_});
    bg.setSize({kWidth, kHeight});
    bg.setFillColor(sf::Color(15, 20, 35, 230));
    target.draw(bg);

    // 像素边框
    PixelArtStyle::DrawPixelButton(
        *target.draw_list(),
        {x_, y_, kWidth, kHeight},
        0
    );

    // 内边框装饰
    sf::RectangleShape inner_border;
    inner_border.setPosition({x_ + 5, y_ + 5});
    inner_border.setSize({kWidth - 10, kHeight - 10});
    inner_border.setFillColor(sf::Color::Transparent);
    inner_border.setOutlineThickness(1);
    inner_border.setOutlineColor(sf::Color(80, 100, 140, 100));
    target.draw(inner_border);
}

void PixelMysticMirrorPanel::RenderTitle(sf::RenderTarget& target) {
    sf::Text title;
    title.setFont(GameConstants::GetFont());
    title.setString("☁️ 观 云 镜 ☁️");
    title.setCharacterSize(22);
    title.setFillColor(GetTitleColor());

    sf::FloatRect bounds = title.getLocalBounds();
    title.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    title.setPosition({x_ + kWidth / 2.0f, y_ + 30.0f});

    target.draw(title);

    // 分隔线
    sf::RectangleShape line;
    line.setPosition({x_ + 20, y_ + 55});
    line.setSize({kWidth - 40, 2});
    line.setFillColor(sf::Color(80, 100, 140, 100));
    target.draw(line);
}

void PixelMysticMirrorPanel::RenderFortuneSection(sf::RenderTarget& target, const std::string& text) {
    float section_y = y_ + 70.0f;

    // 标签
    sf::Text label;
    label.setFont(GameConstants::GetFont());
    label.setString("【今日运势】");
    label.setCharacterSize(16);
    label.setFillColor(sf::Color(180, 200, 230));
    label.setPosition({x_ + kPadding, section_y});
    target.draw(label);

    // 内容框
    sf::RectangleShape content_bg;
    content_bg.setPosition({x_ + kPadding, section_y + 25});
    content_bg.setSize({kWidth - kPadding * 2, 50});
    content_bg.setFillColor(sf::Color(30, 40, 60, 150));
    target.draw(content_bg);

    // 运势文本
    sf::Text content;
    content.setFont(GameConstants::GetFont());
    content.setString(text);
    content.setCharacterSize(15);
    content.setFillColor(sf::Color(240, 230, 200));

    sf::FloatRect content_bounds = content.getLocalBounds();
    content.setPosition({
        x_ + kPadding + 10,
        section_y + 25 + (50 - content_bounds.size.y) / 2.0f
    });
    target.draw(content);
}

void PixelMysticMirrorPanel::RenderWeatherSection(sf::RenderTarget& target,
                                                const std::string& today,
                                                const std::string& tomorrow,
                                                int tide_day) {
    float section_y = y_ + 165.0f;

    // 标签
    sf::Text label;
    label.setFont(GameConstants::GetFont());
    label.setString("【天气预报】");
    label.setCharacterSize(16);
    label.setFillColor(sf::Color(180, 200, 230));
    label.setPosition({x_ + kPadding, section_y});
    target.draw(label);

    // 天气信息框
    sf::RectangleShape content_bg;
    content_bg.setPosition({x_ + kPadding, section_y + 25});
    content_bg.setSize({kWidth - kPadding * 2, 50});
    content_bg.setFillColor(sf::Color(30, 40, 60, 150));
    target.draw(content_bg);

    // 天气文本
    sf::Text weather_text;
    weather_text.setFont(GameConstants::GetFont());
    weather_text.setString("今日：" + today + "    明日：" + tomorrow);
    weather_text.setCharacterSize(14);
    weather_text.setFillColor(sf::Color(220, 220, 200));
    weather_text.setPosition({x_ + kPadding + 10, section_y + 30});
    target.draw(weather_text);

    // 大潮日提示
    if (tide_day > 0) {
        sf::Text tide_text;
        tide_text.setFont(GameConstants::GetFont());
        tide_text.setString("大潮日：第" + std::to_string(tide_day) + "天");
        tide_text.setCharacterSize(13);
        tide_text.setFillColor(sf::Color(255, 200, 100));
        tide_text.setPosition({x_ + kPadding + 10, section_y + 50});
        target.draw(tide_text);
    }
}

void PixelMysticMirrorPanel::RenderRecipeSection(sf::RenderTarget& target, const std::string& text) {
    float section_y = y_ + 260.0f;

    // 标签
    sf::Text label;
    label.setFont(GameConstants::GetFont());
    label.setString("【今日食谱】");
    label.setCharacterSize(16);
    label.setFillColor(sf::Color(180, 200, 230));
    label.setPosition({x_ + kPadding, section_y});
    target.draw(label);

    // 内容框
    sf::RectangleShape content_bg;
    content_bg.setPosition({x_ + kPadding, section_y + 25});
    content_bg.setSize({kWidth - kPadding * 2, 50});
    content_bg.setFillColor(sf::Color(30, 40, 60, 150));
    target.draw(content_bg);

    // 食谱文本
    sf::Text content;
    content.setFont(GameConstants::GetFont());
    content.setString(text);
    content.setCharacterSize(15);
    content.setFillColor(sf::Color(220, 230, 200));

    sf::FloatRect content_bounds = content.getLocalBounds();
    content.setPosition({
        x_ + kPadding + 10,
        section_y + 25 + (50 - content_bounds.size.y) / 2.0f
    });
    target.draw(content);
}

void PixelMysticMirrorPanel::RenderAuraSection(sf::RenderTarget& target, float density, const std::string& status) {
    float section_y = y_ + 355.0f;

    // 标签
    sf::Text label;
    label.setFont(GameConstants::GetFont());
    label.setString("【云海观测】");
    label.setCharacterSize(16);
    label.setFillColor(sf::Color(180, 200, 230));
    label.setPosition({x_ + kPadding, section_y});
    target.draw(label);

    // 云海浓度条
    float bar_width = kWidth - kPadding * 2;
    float bar_height = 20.0f;
    float bar_x = x_ + kPadding;
    float bar_y = section_y + 28;

    // 背景
    sf::RectangleShape bar_bg;
    bar_bg.setPosition({bar_x, bar_y});
    bar_bg.setSize({bar_width, bar_height});
    bar_bg.setFillColor(sf::Color(20, 30, 50));
    target.draw(bar_bg);

    // 填充
    float fill_width = bar_width * std::clamp(density, 0.0f, 1.0f);
    sf::RectangleShape bar_fill;
    bar_fill.setPosition({bar_x, bar_y});

    // 根据浓度选择颜色
    sf::Color fill_color;
    if (density >= 0.7f) {
        fill_color = sf::Color(100, 200, 255);  // 充沛 - 蓝色
    } else if (density >= 0.4f) {
        fill_color = sf::Color(150, 220, 150);  // 适中 - 绿色
    } else {
        fill_color = sf::Color(200, 150, 100); // 稀薄 - 橙色
    }
    bar_fill.setSize({fill_width, bar_height});
    bar_fill.setFillColor(fill_color);
    target.draw(bar_fill);

    // 百分比和状态
    sf::Text percent_text;
    percent_text.setFont(GameConstants::GetFont());
    percent_text.setString(std::to_string(static_cast<int>(density * 100)) + "%  " + status);
    percent_text.setCharacterSize(14);
    percent_text.setFillColor(sf::Color::White);
    percent_text.setPosition({bar_x + 5, bar_y + 3});
    target.draw(percent_text);
}

void PixelMysticMirrorPanel::RenderCloseButton(sf::RenderTarget& target) {
    float btn_width = 100.0f;
    float btn_height = 35.0f;
    float btn_x = x_ + (kWidth - btn_width) / 2.0f;
    float btn_y = y_ + kHeight - 55.0f;

    // 按钮背景
    PixelArtStyle::DrawPixelButton(
        *target.draw_list(),
        {btn_x, btn_y, btn_width, btn_height},
        0
    );

    // 按钮文字
    sf::Text btn_text;
    btn_text.setFont(GameConstants::GetFont());
    btn_text.setString("[ 关闭 ]");
    btn_text.setCharacterSize(14);
    btn_text.setFillColor(sf::Color(200, 210, 230));

    sf::FloatRect bounds = btn_text.getLocalBounds();
    btn_text.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    btn_text.setPosition({btn_x + btn_width / 2.0f, btn_y + btn_height / 2.0f});
    target.draw(btn_text);

    // 提示文字
    sf::Text hint;
    hint.setFont(GameConstants::GetFont());
    hint.setString("按 ESC 或 E 键关闭");
    hint.setCharacterSize(11);
    hint.setFillColor(sf::Color(120, 130, 150));
    hint.setPosition({x_ + kWidth / 2.0f, y_ + kHeight - 12});
    sf::FloatRect hint_bounds = hint.getLocalBounds();
    hint.setOrigin({hint_bounds.size.x / 2.0f, 0});
    target.draw(hint);
}

sf::Color PixelMysticMirrorPanel::GetTitleColor() const {
    return sf::Color(255, 230, 180);
}

sf::Color PixelMysticMirrorPanel::GetSectionBgColor() const {
    return sf::Color(30, 40, 60, 150);
}

sf::Color PixelMysticMirrorPanel::GetTextColor() const {
    return sf::Color(240, 230, 200);
}

// ============================================================================
// MysticMirrorUI 实现
// ============================================================================
MysticMirrorUI& MysticMirrorUI::Instance() {
    static MysticMirrorUI instance;
    return instance;
}

void MysticMirrorUI::Initialize() {
    panel_.Initialize();
}

void MysticMirrorUI::Show() {
    panel_.Show();
    MysticMirrorSystem::Instance().RecordView();
}

void MysticMirrorUI::Hide() {
    panel_.Hide();
}

void MysticMirrorUI::Update(float delta_seconds) {
    panel_.Update(delta_seconds);
}

bool MysticMirrorUI::HandleInput(const sf::Event& event) {
    return panel_.HandleInput(event);
}

void MysticMirrorUI::Render(sf::RenderTarget& target) {
    panel_.Render(target);
}

void MysticMirrorUI::TriggerInteraction() {
    if (!panel_.IsVisible()) {
        Show();
    }
}

}  // namespace CloudSeamanor::engine
