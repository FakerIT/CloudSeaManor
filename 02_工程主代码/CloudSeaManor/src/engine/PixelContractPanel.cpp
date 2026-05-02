#include "CloudSeamanor/engine/PixelContractPanel.hpp"
#include "CloudSeamanor/ContractSystem.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include <algorithm>

namespace CloudSeamanor::engine {

using namespace CloudSeamanor::domain;

// ============================================================================
// PixelContractPanel 实现
// ============================================================================
PixelContractPanel::PixelContractPanel()
    : x_(0), y_(0), is_visible_(false),
      scroll_offset_(0.0f), max_scroll_(0.0f),
      selected_contract_(0), show_bundle_detail_(false),
      is_closing_(false), close_animation_(0.0f) {
}

void PixelContractPanel::Initialize() {
    const auto& window = GameConstants::GetWindowSize();
    x_ = (window.x - kWidth) / 2.0f;
    y_ = (window.y - kHeight) / 2.0f;
    LoadContractData();
}

void PixelContractPanel::Show() {
    is_visible_ = true;
    is_closing_ = false;
    close_animation_ = 0.0f;
    LoadContractData(); // 刷新数据
}

void PixelContractPanel::Hide() {
    is_closing_ = true;
    show_bundle_detail_ = false;
}

void PixelContractPanel::Update(float delta_seconds) {
    if (is_closing_) {
        close_animation_ += delta_seconds * 3.0f;
        if (close_animation_ >= 1.0f) {
            is_visible_ = false;
            is_closing_ = false;
        }
    }

    // 计算最大滚动
    float content_height = contracts_.size() * kContractHeight;
    max_scroll_ = std::max(0.0f, content_height - (kHeight - 100.0f));
}

void PixelContractPanel::Render(sf::RenderTarget& target) {
    if (!is_visible_ && !is_closing_) return;

    float alpha = is_closing_ ? (1.0f - close_animation_) : 1.0f;

    // 裁剪区域
    sf::FloatRect clipRect(x_, y_, kWidth, kHeight * alpha);
    sf::View oldView = target.getView();
    target.setView(sf::View(clipRect));

    RenderBackground(target);
    RenderTitle(target);
    RenderContractList(target);

    if (show_bundle_detail_ && selected_contract_ < contracts_.size()) {
        RenderBundleDetail(target, contracts_[selected_contract_]);
    }

    RenderCloseButton(target);
    RenderScrollBar(target);

    target.setView(oldView);
}

bool PixelContractPanel::HandleInput(const sf::Event& event) {
    if (!is_visible_) return false;

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Escape:
            case sf::Keyboard::E:
                Hide();
                return true;

            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                SelectContract(selected_contract_ - 1);
                return true;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                SelectContract(selected_contract_ + 1);
                return true;

            case sf::Keyboard::Enter:
            case sf::Keyboard::Space:
                if (show_bundle_detail_) {
                    show_bundle_detail_ = false;
                } else {
                    show_bundle_detail_ = true;
                }
                return true;

            case sf::Keyboard::Return:
                if (selected_contract_ < contracts_.size()) {
                    const auto& contract = contracts_[selected_contract_];
                    if (contract.can_claim_reward) {
                        ClaimReward(contract.id);
                    }
                }
                return true;
        }
    }

    return false;
}

void PixelContractPanel::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void PixelContractPanel::LoadContractData() {
    contracts_.clear();

    auto& system = ContractSystem::Instance();
    auto all_contracts = system.GetAllContracts();

    for (const auto* contract : all_contracts) {
        ContractItemData item;
        item.id = contract->id;
        item.name = contract->name_zh;
        item.description = contract->description_zh;

        if (!contract->spirit.visual_color.empty()) {
            item.spirit_color = contract->spirit.visual_color;
        } else {
            item.spirit_color = "88CCFF";
        }

        // 加载Bundle数据
        auto bundle_progress = system.GetBundleProgress(contract->id);
        int total_required = 0;
        int total_collected = 0;

        for (const auto* bp : bundle_progress) {
            ContractBundleData bundle;
            bundle.required = bp->required;
            bundle.collected = bp->collected;
            bundle.is_complete = bp->IsComplete();

            // 查找bundle名称
            for (const auto& b : contract->bundles) {
                if (b.id == bp->bundle_id) {
                    bundle.item_name = b.item_name_zh;
                    bundle.description = b.description_zh;
                    break;
                }
            }

            item.bundles.push_back(bundle);
            total_required++;
            if (bp->IsComplete()) total_collected++;
        }

        item.progress = total_required > 0 ? static_cast<float>(total_collected) / total_required : 0.0f;
        item.is_unlocked = system.IsContractUnlocked(contract->id);
        item.is_completed = system.IsContractCompleted(contract->id);
        item.is_rewarded = system.IsContractRewarded(contract->id);
        item.can_claim_reward = item.is_completed && !item.is_rewarded;

        contracts_.push_back(item);
    }
}

void PixelContractPanel::SelectContract(int index) {
    index = std::clamp(index, 0, static_cast<int>(contracts_.size()) - 1);
    selected_contract_ = index;
    show_bundle_detail_ = false;
}

void PixelContractPanel::ClaimReward(const std::string& contract_id) {
    ContractSystem::Instance().TryClaimReward(contract_id);
    LoadContractData(); // 刷新
}

// ============================================================================
// 渲染方法
// ============================================================================
void PixelContractPanel::RenderBackground(sf::RenderTarget& target) {
    sf::RectangleShape bg;
    bg.setPosition({x_, y_});
    bg.setSize({kWidth, kHeight});
    bg.setFillColor(sf::Color(15, 20, 35, 230));
    target.draw(bg);

    PixelArtStyle::DrawPixelButton(
        *target.draw_list(),
        {x_, y_, kWidth, kHeight},
        0
    );
}

void PixelContractPanel::RenderTitle(sf::RenderTarget& target) {
    sf::Text title;
    title.setFont(GameConstants::GetFont());
    title.setString("📜 茶 灵 契 约 📜");
    title.setCharacterSize(22);
    title.setFillColor(sf::Color(255, 230, 180));

    sf::FloatRect bounds = title.getLocalBounds();
    title.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    title.setPosition({x_ + kWidth / 2.0f, y_ + 30.0f});
    target.draw(title);

    // 进度显示
    auto& system = ContractSystem::Instance();
    sf::Text progress_text;
    progress_text.setFont(GameConstants::GetFont());
    progress_text.setString("完成进度: " +
        std::to_string(system.GetCompletedCount()) + "/" +
        std::to_string(system.GetTotalContractCount()));
    progress_text.setCharacterSize(14);
    progress_text.setFillColor(sf::Color(180, 200, 220));
    progress_text.setPosition({x_ + kWidth / 2.0f, y_ + 55.0f});
    bounds = progress_text.getLocalBounds();
    progress_text.setOrigin({bounds.size.x / 2.0f, 0});
    target.draw(progress_text);

    // 分隔线
    sf::RectangleShape line;
    line.setPosition({x_ + 20, y_ + 78});
    line.setSize({kWidth - 40, 2});
    line.setFillColor(sf::Color(80, 100, 140, 100));
    target.draw(line);
}

void PixelContractPanel::RenderContractList(sf::RenderTarget& target) {
    float list_y = y_ + 85.0f;
    float item_y = list_y - scroll_offset_;

    for (size_t i = 0; i < contracts_.size(); ++i) {
        if (item_y + kContractHeight < list_y || item_y > y_ + kHeight - 60) {
            item_y += kContractHeight;
            continue;
        }

        RenderContractItem(target, contracts_[i], item_y);
        item_y += kContractHeight;
    }
}

void PixelContractPanel::RenderContractItem(sf::RenderTarget& target, const ContractItemData& contract, float item_y) {
    bool is_selected = (contracts_[selected_contract_].id == contract.id);

    // 背景
    sf::RectangleShape item_bg;
    item_bg.setPosition({x_ + 10, item_y});
    item_bg.setSize({kWidth - 20, kContractHeight - 5});

    if (!contract.is_unlocked) {
        item_bg.setFillColor(sf::Color(30, 30, 40, 150));
    } else if (contract.is_completed && contract.is_rewarded) {
        item_bg.setFillColor(sf::Color(40, 60, 50, 150));
    } else if (contract.is_completed) {
        item_bg.setFillColor(sf::Color(60, 50, 40, 150));
    } else if (is_selected) {
        item_bg.setFillColor(sf::Color(50, 60, 80, 180));
    } else {
        item_bg.setFillColor(sf::Color(35, 45, 60, 150));
    }
    target.draw(item_bg);

    // 选中边框
    if (is_selected) {
        sf::RectangleShape border;
        border.setPosition({x_ + 8, item_y - 2});
        border.setSize({kWidth - 16, kContractHeight - 1});
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineThickness(2);
        border.setOutlineColor(sf::Color(100, 150, 200));
        target.draw(border);
    }

    // 契约名称
    sf::Text name_text;
    name_text.setFont(GameConstants::GetFont());
    name_text.setString(contract.name);
    name_text.setCharacterSize(16);
    name_text.setFillColor(contract.is_unlocked ? sf::Color(240, 230, 200) : sf::Color(100, 100, 120));
    name_text.setPosition({x_ + 25, item_y + 10});
    target.draw(name_text);

    // 状态标签
    sf::Text status_text;
    status_text.setFont(GameConstants::GetFont());
    if (!contract.is_unlocked) {
        status_text.setString("[未解锁]");
        status_text.setFillColor(sf::Color(120, 120, 140));
    } else if (contract.is_rewarded) {
        status_text.setString("[已完成]");
        status_text.setFillColor(sf::Color(100, 200, 150));
    } else if (contract.is_completed) {
        status_text.setString("[可领取]");
        status_text.setFillColor(sf::Color(255, 200, 100));
    } else {
        status_text.setString("[收集中]");
        status_text.setFillColor(sf::Color(150, 180, 220));
    }
    status_text.setCharacterSize(12);
    status_text.setPosition({x_ + 25, item_y + 32});
    target.draw(status_text);

    // 进度条
    RenderProgressBar(target, x_ + 25, item_y + 50, kWidth - 120, contract.progress);

    // 按Enter提示
    if (is_selected && contract.is_completed && !contract.is_rewarded) {
        sf::Text hint;
        hint.setFont(GameConstants::GetFont());
        hint.setString("按 Enter 领取奖励");
        hint.setCharacterSize(11);
        hint.setFillColor(sf::Color(255, 220, 100));
        hint.setPosition({x_ + kWidth - 120, item_y + 45});
        target.draw(hint);
    }
}

void PixelContractPanel::RenderBundleDetail(sf::RenderTarget& target, const ContractItemData& contract) {
    float detail_y = y_ + 85.0f;
    float detail_height = kHeight - 100.0f;

    // 半透明背景
    sf::RectangleShape overlay;
    overlay.setPosition({x_ + 10, detail_y});
    overlay.setSize({kWidth - 20, detail_height});
    overlay.setFillColor(sf::Color(20, 25, 40, 220));
    target.draw(overlay);

    // 标题
    sf::Text title;
    title.setFont(GameConstants::GetFont());
    title.setString(contract.name + " - 所需材料");
    title.setCharacterSize(16);
    title.setFillColor(sf::Color(240, 230, 200));
    title.setPosition({x_ + 25, detail_y + 10});
    target.draw(title);

    // Bundle列表
    float bundle_y = detail_y + 40.0f;
    for (const auto& bundle : contract.bundles) {
        // 物品名称
        sf::Text item_text;
        item_text.setFont(GameConstants::GetFont());
        item_text.setString(bundle.item_name);
        item_text.setCharacterSize(14);
        item_text.setFillColor(bundle.is_complete ? sf::Color(100, 200, 150) : sf::Color(200, 200, 200));
        item_text.setPosition({x_ + 25, bundle_y});
        target.draw(item_text);

        // 进度
        sf::Text progress_text;
        progress_text.setFont(GameConstants::GetFont());
        progress_text.setString(std::to_string(bundle.collected) + "/" + std::to_string(bundle.required));
        progress_text.setCharacterSize(14);
        progress_text.setFillColor(bundle.is_complete ? sf::Color(100, 255, 150) : sf::Color(150, 150, 150));
        progress_text.setPosition({x_ + kWidth - 80, bundle_y});
        target.draw(progress_text);

        // 描述
        sf::Text desc_text;
        desc_text.setFont(GameConstants::GetFont());
        desc_text.setString(bundle.description);
        desc_text.setCharacterSize(11);
        desc_text.setFillColor(sf::Color(120, 120, 140));
        desc_text.setPosition({x_ + 25, bundle_y + 18});
        target.draw(desc_text);

        bundle_y += kBundleItemHeight;
    }

    // 操作提示
    sf::Text hint;
    hint.setFont(GameConstants::GetFont());
    hint.setString("按 Enter 或 Space 返回列表");
    hint.setCharacterSize(12);
    hint.setFillColor(sf::Color(150, 150, 170));
    hint.setPosition({x_ + kWidth / 2.0f, y_ + kHeight - 30});
    sf::FloatRect bounds = hint.getLocalBounds();
    hint.setOrigin({bounds.size.x / 2.0f, 0});
    target.draw(hint);
}

void PixelContractPanel::RenderProgressBar(sf::RenderTarget& target, float x, float y, float width, float progress) {
    // 背景
    sf::RectangleShape bar_bg;
    bar_bg.setPosition({x, y});
    bar_bg.setSize({width, 12});
    bar_bg.setFillColor(sf::Color(20, 30, 50));
    target.draw(bar_bg);

    // 填充
    float fill_width = width * std::clamp(progress, 0.0f, 1.0f);
    sf::RectangleShape bar_fill;
    bar_fill.setPosition({x, y});
    bar_fill.setSize({fill_width, 12});

    sf::Color fill_color;
    if (progress >= 1.0f) {
        fill_color = sf::Color(100, 200, 150); // 完成 - 绿色
    } else if (progress >= 0.5f) {
        fill_color = sf::Color(150, 180, 220); // 进行中 - 蓝色
    } else {
        fill_color = sf::Color(100, 140, 180); // 未完成 - 灰蓝
    }
    bar_fill.setFillColor(fill_color);
    target.draw(bar_fill);
}

void PixelContractPanel::RenderCloseButton(sf::RenderTarget& target) {
    float btn_width = 80.0f;
    float btn_height = 30.0f;
    float btn_x = x_ + (kWidth - btn_width) / 2.0f;
    float btn_y = y_ + kHeight - 45.0f;

    PixelArtStyle::DrawPixelButton(
        *target.draw_list(),
        {btn_x, btn_y, btn_width, btn_height},
        0
    );

    sf::Text btn_text;
    btn_text.setFont(GameConstants::GetFont());
    btn_text.setString("关闭");
    btn_text.setCharacterSize(14);
    btn_text.setFillColor(sf::Color(200, 210, 230));
    sf::FloatRect bounds = btn_text.getLocalBounds();
    btn_text.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    btn_text.setPosition({btn_x + btn_width / 2.0f, btn_y + btn_height / 2.0f});
    target.draw(btn_text);
}

void PixelContractPanel::RenderScrollBar(sf::RenderTarget& target) {
    if (max_scroll_ <= 0) return;

    float bar_x = x_ + kWidth - 12;
    float bar_y = y_ + 85;
    float bar_height = kHeight - 100;
    float thumb_height = bar_height * 0.3f;
    float thumb_y = bar_y + (scroll_offset_ / max_scroll_) * (bar_height - thumb_height);

    // 轨道
    sf::RectangleShape track;
    track.setPosition({bar_x, bar_y});
    track.setSize({8, bar_height});
    track.setFillColor(sf::Color(30, 40, 60));
    target.draw(track);

    // 滑块
    sf::RectangleShape thumb;
    thumb.setPosition({bar_x, thumb_y});
    thumb.setSize({8, thumb_height});
    thumb.setFillColor(sf::Color(80, 100, 140));
    target.draw(thumb);
}

// ============================================================================
// ContractUI 实现
// ============================================================================
ContractUI& ContractUI::Instance() {
    static ContractUI instance;
    return instance;
}

void ContractUI::Initialize() {
    panel_.Initialize();
}

void ContractUI::Show() {
    panel_.Show();
}

void ContractUI::Hide() {
    panel_.Hide();
}

void ContractUI::Update(float delta_seconds) {
    panel_.Update(delta_seconds);
}

bool ContractUI::HandleInput(const sf::Event& event) {
    return panel_.HandleInput(event);
}

void ContractUI::Render(sf::RenderTarget& target) {
    panel_.Render(target);
}

void ContractUI::TriggerInteraction() {
    if (!panel_.IsVisible()) {
        Show();
    }
}

}  // namespace CloudSeamanor::engine
