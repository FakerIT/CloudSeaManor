#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"
#include "CloudSeamanor/SaveSlotManager.hpp"

namespace CloudSeamanor::infrastructure {
class ResourceManager;
}

#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <string>

namespace CloudSeamanor::engine {

class PixelSettingsPanel : public PixelUiPanel {
public:
    struct TextConfig {
        std::string slots_title = "存档槽位（1-3）";
        std::string save_slot_prefix = "保存到槽位:";
        std::string load_slot_prefix = "读取槽位:";
        std::string bgm_prefix = "BGM 音量:";
        std::string sfx_prefix = "SFX 音量:";
        std::string display_mode_prefix = "显示模式:";
        std::string fullscreen_text = "全屏";
        std::string windowed_text = "窗口";
        std::string operation_hint = "操作: WS 选择, AD 调整, Enter 应用, Esc 关闭";
        std::string slot_prefix = "槽位";
        std::string empty_slot_text = "空";
        std::string has_save_text = "有存档";
        std::string day_prefix = "Day ";
        std::string no_preview_text = "无预览";
    };

    PixelSettingsPanel();

    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }
    void SetResourceManager(infrastructure::ResourceManager* rm) { resource_manager_ = rm; }
    void SetVisible(bool visible);
    [[nodiscard]] bool IsVisible() const { return visible_; }

    void MoveSelection(int delta);
    void AdjustValue(int delta);
    void SetSlots(const std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata>& slots);
    void SetTextConfig(const TextConfig& text_config) { text_config_ = text_config; }
    void SetRuntimeValues(float bgm_volume, float sfx_volume, bool fullscreen);

    [[nodiscard]] int SelectedSlot() const { return selected_slot_; }
    [[nodiscard]] int SelectedRow() const { return selected_row_; }
    [[nodiscard]] float BgmVolume() const { return bgm_volume_; }
    [[nodiscard]] float SfxVolume() const { return sfx_volume_; }
    [[nodiscard]] bool Fullscreen() const { return fullscreen_; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    void RefreshThumbnails_();

    const PixelFontRenderer* font_renderer_ = nullptr;
    infrastructure::ResourceManager* resource_manager_ = nullptr;
    bool visible_ = false;
    int selected_row_ = 0;
    int selected_slot_ = 1;
    float bgm_volume_ = 0.7f;
    float sfx_volume_ = 1.0f;
    bool fullscreen_ = false;
    std::array<CloudSeamanor::infrastructure::SaveSlotMetadata, 3> slots_{};
    TextConfig text_config_{};

    struct ThumbnailCache {
        std::string path;
        sf::Texture texture;
        bool loaded = false;
    };
    std::array<ThumbnailCache, 3> thumbnails_{};
};

}  // namespace CloudSeamanor::engine
