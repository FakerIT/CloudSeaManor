#include "CloudSeamanor/engine/MainMenuController.hpp"

#include "CloudSeamanor/app/CloudSeaManor.hpp"
#include "CloudSeamanor/infrastructure/ResourceManager.hpp"
#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <filesystem>

namespace CloudSeamanor::engine {

namespace {

std::string BuildMainMenuRecentSavePreview_() {
    const CloudSeamanor::infrastructure::SaveSlotManager slot_manager;
    const auto slots = slot_manager.ReadAllMetadata();
    const auto it = std::max_element(
        slots.begin(),
        slots.end(),
        [](const auto& lhs, const auto& rhs) {
            if (lhs.exists != rhs.exists) {
                return !lhs.exists && rhs.exists;
            }
            if (lhs.day != rhs.day) {
                return lhs.day < rhs.day;
            }
            return lhs.slot_index > rhs.slot_index;
        });
    if (it == slots.end() || !it->exists) {
        return "最近存档：无";
    }
    const std::string season = it->season_text.empty() ? "未知季节" : it->season_text;
    const std::string when = it->saved_at_text.empty() ? "未知时间" : it->saved_at_text;
    return "最近存档：槽位" + std::to_string(it->slot_index)
        + " 第" + std::to_string(it->day) + "天 " + season + " " + when;
}

}  // namespace

// ============================================================================
// 【MainMenuController】构造函数
// ============================================================================
MainMenuController::MainMenuController() {
    // 初始化面板和角落块
    panel_.setFillColor(sf::Color(32, 26, 20, 240));
    panel_.setOutlineThickness(2.0f);
    panel_.setOutlineColor(sf::Color(110, 88, 60));

    for (auto& corner : corner_blocks_) {
        corner.setFillColor(sf::Color(120, 82, 46, 255));
    }
}

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void MainMenuController::Initialize(const sf::Font* font, sf::Vector2u window_size, MainMenuCallbacks callbacks) {
    callbacks_ = std::move(callbacks);
    visible_ = true;
    selected_index_ = 0;
    pending_action_ = MainMenuAction::None;
    transition_out_ = false;
    fade_alpha_ = 0.0f;
    anim_time_ = 0.0f;

    InitializeFont_(font);

    // 检查存档
    const CloudSeamanor::infrastructure::SaveSlotManager slot_manager;
    const auto slots = slot_manager.ReadAllMetadata();
    has_save_ = std::any_of(slots.begin(), slots.end(), [](const auto& slot) {
        return slot.exists;
    });

    if (!has_save_ && items_[1]) {
        items_[1]->setString(MakeUtf8String(u8"继续游戏（无存档）"));
    }

    InitializeLayout_();
    UpdateLayout(window_size);
}

// ============================================================================
// 【InitializeFont_】初始化字体
// ============================================================================
void MainMenuController::InitializeFont_(const sf::Font* font) {
    if (font == nullptr) {
        title_.reset();
        for (auto& item : items_) {
            item.reset();
        }
        save_preview_text_.reset();
        status_text_.reset();
        visible_ = false;
        return;
    }

    // 标题
    title_ = std::make_unique<sf::Text>(*font);
    {
        const auto* begin = reinterpret_cast<const char*>(u8"云海山庄");
        const auto* end = begin + (sizeof(u8"云海山庄") - 1);
        title_->setString(sf::String::fromUtf8(begin, end));
    }

    // 菜单项
    const auto MakeUtf8 = [](const char8_t* literal) -> sf::String {
        const auto* begin = reinterpret_cast<const char*>(literal);
        const auto* end = begin;
        while (*end != '\0') {
            ++end;
        }
        return sf::String::fromUtf8(begin, end);
    };
    const std::array<sf::String, kMainMenuItemCount> menu_labels{
        MakeUtf8(u8"开始游戏"),
        MakeUtf8(u8"继续游戏"),
        MakeUtf8(u8"设置"),
        MakeUtf8(u8"关于"),
        MakeUtf8(u8"退出游戏"),
    };

    for (int i = 0; i < kMainMenuItemCount; ++i) {
        items_[i] = std::make_unique<sf::Text>(*font);
        items_[i]->setString(menu_labels[static_cast<std::size_t>(i)]);
    }

    // 存档预览文本
    save_preview_text_ = std::make_unique<sf::Text>(*font);
    if (has_save_) {
        const std::string preview = BuildMainMenuRecentSavePreview_();
        save_preview_text_->setString(sf::String::fromUtf8(preview.begin(), preview.end()));
    } else {
        const auto* begin = reinterpret_cast<const char*>(u8"最近存档：无");
        const auto* end = begin + (sizeof(u8"最近存档：无") - 1);
        save_preview_text_->setString(sf::String::fromUtf8(begin, end));
    }

    // 状态文本
    status_text_ = std::make_unique<sf::Text>(*font);
    {
        const auto* begin = reinterpret_cast<const char*>(u8"方向键/手柄十字键选择，回车/A确认");
        const auto* end = begin + (sizeof(u8"方向键/手柄十字键选择，回车/A确认") - 1);
        status_text_->setString(sf::String::fromUtf8(begin, end));
    }
}

// ============================================================================
// 【InitializeLayout_】初始化布局
// ============================================================================
void MainMenuController::InitializeLayout_() {
    // 背景加载（如果资源管理器可用）
    // 注意：完整实现需要访问 ResourceManager
}

// ============================================================================
// 【UpdateLayout】更新布局
// ============================================================================
void MainMenuController::UpdateLayout(sf::Vector2u window_size) {
    (void)window_size;
    const float w = 1280.0f;  // ScreenConfig::Width
    const float h = 720.0f;   // ScreenConfig::Height
    const float scale = 1.0f;

    // 面板
    const sf::Vector2f panel_size{560.0f * scale, 410.0f * scale};
    panel_.setSize(panel_size);
    panel_.setPosition({(w - panel_size.x) * 0.5f, (h - panel_size.y) * 0.5f});
    panel_.setOutlineThickness(std::max(1.0f, 2.0f * scale));

    // 标题
    if (title_) {
        title_->setCharacterSize(static_cast<unsigned>(std::round(18.0f * scale)));
        const auto title_bounds = title_->getLocalBounds();
        title_->setPosition({
            panel_.getPosition().x + (panel_size.x - title_bounds.size.x) * 0.5f - title_bounds.position.x,
            panel_.getPosition().y + 22.0f * scale
        });
    }

    // 角落块
    for (int i = 0; i < 4; ++i) {
        auto& corner = corner_blocks_[i];
        corner.setFillColor(sf::Color(120, 82, 46, 255));
        corner.setSize({14.0f * scale, 14.0f * scale});
    }
    corner_blocks_[0].setPosition(panel_.getPosition());
    corner_blocks_[1].setPosition({panel_.getPosition().x + panel_size.x - corner_blocks_[1].getSize().x, panel_.getPosition().y});
    corner_blocks_[2].setPosition({panel_.getPosition().x, panel_.getPosition().y + panel_size.y - corner_blocks_[2].getSize().y});
    corner_blocks_[3].setPosition({panel_.getPosition().x + panel_size.x - corner_blocks_[3].getSize().x, panel_.getPosition().y + panel_size.y - corner_blocks_[3].getSize().y});

    // 按钮区域
    for (int i = 0; i < kMainMenuItemCount; ++i) {
        button_rects_[i] = sf::FloatRect{
            {panel_.getPosition().x + 70.0f * scale,
             panel_.getPosition().y + (96.0f + static_cast<float>(i) * 56.0f) * scale},
            {panel_size.x - 140.0f * scale, 46.0f * scale}
        };
        if (!items_[i]) continue;
        items_[i]->setCharacterSize(static_cast<unsigned>(std::round(14.0f * scale)));
        const auto b = items_[i]->getLocalBounds();
        items_[i]->setPosition({
            button_rects_[i].position.x + (button_rects_[i].size.x - b.size.x) * 0.5f - b.position.x,
            button_rects_[i].position.y + (button_rects_[i].size.y - b.size.y) * 0.5f - b.position.y - 2.0f * scale
        });
    }

    // 存档预览
    if (save_preview_text_) {
        save_preview_text_->setCharacterSize(static_cast<unsigned>(std::round(11.0f * scale)));
        const auto& rect = button_rects_[1];
        save_preview_text_->setPosition({rect.position.x + rect.size.x + 12.0f * scale, rect.position.y + 10.0f * scale});
    }

    // 状态文本
    if (status_text_) {
        status_text_->setCharacterSize(static_cast<unsigned>(std::round(10.0f * scale)));
        const auto b = status_text_->getLocalBounds();
        status_text_->setPosition({
            panel_.getPosition().x + (panel_size.x - b.size.x) * 0.5f - b.position.x,
            panel_.getPosition().y + panel_size.y - 24.0f * scale
        });
    }
}

// ============================================================================
// 【HandleInput】处理输入
// ============================================================================
bool MainMenuController::HandleInput(const sf::Event::KeyPressed& key_event) {
    if (!visible_) return false;

    switch (key_event.code) {
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::W:
        selected_index_ = (selected_index_ - 1 + kMainMenuItemCount) % kMainMenuItemCount;
        return true;

    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S:
        selected_index_ = (selected_index_ + 1) % kMainMenuItemCount;
        return true;

    case sf::Keyboard::Key::Enter:
    case sf::Keyboard::Key::Space: {
        static const std::array<MainMenuAction, kMainMenuItemCount> actions{
            MainMenuAction::StartGame,
            MainMenuAction::ContinueGame,
            MainMenuAction::Settings,
            MainMenuAction::About,
            MainMenuAction::ExitGame
        };
        pending_action_ = actions[static_cast<std::size_t>(selected_index_)];
        transition_out_ = true;
        return true;
    }

    default:
        return false;
    }
}

// ============================================================================
// 【ExecuteAction】执行动作
// ============================================================================
void MainMenuController::ExecuteAction(MainMenuAction action) {
    auto SetStatusUtf8 = [this](const char8_t* literal) {
        if (!status_text_) return;
        const auto* begin = reinterpret_cast<const char*>(literal);
        const auto* end = begin;
        while (*end != '\0') {
            ++end;
        }
        status_text_->setString(sf::String::fromUtf8(begin, end));
    };

    switch (action) {
    case MainMenuAction::StartGame:
        visible_ = false;
        if (callbacks_.push_hint) {
            callbacks_.push_hint("正在开始新游戏...", 1.5f);
        }
        break;

    case MainMenuAction::ContinueGame:
        if (!has_save_) {
            if (callbacks_.push_hint) {
                callbacks_.push_hint("当前没有可用存档。", 2.0f);
            }
            SetStatusUtf8(u8"当前无存档：请先开始新游戏。");
            break;
        }
        if (callbacks_.load_game) {
            callbacks_.load_game();
        }
        visible_ = false;
        break;

    case MainMenuAction::Settings:
        if (callbacks_.push_hint) {
            callbacks_.push_hint("设置面板可在游戏内按 Esc 打开。", 2.4f);
        }
        SetStatusUtf8(u8"设置：进入游戏后按 Esc 打开设置面板。");
        transition_out_ = false;
        pending_action_ = MainMenuAction::None;
        fade_alpha_ = 1.0f;
        break;

    case MainMenuAction::About:
        if (callbacks_.push_hint) {
            callbacks_.push_hint("云海山庄 v0.1.0 - 东方治愈像素农庄", 2.8f);
        }
        SetStatusUtf8(u8"云海山庄 v0.1.0 - 东方治愈像素农庄");
        transition_out_ = false;
        pending_action_ = MainMenuAction::None;
        fade_alpha_ = 1.0f;
        break;

    case MainMenuAction::ExitGame:
        if (callbacks_.exit_game) {
            callbacks_.exit_game();
        }
        break;

    default:
        break;
    }
}

// ============================================================================
// 【Render】渲染
// ============================================================================
void MainMenuController::Render(sf::RenderWindow& window) {
    if (!visible_) return;

    // 背景精灵
    if (background_sprite_) {
        window.draw(*background_sprite_);
    }

    // 面板
    window.draw(panel_);

    // 角落块
    for (const auto& corner : corner_blocks_) {
        window.draw(corner);
    }

    // 标题
    if (title_) {
        window.draw(*title_);
    }

    // 菜单项
    for (int i = 0; i < kMainMenuItemCount; ++i) {
        if (!items_[i]) continue;

        // 高亮当前选中项
        if (i == selected_index_) {
            const auto& rect = button_rects_[i];
            sf::RectangleShape highlight(rect.size);
            highlight.setPosition(rect.position);
            highlight.setFillColor(sf::Color(80, 60, 40, 180));
            highlight.setOutlineThickness(0.0f);
            window.draw(highlight);
        }

        window.draw(*items_[i]);
    }

    // 存档预览
    if (save_preview_text_) {
        window.draw(*save_preview_text_);
    }

    // 状态文本
    if (status_text_) {
        window.draw(*status_text_);
    }
}

// ============================================================================
// 【MakeUtf8String】创建 UTF-8 字符串
// ============================================================================
sf::String MainMenuController::MakeUtf8String(const char8_t* literal) {
    const auto* begin = reinterpret_cast<const char*>(literal);
    const auto* end = begin;
    while (*end != '\0') {
        ++end;
    }
    return sf::String::fromUtf8(begin, end);
}

}  // namespace CloudSeamanor::engine
