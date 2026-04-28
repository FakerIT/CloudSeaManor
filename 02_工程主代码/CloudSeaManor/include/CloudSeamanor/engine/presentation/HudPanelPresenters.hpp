#pragma once

#include "CloudSeamanor/GameRuntime.hpp"
#include "CloudSeamanor/PixelGameHud.hpp"

namespace CloudSeamanor::engine::audio {
class AudioManager;
}

namespace CloudSeamanor::engine {

class HudPanelPresenters {
public:
    static void ApplyRuntimeConfiguration(
        PixelGameHud& hud,
        GameRuntime& runtime,
        const engine::audio::AudioManager* audio,
        bool fullscreen);

    static void UpdateTeaGardenPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateFestivalPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateShopPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateMailPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateAchievementPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateSpiritBeastPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateBuildingPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateContractPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateNpcDetailPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateSpiritRealmPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateBeastiaryPanel(PixelGameHud& hud, GameRuntime& runtime);
    static void UpdateWorkshopPanel(PixelGameHud& hud, GameRuntime& runtime);
};

}  // namespace CloudSeamanor::engine

