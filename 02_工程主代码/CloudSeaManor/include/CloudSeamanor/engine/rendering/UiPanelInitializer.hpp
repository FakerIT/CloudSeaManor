#pragma once

// ============================================================================
// 【UiPanelInitializer】UI 面板初始化器
// ============================================================================
// Responsibilities:
// - Define layout constants (positions, sizes, alphas)
// - Initialize all HUD panels (shape, fill, outline)
// - Initialize all HUD texts (font, size, color, position)
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"

namespace CloudSeamanor::engine {

class UiPanelInitializer {
public:
    static void InitializePanels(
        UiPanels& panels,
        const infrastructure::UiLayoutConfig* config = nullptr);
    static void InitializeTexts(
        const sf::Font& font,
        UiTexts& texts,
        const infrastructure::UiLayoutConfig* config = nullptr);
};

}  // namespace CloudSeamanor::engine
