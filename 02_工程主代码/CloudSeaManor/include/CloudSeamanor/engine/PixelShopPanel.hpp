#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct ShopPanelItemViewData {
    std::string name;
    int price = 0;
    int stock = 0;
};

struct ShopPanelViewData {
    std::string shop_name = "云海杂货铺";
    int player_gold = 0;
    std::vector<ShopPanelItemViewData> items;
    std::string selected_item_desc;
    std::string items_header_text = "商品列表: 名称 / 价格 / 库存";
    std::string player_gold_prefix = "玩家金币:";
    std::string selected_item_empty_text = "选中商品: 暂无";
    std::string actions_text = "[购买] [取消]";
};

class PixelShopPanel : public PixelUiPanel {
public:
    PixelShopPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const ShopPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    ShopPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
