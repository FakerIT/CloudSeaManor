#include "CloudSeamanor/engine/LoopDebugPanel.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【LoopDebugPanel】构造函数
// ============================================================================
LoopDebugPanel::LoopDebugPanel()
    : is_visible_(false)
    , needs_rebuild_(true) {

    background_.setFillColor(sf::Color(16, 12, 8, 220));
    background_.setOutlineColor(sf::Color(90, 70, 50, 180));
    background_.setOutlineThickness(2.0f);

    header_bar_.setFillColor(sf::Color(45, 35, 25, 255));
    header_bar_.setOutlineThickness(0.0f);

    separator_.setFillColor(sf::Color(70, 55, 40, 200));
    separator_.setOutlineThickness(0.0f);
}

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void LoopDebugPanel::Initialize(const sf::Font& font) {
    font_ = &font;
    needs_rebuild_ = true;

    title_text_ = std::make_unique<sf::Text>(font);
    title_text_->setString("Loop Performance");
    title_text_->setCharacterSize(14);
    title_text_->setFillColor(sf::Color(220, 200, 160, 255));

    total_frame_text_ = std::make_unique<sf::Text>(font);
    total_frame_text_->setCharacterSize(12);
    total_frame_text_->setFillColor(sf::Color(180, 220, 180, 255));

    busy_frame_text_ = std::make_unique<sf::Text>(font);
    busy_frame_text_->setCharacterSize(12);
    busy_frame_text_->setFillColor(sf::Color(220, 180, 100, 255));

    RebuildTexts_(font);
}

// ============================================================================
// 【Update】更新
// ============================================================================
void LoopDebugPanel::Update(const GameLoopCoordinator& coordinator) {
    if (!is_visible_ || needs_rebuild_) return;

    const float total_ms = coordinator.GetTotalFrameTimeMs();
    const bool is_busy = coordinator.IsFrameBusy();

    // 更新总帧时间
    if (total_frame_text_) {
        std::ostringstream oss;
        oss << "Total: " << total_ms << " ms";
        total_frame_text_->setString(oss.str());
        total_frame_text_->setFillColor(is_busy
            ? sf::Color(255, 200, 100, 255)
            : sf::Color(180, 220, 180, 255));
    }

    // 更新繁忙帧指示
    if (busy_frame_text_) {
        if (is_busy) {
            busy_frame_text_->setString("[BUSY] Frame exceeded 16.67ms");
            busy_frame_text_->setFillColor(sf::Color(255, 150, 100, 255));
        } else {
            busy_frame_text_->setString("[OK] Frame within budget");
            busy_frame_text_->setFillColor(sf::Color(100, 200, 100, 255));
        }
    }

    // 更新阶段统计
    float phase_y = 0;
    for (int i = 1; i <= static_cast<int>(LoopPhase::Count); ++i) {
        if (i >= 1 && i - 1 < static_cast<int>(phase_texts_.size())) {
            const auto phase = static_cast<LoopPhase>(i - 1);
            const auto& stats = coordinator.GetPhaseStats(phase);

            std::ostringstream oss;
            oss << GetPhaseName(phase) << ": "
                << stats.AverageTimeMs() << "/" << stats.max_time_ms << " ms";

            phase_texts_[i]->setString(oss.str());

            // 高亮耗时较长的阶段
            if (stats.max_time_ms > 5.0f) {
                phase_texts_[i]->setFillColor(sf::Color(255, 200, 100, 255));
            } else if (stats.max_time_ms > 2.0f) {
                phase_texts_[i]->setFillColor(sf::Color(220, 220, 180, 255));
            } else {
                phase_texts_[i]->setFillColor(sf::Color(200, 200, 180, 255));
            }
        }
    }

    // 更新系统统计
    const auto& sys_stats = coordinator.GetSystemStats();
    int sys_idx = 1;
    for (size_t i = 0; i < sys_stats.size() && sys_idx < static_cast<int>(system_texts_.size()); ++i) {
        const auto& stats = sys_stats[i];
        if (stats.call_count > 0) {
            std::ostringstream oss;
            oss << stats.system_name << ": "
                << (stats.total_time_ms / stats.call_count) << " ms";

            system_texts_[sys_idx]->setString(oss.str());

            // 高亮耗时较长的系统
            const float avg = stats.total_time_ms / stats.call_count;
            if (avg > 3.0f) {
                system_texts_[sys_idx]->setFillColor(sf::Color(255, 180, 100, 255));
            } else if (avg > 1.0f) {
                system_texts_[sys_idx]->setFillColor(sf::Color(200, 220, 180, 255));
            } else {
                system_texts_[sys_idx]->setFillColor(sf::Color(170, 170, 160, 255));
            }
            ++sys_idx;
        }
    }

    // 清空未使用的系统统计文本
    for (size_t i = sys_idx; i < system_texts_.size(); ++i) {
        system_texts_[i]->setString("");
    }
}

// ============================================================================
// 【Render】渲染
// ============================================================================
void LoopDebugPanel::Render(sf::RenderWindow& window) {
    if (!is_visible_ || needs_rebuild_) return;

    // 面板位置（左上角）
    const float x = kPadding;
    const float y = kPadding;

    // 更新背景
    background_.setSize({kPanelWidth, kPanelHeight});
    background_.setPosition({x, y});

    header_bar_.setSize({kPanelWidth, kHeaderHeight});
    header_bar_.setPosition({x, y});

    separator_.setSize({kPanelWidth - 4.0f, 1.0f});
    separator_.setPosition({x + 2.0f, y + kHeaderHeight});

    // 绘制背景
    window.draw(background_);
    window.draw(header_bar_);

    // 绘制标题
    if (title_text_) {
        title_text_->setPosition({x + kPadding, y + 5.0f});
        window.draw(*title_text_);
    }

    // 绘制阶段统计
    float line_y = y + kHeaderHeight + kPadding + 2.0f;
    for (size_t i = 0; i < phase_texts_.size(); ++i) {
        if (phase_texts_[i]) {
            phase_texts_[i]->setPosition({x + kPadding, line_y});
            window.draw(*phase_texts_[i]);
            line_y += kLineHeight;
        }
    }

    // 绘制分隔线
    window.draw(separator_);

    // 绘制系统统计
    line_y += 4.0f;
    for (const auto& text : system_texts_) {
        if (text && !text->getString().isEmpty()) {
            text->setPosition({x + kPadding, line_y});
            window.draw(*text);
            line_y += kLineHeight;
        }
    }

    // 绘制底部统计
    if (total_frame_text_) {
        total_frame_text_->setPosition({x + kPadding, y + kPanelHeight - 28.0f});
        window.draw(*total_frame_text_);
    }
    if (busy_frame_text_) {
        busy_frame_text_->setPosition({x + kPadding, y + kPanelHeight - 14.0f});
        window.draw(*busy_frame_text_);
    }
}

// ============================================================================
// 【RebuildTexts_】重建文本
// ============================================================================
void LoopDebugPanel::RebuildTexts_(const sf::Font& font) {
    if (!font_) return;

    // 清空现有文本
    phase_texts_.clear();
    system_texts_.clear();

    // 创建阶段标题
    auto phase_header = std::make_unique<sf::Text>(font);
    phase_header->setString("--- Phase Stats ---");
    phase_header->setCharacterSize(11);
    phase_header->setFillColor(sf::Color(150, 130, 100, 255));
    phase_texts_.push_back(std::move(phase_header));

    // 为每个阶段创建统计文本
    for (int i = 0; i < static_cast<int>(LoopPhase::Count); ++i) {
        auto text = std::make_unique<sf::Text>(font);
        text->setCharacterSize(11);
        text->setFillColor(sf::Color(200, 200, 180, 255));
        phase_texts_.push_back(std::move(text));
    }

    // 创建系统标题
    auto sys_header = std::make_unique<sf::Text>(font);
    sys_header->setString("--- System Stats ---");
    sys_header->setCharacterSize(11);
    sys_header->setFillColor(sf::Color(150, 130, 100, 255));
    system_texts_.push_back(std::move(sys_header));

    // 预留系统统计文本（最多显示10个）
    for (int i = 0; i < 10; ++i) {
        auto text = std::make_unique<sf::Text>(font);
        text->setCharacterSize(10);
        text->setFillColor(sf::Color(170, 170, 160, 255));
        system_texts_.push_back(std::move(text));
    }

    needs_rebuild_ = false;
}

} // namespace CloudSeamanor::engine
