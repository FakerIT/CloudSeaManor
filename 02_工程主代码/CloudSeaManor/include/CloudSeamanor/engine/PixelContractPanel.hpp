#pragma once

#include <string>
#include <vector>
#include <functional>
#include "SFML/Graphics.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【ContractBundleData】契约捆绑包数据（必须放在 ContractItemData 之前声明）
// ============================================================================
struct ContractBundleData {
    std::string item_name;
    std::string description;
    int required = 0;
    int collected = 0;
    bool is_complete = false;
};

// ============================================================================
// 【ContractItemData】契约条目显示数据
// ============================================================================
struct ContractItemData {
    std::string id;
    std::string name;
    std::string description;
    std::string spirit_color;
    std::vector<ContractBundleData> bundles;
    float progress = 0.0f;
    bool is_unlocked = false;
    bool is_completed = false;
    bool is_rewarded = false;
    bool can_claim_reward = false;
};

// ============================================================================
// 【ContractPanelViewData】契约面板视图数据
// ============================================================================
using ContractPanelViewData = std::vector<ContractItemData>;

// ============================================================================
// 【PixelContractPanel】契约面板
// ============================================================================
class PixelContractPanel {
public:
    PixelContractPanel();
    ~PixelContractPanel() = default;

    // ========================================================================
    // 初始化
    // ========================================================================
    void Initialize();

    // ========================================================================
    // 显示/隐藏
    // ========================================================================
    void Show();
    void Hide();
    bool IsVisible() const { return is_visible_; }

    // ========================================================================
    // 更新
    // ========================================================================
    void Update(float delta_seconds);
    void Render(sf::RenderTarget& target);

    // ========================================================================
    // 输入处理
    // ========================================================================
    bool HandleInput(const sf::Event& event);

    // ========================================================================
    // 位置
    // ========================================================================
    void SetPosition(float x, float y);
    sf::Vector2f GetPosition() const { return {x_, y_}; }

private:
    // ========================================================================
    // UI 参数
    // ========================================================================
    static constexpr float kWidth = 550.0f;
    static constexpr float kHeight = 550.0f;
    static constexpr float kPadding = 20.0f;
    static constexpr float kContractHeight = 70.0f;
    static constexpr float kBundleItemHeight = 30.0f;

    float x_ = 0.0f;
    float y_ = 0.0f;
    bool is_visible_ = false;

    // 滚动
    float scroll_offset_ = 0.0f;
    float max_scroll_ = 0.0f;

    // 选择
    int selected_contract_ = 0;
    bool show_bundle_detail_ = false;

    // 动画
    bool is_closing_ = false;
    float close_animation_ = 0.0f;

    // 数据
    std::vector<ContractItemData> contracts_;

    // ========================================================================
    // 渲染方法
    // ========================================================================
    void RenderBackground(sf::RenderTarget& target);
    void RenderTitle(sf::RenderTarget& target);
    void RenderContractList(sf::RenderTarget& target);
    void RenderContractItem(sf::RenderTarget& target, const ContractItemData& contract, float y);
    void RenderBundleDetail(sf::RenderTarget& target, const ContractItemData& contract);
    void RenderProgressBar(sf::RenderTarget& target, float x, float y, float width, float progress);
    void RenderCloseButton(sf::RenderTarget& target);
    void RenderScrollBar(sf::RenderTarget& target);

    // ========================================================================
    // 数据加载
    // ========================================================================
    void LoadContractData();

    // ========================================================================
    // 工具方法
    // ========================================================================
    void SelectContract(int index);
    void ClaimReward(const std::string& contract_id);
};

// ============================================================================
// 【ContractUI】契约UI管理器
// ============================================================================
class ContractUI {
public:
    static ContractUI& Instance();

    void Initialize();
    void Show();
    void Hide();
    bool IsVisible() const { return panel_.IsVisible(); }

    void Update(float delta_seconds);
    bool HandleInput(const sf::Event& event);
    void Render(sf::RenderTarget& target);

    // 触发契约界面
    void TriggerInteraction();

private:
    ContractUI() = default;
    ~ContractUI() = default;
    ContractUI(const ContractUI&) = delete;
    ContractUI& operator=(const ContractUI&) = delete;

    PixelContractPanel panel_;
};

}  // namespace CloudSeamanor::engine
