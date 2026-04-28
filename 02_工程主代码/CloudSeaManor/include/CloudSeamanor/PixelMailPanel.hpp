#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct MailPanelEntryViewData {
    std::string sender;
    std::string subject;
    std::string time_text;
};

struct MailPanelViewData {
    std::vector<MailPanelEntryViewData> arrived_entries;
    std::vector<MailPanelEntryViewData> pending_entries;
    std::string detail_text;
    int unread_count = 0;
    std::string list_title_text = "邮件列表: 发件人 / 主题 / 时间";
    std::string arrived_title_text = "已到达";
    std::string pending_title_text = "未到达";
    std::string empty_detail_text = "详情: 暂无邮件内容";
    std::string unread_prefix_text = "❗ 新邮件";
    std::string unread_suffix_text = "封";
    std::string actions_text = "[收取物品] [删除邮件]";
};

class PixelMailPanel : public PixelUiPanel {
public:
    PixelMailPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const MailPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    MailPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
