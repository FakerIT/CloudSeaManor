#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"
#include "CloudSeamanor/SaveSlotManager.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <string>

namespace CloudSeamanor::engine {

class PixelSettingsPanel : public PixelUiPanel {
public:
    PixelSettingsPanel();

    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }
    void SetVisible(bool visible);
    [[nodiscard]] bool IsVisible() const { return visible_; }

    void MoveSelection(int delta);
    void AdjustValue(int delta);
    void SetSlots(const std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata>& slots);

    [[nodiscard]] int SelectedSlot() const { return selected_slot_; }
    [[nodiscard]] int SelectedRow() const { return selected_row_; }
    [[nodiscard]] float BgmVolume() const { return bgm_volume_; }
    [[nodiscard]] float SfxVolume() const { return sfx_volume_; }
    [[nodiscard]] bool Fullscreen() const { return fullscreen_; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    void RefreshThumbnails_();

    const PixelFontRenderer* font_renderer_ = nullptr;
    bool visible_ = false;
    int selected_row_ = 0;
    int selected_slot_ = 1;
    float bgm_volume_ = 0.7f;
    float sfx_volume_ = 1.0f;
    bool fullscreen_ = false;
    std::array<CloudSeamanor::infrastructure::SaveSlotMetadata, 3> slots_{};

    struct ThumbnailCache {
        std::string path;
        sf::Texture texture;
        bool loaded = false;
    };
    std::array<ThumbnailCache, 3> thumbnails_{};
};

}  // namespace CloudSeamanor::engine
