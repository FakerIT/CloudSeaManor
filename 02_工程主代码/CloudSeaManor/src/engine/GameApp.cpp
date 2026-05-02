#include "CloudSeamanor/app/GameApp.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/domain/RecipeData.hpp"
#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/engine/UiAtlasMappings.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"
#include "CloudSeamanor/engine/PixelQuestMenu.hpp"
#include "CloudSeamanor/engine/presentation/HudPresenter.hpp"
#include "CloudSeamanor/engine/presentation/HudPanelPresenters.hpp"
#include "CloudSeamanor/engine/presentation/HudSideEffects.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"
#include "CloudSeamanor/infrastructure/SpriteMapping.hpp"

// ============================================================================
// 新系统头文件
// ============================================================================
#include "CloudSeamanor/FishingSystem.hpp"
#include "CloudSeamanor/engine/FishingMiniGame.hpp"
#include "CloudSeamanor/MysticMirrorSystem.hpp"
#include "CloudSeamanor/engine/PixelMysticMirrorPanel.hpp"
#include "CloudSeamanor/ContractSystem.hpp"
#include "CloudSeamanor/engine/PixelContractPanel.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string_view>

// RE-206 过渡层：UI 状态已打包到 ui_state_。
// 保留旧成员名映射，降低本次重构改动面，后续可继续清理。
#define runtime_ready_ ui_state_.runtime_ready
#define pending_exit_ ui_state_.pending_exit
#define spirit_name_input_ ui_state_.spirit_name_input
#define pending_save_overwrite_slot_ ui_state_.pending_save_overwrite_slot
#define pending_save_overwrite_seconds_ ui_state_.pending_save_overwrite_seconds
#define main_menu_ ui_state_.main_menu

#define show_main_menu_ main_menu_.visible
#define main_menu_index_ main_menu_.index
#define main_menu_has_save_ main_menu_.has_save
#define pending_main_menu_action_ main_menu_.pending_action
#define main_menu_transition_out_ main_menu_.transition_out
#define main_menu_fade_alpha_ main_menu_.fade_alpha
#define main_menu_anim_time_ main_menu_.anim_time
#define main_menu_hover_lerp_ main_menu_.hover_lerp
#define main_menu_panel_ main_menu_.panel
#define main_menu_background_sprite_ main_menu_.background_sprite
#define main_menu_background_loaded_ main_menu_.background_loaded
#define main_menu_corner_blocks_ main_menu_.corner_blocks
#define main_menu_button_rects_ main_menu_.button_rects
#define main_menu_title_ main_menu_.title
#define main_menu_items_ main_menu_.items
#define main_menu_save_preview_text_ main_menu_.save_preview_text
#define main_menu_status_text_ main_menu_.status_text

namespace CloudSeamanor::engine {

using CloudSeamanor::infrastructure::Logger;

// ============================================================================
// 【SpriteThemeManager】多主题皮肤管理器 - 实现
// ============================================================================
bool SpriteThemeManager::LoadThemeConfig(const std::string& config_path) {
    namespace fs = std::filesystem;

    themes_.clear();

    if (!fs::exists(config_path)) {
        // 如果配置文件不存在，使用默认主题
        Theme default_theme;
        default_theme.id = "default";
        default_theme.name = "default";
        default_theme.display_name = "默认主题";
        default_theme.mapping_csv = "assets/configs/sprite_mapping.csv";
        default_theme.description = "云海山庄默认美术资源";
        themes_.push_back(default_theme);
        current_theme_id_ = "default";
        return true;
    }

    std::ifstream file(config_path);
    if (!file.is_open()) {
        Logger::Error("SpriteThemeManager: Cannot open config: " + config_path);
        return false;
    }

    std::string line;
    bool header_found = false;
    std::vector<std::string> headers;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto fields = ParseCsvLine_(line);
        if (!header_found) {
            headers = fields;
            header_found = true;
            continue;
        }

        if (fields.size() < 4) continue;

        Theme theme;
        for (size_t i = 0; i < fields.size() && i < headers.size(); ++i) {
            if (headers[i] == "Id") theme.id = Trim(fields[i]);
            else if (headers[i] == "Name") theme.name = Trim(fields[i]);
            else if (headers[i] == "DisplayName") theme.display_name = Trim(fields[i]);
            else if (headers[i] == "MappingCsv") theme.mapping_csv = Trim(fields[i]);
            else if (headers[i] == "Description") theme.description = Trim(fields[i]);
        }

        if (!theme.id.empty()) {
            themes_.push_back(theme);
        }
    }

    if (themes_.empty()) {
        Theme fallback;
        fallback.id = "default";
        fallback.display_name = "默认主题";
        fallback.mapping_csv = "assets/configs/sprite_mapping.csv";
        themes_.push_back(fallback);
    }

    current_theme_id_ = themes_.front().id;
    Logger::Info("SpriteThemeManager: Loaded " + std::to_string(themes_.size()) + " themes");
    return true;
}

const SpriteThemeManager::Theme* SpriteThemeManager::GetCurrentTheme() const {
    for (const auto& theme : themes_) {
        if (theme.id == current_theme_id_) {
            return &theme;
        }
    }
    return nullptr;
}

bool SpriteThemeManager::SwitchTheme(const std::string& theme_id) {
    // 查找主题
    const Theme* target = nullptr;
    for (const auto& theme : themes_) {
        if (theme.id == theme_id) {
            target = &theme;
            break;
        }
    }

    if (!target) {
        Logger::Warning("SpriteThemeManager: Theme not found: " + theme_id);
        return false;
    }

    // 加载新映射表
    current_mapping_.LoadFromFile(target->mapping_csv);
    current_theme_id_ = theme_id;

    Logger::Info("SpriteThemeManager: Switched to theme: " + target->display_name);
    return true;
}

std::vector<std::string> SpriteThemeManager::ParseCsvLine_(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

std::string SpriteThemeManager::Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) ++start;
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) --end;
    return str.substr(start, end - start);
}

namespace {

bool IsAutoFontSpec(const std::string& value) {
    return value.empty() || value == "default" || value == "auto";
}

std::string PathToUtf8String(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

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

std::vector<MapMarker> BuildRuntimeMapMarkers_(const GameRuntime& runtime) {
    std::vector<MapMarker> markers;
    const auto& ws = runtime.WorldState();
    for (const auto& npc : ws.GetNpcs()) {
        if (npc.heart_level < 4) {
            continue;
        }
        MapMarker marker;
        marker.name = npc.display_name + "@" + (npc.current_location.empty() ? "云海山庄" : npc.current_location);
        marker.world_position = {npc.position.x, npc.position.y};
        marker.color = sf::Color(180, 220, 255);
        marker.is_npc = true;
        markers.push_back(std::move(marker));
    }
    return markers;
}

sf::View ComputeIntegerScaleView(unsigned int window_w, unsigned int window_h) {
    const float base_w = static_cast<float>(ScreenConfig::Width);
    const float base_h = static_cast<float>(ScreenConfig::Height);
    const float sx = static_cast<float>(window_w) / base_w;
    const float sy = static_cast<float>(window_h) / base_h;
    const float uniform_scale = std::max(0.0001f, std::min(sx, sy));

    const float vp_w = (base_w * uniform_scale) / static_cast<float>(window_w);
    const float vp_h = (base_h * uniform_scale) / static_cast<float>(window_h);
    const float vp_x = (1.0f - vp_w) * 0.5f;
    const float vp_y = (1.0f - vp_h) * 0.5f;

    sf::View view(sf::FloatRect({0.0f, 0.0f}, {base_w, base_h}));
    view.setViewport(sf::FloatRect({vp_x, vp_y}, {vp_w, vp_h}));
    return view;
}

float ComputeIntegerScale(unsigned int window_w, unsigned int window_h) {
    const float base_w = static_cast<float>(ScreenConfig::Width);
    const float base_h = static_cast<float>(ScreenConfig::Height);
    const float sx = static_cast<float>(window_w) / base_w;
    const float sy = static_cast<float>(window_h) / base_h;
    return std::max(1.0f, std::floor(std::min(sx, sy)));
}

bool LoadBooleanFromJsonLikeFile(const std::string& path, const std::string& key, bool fallback) {
    std::ifstream in(path);
    if (!in.is_open()) return fallback;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(key) == std::string::npos) continue;
        if (line.find("true") != std::string::npos) return true;
        if (line.find("false") != std::string::npos) return false;
    }
    return fallback;
}

void PreloadCoreUiSfx_(CloudSeamanor::engine::audio::AudioManager& audio) {
    const std::array<std::pair<const char*, const char*>, 7> kCoreSfx = {{
        {"ui_open", "assets/audio/sfx/ui_open.wav"},
        {"ui_close", "assets/audio/sfx/ui_close.wav"},
        {"ui_select", "assets/audio/sfx/ui_select.wav"},
        {"ui_hover", "assets/audio/sfx/ui_hover.wav"},
        {"ui_error", "assets/audio/sfx/ui_error.wav"},
        {"ui_click", "assets/audio/sfx/ui_click.wav"},
        {"achievement_unlock", "assets/audio/sfx/achievement_unlock.wav"},
    }};
    int loaded = 0;
    for (const auto& [id, path] : kCoreSfx) {
        if (audio.PreloadSFX(id, path)) {
            ++loaded;
        }
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "GameApp: 核心 UI 音效预加载完成 loaded=" + std::to_string(loaded)
        + "/" + std::to_string(kCoreSfx.size()));
}

void PreloadCoreAmbient_(CloudSeamanor::engine::audio::AudioManager& audio) {
    const std::array<std::pair<const char*, const char*>, 4> kCoreAmbient = {{
        {"assets/audio/ambient/rain.wav", "assets/audio/ambient/rain.wav"},
        {"assets/audio/ambient/wind_mist.wav", "assets/audio/ambient/wind_mist.wav"},
        {"assets/audio/ambient/wind_strong.wav", "assets/audio/ambient/wind_strong.wav"},
        {"assets/audio/ambient/tide_magic.wav", "assets/audio/ambient/tide_magic.wav"},
    }};
    int loaded = 0;
    for (const auto& [id, path] : kCoreAmbient) {
        if (audio.PreloadSFX(id, path)) {
            ++loaded;
        }
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "GameApp: 天气环境音预加载完成 loaded=" + std::to_string(loaded)
        + "/" + std::to_string(kCoreAmbient.size()));
}

void PreloadCoreGameSfx_(CloudSeamanor::engine::audio::AudioManager& audio) {
    // P15-CONTENT-005: 基础游戏 SFX 预加载（收割/种植/浇水/升级等）
    const std::array<std::pair<const char*, const char*>, 12> kGameSfx = {{
        {"harvest", "assets/audio/sfx/harvest.wav"},
        {"plant", "assets/audio/sfx/plant.wav"},
        {"water", "assets/audio/sfx/water.wav"},
        {"level_up", "assets/audio/sfx/level_up.wav"},
        {"dialogue_continue", "assets/audio/sfx/dialogue_continue.wav"},
        {"dialogue_choice", "assets/audio/sfx/dialogue_choice.wav"},
        {"shop_purchase", "assets/audio/sfx/shop_purchase.wav"},
        {"shop_sell", "assets/audio/sfx/shop_sell.wav"},
        {"gift", "assets/audio/sfx/gift.wav"},
        {"heart_event", "assets/audio/sfx/heart_event.wav"},
        {"heart_gain", "assets/audio/sfx/heart_gain.wav"},
        {"notification", "assets/audio/sfx/notification.wav"},
    }};
    int loaded = 0;
    for (const auto& [id, path] : kGameSfx) {
        if (audio.PreloadSFX(id, path)) {
            ++loaded;
        }
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "GameApp: 基础游戏音效预加载完成 loaded=" + std::to_string(loaded)
        + "/" + std::to_string(kGameSfx.size()));
}

void RunStartupResourceChecklist_() {
    namespace fs = std::filesystem;

    struct ResourceCheckItem {
        std::string label;
        std::string path;
        std::string degrade_strategy;
    };

    const std::initializer_list<ResourceCheckItem> checklist = {
        {"Core directory", "assets", "缺失将导致大部分资源不可用，仅保留日志输出"},
        {"Sprites directory", "assets/sprites", "缺失将禁用图集驱动精灵，改用占位渲染"},
        {"Textures directory", "assets/textures", "缺失将触发图集纹理占位回退"},
        {"Fonts directory", "assets/fonts", "缺失将尝试系统字体自动选择；失败则关闭文本 UI"},
        {"Audio directory", "assets/audio", "缺失将关闭 BGM/SFX 播放，仅保留静默运行"},
        {"Data directory", "assets/data", "缺失将使用内置默认值并跳过外部数据扩展"},
        {"Maps directory", "assets/maps", "缺失将回退为最小可运行场景"},
        {"NPC text data", "assets/data/NPC_Texts.json", "缺失将使用默认 NPC 文本占位"},
        {"Delivery table", "assets/data/NpcDeliveryTable.csv", "缺失将禁用 NPC 委托刷新"},
        {"Farm map", "assets/maps/prototype_farm.tmx", "缺失将回退到内置空地图"},
    };

    int missing_count = 0;
    for (const auto& item : checklist) {
        if (!fs::exists(item.path)) {
            ++missing_count;
            Logger::Warning(
                "StartupResourceChecklist: missing=" + item.label + ", path=" + item.path
                + ", degrade=" + item.degrade_strategy);
        } else {
            Logger::Info("StartupResourceChecklist: ok=" + item.label + ", path=" + item.path);
        }
    }

    if (missing_count == 0) {
        Logger::Info("StartupResourceChecklist: all required startup assets are available.");
        return;
    }
    Logger::Warning("StartupResourceChecklist: missing_count=" + std::to_string(missing_count)
                    + ". Game will continue with degradation mode.");
}

void VerifyAudioIdFileMapping_() {
    namespace fs = std::filesystem;
    const std::initializer_list<std::pair<const char*, const char*>> required_audio = {
        {"scene:farm", "assets/audio/bgm/farmhouse_theme.wav"},
        {"scene:shop", "assets/audio/bgm/shop_theme.wav"},
        {"scene:spirit_realm_layer1", "assets/audio/bgm/spirit_realm_calm.wav"},
        {"scene:spirit_realm_layer2", "assets/audio/bgm/spirit_realm_tension.wav"},
        {"scene:spirit_realm_layer3", "assets/audio/bgm/spirit_realm_epic.wav"},
        {"scene:festival", "assets/audio/bgm/festival_theme.wav"},
        {"scene:tide", "assets/audio/bgm/tide_theme.wav"},
        {"season:spring", "assets/audio/bgm/spring_theme.wav"},
        {"season:summer", "assets/audio/bgm/summer_theme.wav"},
        {"season:autumn", "assets/audio/bgm/autumn_theme.wav"},
        {"season:winter", "assets/audio/bgm/winter_theme.wav"},
        {"sfx:ui_open", "assets/audio/sfx/ui_open.wav"},
        {"sfx:ui_close", "assets/audio/sfx/ui_close.wav"},
        {"sfx:ui_select", "assets/audio/sfx/ui_select.wav"},
        {"sfx:ui_hover", "assets/audio/sfx/ui_hover.wav"},
        {"sfx:ui_error", "assets/audio/sfx/ui_error.wav"},
        {"sfx:ui_click", "assets/audio/sfx/ui_click.wav"},
        {"sfx:achievement_unlock", "assets/audio/sfx/achievement_unlock.wav"},
    };

    int missing = 0;
    for (const auto& [id, path] : required_audio) {
        if (!fs::exists(path)) {
            ++missing;
            Logger::Warning("AudioRouteCheck: missing id=" + std::string(id) + ", path=" + std::string(path));
        }
    }
    if (missing == 0) {
        Logger::Info("AudioRouteCheck: scene_bgm and AudioManager core ids are fully mapped.");
    } else {
        Logger::Warning("AudioRouteCheck: missing_count=" + std::to_string(missing));
    }
}

void VerifyUiAtlasTextureMapping_() {
    namespace fs = std::filesystem;
    const std::initializer_list<std::pair<const char*, const char*>> required_ui = {
        {"atlas:ui_main", "assets/textures/third_party/kenney_tiny-dungeon/PNG/tilemap.png"},
        {"atlas:ui_borders", "assets/textures/third_party/kenney_tiny-dungeon/PNG/tilemap.png"},
        {"atlas:ui_icons", "assets/textures/third_party/kenney_input-prompts-pixel/PNG/tilemap.png"},
        {"fallback:atlas_texture", "assets/textures/third_party/kenney_tiny-town/Tilemap/tilemap_packed.png"},
    };

    int missing = 0;
    for (const auto& [id, path] : required_ui) {
        if (!fs::exists(path)) {
            ++missing;
            Logger::Warning("UiAtlasCheck: missing id=" + std::string(id) + ", path=" + std::string(path));
        }
    }
    if (missing == 0) {
        Logger::Info("UiAtlasCheck: core atlas textures are ready.");
    } else {
        Logger::Warning("UiAtlasCheck: missing_count=" + std::to_string(missing));
    }
}

}  // namespace

GameApp::~GameApp() = default;

int GameApp::Run() {
    // ============================================================================
    // 【设置工作目录】确保资源文件可以正确加载
    // ============================================================================
    namespace fs = std::filesystem;

    // 记录启动时的工作目录
    const fs::path initial_cwd = fs::current_path();
    Logger::Info("GameApp: 初始工作目录: " + PathToUtf8String(initial_cwd));

    // 只使用当前路径及其父路径探测，避免硬编码含中文目录导致编码异常。
    fs::path valid_root;
    fs::path probe = initial_cwd;
    for (int i = 0; i < 8; ++i) {
        if (fs::exists(probe / "assets") && fs::exists(probe / "configs")) {
            valid_root = probe;
            break;
        }
        if (!probe.has_parent_path()) {
            break;
        }
        probe = probe.parent_path();
    }

    // 如果找到有效的项目根目录，切换到该目录
    if (!valid_root.empty() && valid_root != initial_cwd) {
        std::error_code ec;
        fs::current_path(valid_root, ec);
        if (ec) {
            Logger::Warning("GameApp: 无法切换工作目录到: " + PathToUtf8String(valid_root));
        } else {
            Logger::Info("GameApp: 工作目录已切换到: " + PathToUtf8String(valid_root));
        }
    }

    // 验证关键资源路径是否存在
    const bool assets_exist = fs::exists("assets");
    const bool configs_exist = fs::exists("configs");
    const bool fonts_exist = fs::exists("assets/fonts");
    if (!assets_exist) {
        Logger::Warning("GameApp: 'assets' 目录不存在！当前目录: " + PathToUtf8String(fs::current_path()));
    }
    if (!configs_exist) {
        Logger::Warning("GameApp: 'configs' 目录不存在！当前目录: " + PathToUtf8String(fs::current_path()));
    }
    if (!fonts_exist) {
        Logger::Warning("GameApp: 'assets/fonts' 目录不存在！");
    }
    RunStartupResourceChecklist_();
    VerifyAudioIdFileMapping_();
    VerifyUiAtlasTextureMapping_();

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 0;

    sf::RenderWindow window(
        sf::VideoMode({1280u, 720u}),
        "Cloud Sea Manor",
        sf::Style::Titlebar | sf::Style::Close,
        sf::State::Windowed,
        settings);
    window.setVerticalSyncEnabled(true);
    window.setView(ComputeIntegerScaleView(window.getSize().x, window.getSize().y));

    resources_ = std::make_unique<CloudSeamanor::infrastructure::ResourceManager>();
    resources_->PreloadBundle("core");
    CloudSeamanor::rendering::TextFactory::SetResourceManager(resources_.get());

    // 初始化 SpriteMapping 主题系统
    if (theme_manager_.LoadThemeConfig("configs/sprite_themes.csv")) {
        auto* mapping = theme_manager_.GetCurrentMapping();
        if (mapping) {
            resources_->SetSpriteMapping(mapping, "");
            if (mapping->IsLoaded()) {
                // 预加载常用资源
                std::vector<std::string> common_ids = {
                    "ui_icon_coin", "ui_icon_stamina", "ui_icon_spirit",
                    "ui_icon_cloud_clear", "ui_btn_normal_0", "ui_btn_normal_1"
                };
                resources_->PreloadTexturesBySpriteIds(common_ids);
            }
        }
    }

    if (!ui_layout_config_.LoadFromFile("configs/ui_layout.json")) {
        Logger::LogConfigLoadFailure("GameApp: configs/ui_layout.json, using defaults.");
    }

    audio_ = std::make_unique<CloudSeamanor::engine::audio::AudioManager>();
    audio_->Initialize();
    if (!audio_->LoadConfig("configs/audio.json")) {
        Logger::LogConfigLoadFailure("GameApp: configs/audio.json, using built-in audio defaults.");
    }
    audio_->SetResourceManager(resources_.get());
    PreloadCoreUiSfx_(*audio_);
    PreloadCoreAmbient_(*audio_);
    PreloadCoreGameSfx_(*audio_);

    ui_system_ = std::make_unique<UISystem>();
    hud_renderer_ = std::make_unique<HudRenderer>(*ui_system_);
    pixel_hud_ = std::make_unique<PixelGameHud>();
    pixel_hud_->SetResourceManager(resources_.get());

    // 加载节日装饰配置（数据驱动）
    world_renderer_.LoadFestivalDecorations("assets/data/FestivalDecorations.json");

    const auto& layout_data = ui_layout_config_.IsLoaded()
        ? ui_layout_config_.Data()
        : CloudSeamanor::infrastructure::UiLayoutConfig::GetDefaults();

    bool font_loaded = false;
    const std::string font_id = "main_font";
    if (!IsAutoFontSpec(layout_data.primary_font)) {
        font_loaded = TryLoadConfiguredFont_(font_id, layout_data.primary_font);
    }
    if (!font_loaded) {
        font_loaded = resources_->LoadBestAvailableFont(font_id);
        if (font_loaded) {
            resources_->SetMainFontId(font_id);
        }
    }

    if (font_loaded) {
        runtime_.WorldState().InitializeTexts(resources_->GetFont(font_id));
        runtime_.InitializeLoopDebugPanel(resources_->GetFont(font_id));
        pixel_hud_->Initialize(resources_->GetFont(font_id), &ui_layout_config_);
        pixel_hud_->SetUiScale(1.0f);
        pixel_hud_->SetUiEventCallback([this](PixelGameHud::UiEventType event_type) {
            if (!audio_) return;
            switch (event_type) {
            case PixelGameHud::UiEventType::Open: (void)audio_->PlaySFX("ui_open"); break;
            case PixelGameHud::UiEventType::Close: (void)audio_->PlaySFX("ui_close"); break;
            case PixelGameHud::UiEventType::Select: (void)audio_->PlaySFX("ui_select"); break;
            case PixelGameHud::UiEventType::Hover: (void)audio_->PlaySFX("ui_hover"); break;
            case PixelGameHud::UiEventType::Error: (void)audio_->PlaySFX("ui_error"); break;
            case PixelGameHud::UiEventType::Achievement: (void)audio_->PlaySFX("achievement_unlock"); break;
            }
        });
        pixel_hud_->SetPanelActionCallbacks(PixelGameHud::PanelActionCallbacks{
            .contract_cycle_tracking_volume = [this](int delta) { runtime_.CycleTrackingContractVolume(delta); },
            .mail_collect_arrived = [this]() { runtime_.CollectArrivedMail(); },
            .spirit_beast_toggle_dispatch = [this]() { runtime_.ToggleSpiritBeastDispatch(); },
        });
    }
    InitializeMainMenu_(font_loaded ? &resources_->GetFont(font_id) : nullptr, window.getSize());

    GameRuntimeCallbacks cbs;
    cbs.push_hint = [this](const std::string& msg, float dur) {
        const float configured = runtime_.Config().GetFloat("hint_display_duration", dur);
        const float clamped = std::clamp(configured, 0.8f, 8.0f);
        SetHintMessage(runtime_.WorldState(), msg, clamped);
    };
    cbs.push_notification = [this](const std::string& msg) {
        if (pixel_hud_) {
            pixel_hud_->PushNotification(msg);
            return;
        }
        SetHintMessage(runtime_.WorldState(), msg, 3.6f);
    };
    cbs.log_info = [](const std::string& msg) {
        CloudSeamanor::infrastructure::Logger::Info(msg);
    };
    cbs.play_sfx = [this](const std::string& id) {
        if (!audio_) return;
        (void)audio_->PlaySFX(id);
    };
    cbs.play_bgm = [this](const std::string& bgm_path, bool loop, float fade_in, float fade_out) {
        if (!audio_) return;
        if (!audio_->CurrentBGMId().empty() && audio_->CurrentBGMId() != bgm_path) {
            audio_->StopBGM(std::max(0.0f, fade_out));
        }
        audio_->PlayBGM(bgm_path, loop, std::max(0.0f, fade_in));
    };
    cbs.update_hud_text = [this, font_loaded]() {
        if (!font_loaded) return;
        UpdateUi_();
    };
    cbs.refresh_window_title = [this]() { runtime_.RefreshWindowTitle(); };

    runtime_.SetWindow(&window);
    runtime_.SetResourceManager(resources_.get());
    runtime_ready_ = false;
    try {
        RunWithLoading_(window, "正在加载地图与系统", [this, &cbs]() {
            runtime_.Initialize(
                "configs/gameplay.cfg",
                "assets/data/Schedule_Data.csv",
                "assets/data/Gift_Preference.json",
                "assets/data/NPC_Texts.json",
                "assets/maps/prototype_farm.tmx",
                cbs);
        });
        SetupInputCallbacks_();
        runtime_.InitializeLoopCoordinator();
        runtime_ready_ = true;
    } catch (const std::exception& ex) {
        Logger::Error(std::string("Game runtime init failed, keep menu mode: ") + ex.what());
        show_main_menu_ = true;
    }
    if (runtime_ready_ && pixel_hud_) {
        const bool fullscreen = LoadBooleanFromJsonLikeFile("configs/audio.json", "fullscreen", false);
        HudPanelPresenters::ApplyRuntimeConfiguration(
            *pixel_hud_,
            runtime_,
            audio_.get(),
            fullscreen);
    }

    sf::Clock frame_clock;
    while (window.isOpen()) {
        ProcessEvents(window);
        const float dt = frame_clock.restart().asSeconds();
        Update(dt);
        if (pending_exit_) {
            window.close();
            continue;
        }
        Render(window);
    }

    if (audio_) {
        audio_->Shutdown();
    }
    if (resources_) {
        resources_->ReleaseAll();
    }
    return 0;
}

void GameApp::ProcessEvents(sf::RenderWindow& window) {
    while (const auto ev = window.pollEvent()) {
        const sf::Event& event = *ev;
        if (event.is<sf::Event::Closed>()) {
            window.close();
            return;
        }
        if (const auto* rs = event.getIf<sf::Event::Resized>()) {
            HandleWindowResize_(*rs, window);
        }

        // 输入事件：先喂给 InputManager 维护按键状态
        input_.HandleEvent(event);

        if (show_main_menu_) {
            if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
                HandleMainMenuInput_(*key);
            }
            if (const auto* joy = event.getIf<sf::Event::JoystickMoved>()) {
                if (joy->axis == sf::Joystick::Axis::PovY) {
                    if (joy->position > 60.0f) {
                        main_menu_index_ = (main_menu_index_ + kMainMenuItemCount - 1) % kMainMenuItemCount;
                    } else if (joy->position < -60.0f) {
                        main_menu_index_ = (main_menu_index_ + 1) % kMainMenuItemCount;
                    }
                }
            }
            if (const auto* joy_btn = event.getIf<sf::Event::JoystickButtonPressed>()) {
                if (joy_btn->button == 0) {
                    ExecuteMainMenuSelection_();
                }
            }
            if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
                const sf::Vector2f mouse_pos = window.mapPixelToCoords(move->position);
                for (int i = 0; i < kMainMenuItemCount; ++i) {
                    if (main_menu_button_rects_[i].contains(mouse_pos)) {
                        main_menu_index_ = i;
                        break;
                    }
                }
            }
            if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button == sf::Mouse::Button::Left) {
                    const sf::Vector2f mouse_pos = window.mapPixelToCoords(click->position);
                    for (int i = 0; i < kMainMenuItemCount; ++i) {
                        if (main_menu_button_rects_[i].contains(mouse_pos)) {
                            main_menu_index_ = i;
                            ExecuteMainMenuSelection_();
                            break;
                        }
                    }
                }
            }
            continue;
        }

        if (spirit_name_input_.active) {
            if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::Enter) {
                    if (!spirit_name_input_.buffer.empty()) {
                        runtime_.WorldState().MutableSpiritBeast().custom_name = spirit_name_input_.buffer;
                        runtime_.WorldState().MutableInteraction().dialogue_text =
                            "灵兽已命名为：" + spirit_name_input_.buffer;
                        SetHintMessage(runtime_.WorldState(), "命名完成：" + spirit_name_input_.buffer, 2.4f);
                    }
                    spirit_name_input_.active = false;
                    spirit_name_input_.buffer.clear();
                    continue;
                }
                if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    spirit_name_input_.active = false;
                    spirit_name_input_.buffer.clear();
                    SetHintMessage(runtime_.WorldState(), "已取消灵兽命名。", 1.8f);
                    continue;
                }
                if (key->scancode == sf::Keyboard::Scancode::Backspace) {
                    if (!spirit_name_input_.buffer.empty()) {
                        spirit_name_input_.buffer.pop_back();
                    }
                    SetHintMessage(runtime_.WorldState(), "命名中：" + spirit_name_input_.buffer, 1.2f);
                    continue;
                }
            }
            if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
                if (text->unicode >= 32 && text->unicode < 127 && spirit_name_input_.buffer.size() < 12) {
                    spirit_name_input_.buffer.push_back(static_cast<char>(text->unicode));
                    SetHintMessage(runtime_.WorldState(), "命名中：" + spirit_name_input_.buffer, 1.2f);
                }
                continue;
            }
            continue;
        }

        if (runtime_.IsInBattleMode()) {
            if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
                switch (key->scancode) {
                case sf::Keyboard::Scancode::Q: (void)runtime_.HandleBattleKey(0); break;
                case sf::Keyboard::Scancode::W: (void)runtime_.HandleBattleKey(1); break;
                case sf::Keyboard::Scancode::E: (void)runtime_.HandleBattleKey(2); break;
                case sf::Keyboard::Scancode::R: (void)runtime_.HandleBattleKey(3); break;
                case sf::Keyboard::Scancode::Escape: runtime_.ToggleBattlePause(); break;
                case sf::Keyboard::Scancode::X: runtime_.RetreatBattle(); break;
                case sf::Keyboard::Scancode::B: runtime_.RetreatBattle(); break;
                default: break;
                }
            }
            continue;
        }

        // ============================================================================
        // 新系统UI输入处理
        // ============================================================================
        // 钓鱼小游戏输入（最高优先级）
        if (FishingUI::Instance().IsFishing()) {
            FishingUI::Instance().HandleInput(event);
            continue;
        }

        // 观云镜UI输入
        if (MysticMirrorUI::Instance().IsVisible()) {
            if (MysticMirrorUI::Instance().HandleInput(event)) {
                continue;
            }
        }

        // 契约UI输入
        if (ContractUI::Instance().IsVisible()) {
            if (ContractUI::Instance().HandleInput(event)) {
                continue;
            }
        }

        ForwardEventToHud_(event, window);

        input_handler_.HandleEvent(event);
        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            auto& interaction = runtime_.WorldState().MutableInteraction();
            if (interaction.spirit_beast_menu_open) {
                if (key->scancode == sf::Keyboard::Scancode::Num1) {
                    interaction.spirit_beast_menu_selection = 0;
                    SetHintMessage(runtime_.WorldState(), "灵兽互动选择：喂食", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num2) {
                    interaction.spirit_beast_menu_selection = 1;
                    SetHintMessage(runtime_.WorldState(), "灵兽互动选择：抚摸", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num3) {
                    interaction.spirit_beast_menu_selection = 2;
                    SetHintMessage(runtime_.WorldState(), "灵兽互动选择：派遣", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Enter
                    || key->scancode == sf::Keyboard::Scancode::G) {
                    runtime_.HandleGiftInteraction();
                } else if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    interaction.spirit_beast_menu_open = false;
                    SetHintMessage(runtime_.WorldState(), "已关闭灵兽互动菜单。", 1.4f);
                }
                continue;
            }
            if (interaction.pet_shop_menu_open) {
                if (key->scancode == sf::Keyboard::Scancode::Num1) {
                    interaction.pet_shop_menu_selection = 0;
                    SetHintMessage(runtime_.WorldState(), "宠物选购：猫", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num2) {
                    interaction.pet_shop_menu_selection = 1;
                    SetHintMessage(runtime_.WorldState(), "宠物选购：狗", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num3) {
                    interaction.pet_shop_menu_selection = 2;
                    SetHintMessage(runtime_.WorldState(), "宠物选购：鸟", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Enter
                    || key->scancode == sf::Keyboard::Scancode::E) {
                    runtime_.HandlePrimaryInteraction();
                } else if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    interaction.pet_shop_menu_open = false;
                    SetHintMessage(runtime_.WorldState(), "已关闭宠物选购菜单。", 1.4f);
                }
                continue;
            }
            if (interaction.purchaser_menu_open) {
                if (key->scancode == sf::Keyboard::Scancode::Num1) {
                    interaction.purchaser_menu_selection = 0;
                    SetHintMessage(runtime_.WorldState(), "收购选择：第1项", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num2) {
                    interaction.purchaser_menu_selection = 1;
                    SetHintMessage(runtime_.WorldState(), "收购选择：第2项", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Num3) {
                    interaction.purchaser_menu_selection = 2;
                    SetHintMessage(runtime_.WorldState(), "收购选择：第3项", 1.6f);
                } else if (key->scancode == sf::Keyboard::Scancode::Enter
                    || key->scancode == sf::Keyboard::Scancode::E) {
                    runtime_.HandlePrimaryInteraction();
                } else if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    interaction.purchaser_menu_open = false;
                    SetHintMessage(runtime_.WorldState(), "已关闭收购菜单。", 1.4f);
                }
                continue;
            }
            if (key->scancode == sf::Keyboard::Scancode::J) {
                (void)runtime_.TryEnterBattleByPlayerPosition();
            } else if (key->scancode == sf::Keyboard::Scancode::F2) {
                spirit_name_input_.active = true;
                spirit_name_input_.buffer = runtime_.WorldState().GetSpiritBeast().custom_name;
                SetHintMessage(runtime_.WorldState(), "输入灵兽名称（Enter确认 / Esc取消）", 2.4f);
            }

            // ============================================================================
            // 新系统快捷键
            // ============================================================================
            else if (key->scancode == sf::Keyboard::Scancode::F) {
                // 钓鱼（按F键）
                TryStartFishing_();
            } else if (key->scancode == sf::Keyboard::Scancode::G) {
                // 观云镜（按G键）
                if (!MysticMirrorUI::Instance().IsVisible()) {
                    MysticMirrorUI::Instance().Show();
                    SetHintMessage(runtime_.WorldState(), "打开观云镜", 1.5f);
                }
            } else if (key->scancode == sf::Keyboard::Scancode::Y) {
                // 茶灵契约（按Y键）
                if (!ContractUI::Instance().IsVisible()) {
                    ContractUI::Instance().Show();
                    SetHintMessage(runtime_.WorldState(), "打开茶灵契约", 1.5f);
                }
            }
        }
    }
}

void GameApp::Update(float delta_seconds) {
    loading_screen_.Update(delta_seconds);
    if (audio_) {
        audio_->Update(delta_seconds);
    }

    // ============================================================================
    // 新系统UI更新
    // ============================================================================
    FishingUI::Instance().Update(delta_seconds);
    MysticMirrorUI::Instance().Update(delta_seconds);
    ContractUI::Instance().Update(delta_seconds);

    if (pending_save_overwrite_slot_.has_value()) {
        pending_save_overwrite_seconds_ += delta_seconds;
        if (pending_save_overwrite_seconds_ > 3.0f) {
            pending_save_overwrite_slot_.reset();
            pending_save_overwrite_seconds_ = 0.0f;
        }
    }
        if (!pixel_hud_ || !pixel_hud_->GetSettingsPanel().IsVisible()) {
            pending_save_overwrite_slot_.reset();
            pending_save_overwrite_seconds_ = 0.0f;
        }
    if (show_main_menu_) {
        main_menu_anim_time_ += delta_seconds;
        if (main_menu_transition_out_) {
            main_menu_fade_alpha_ = std::max(0.0f, main_menu_fade_alpha_ - delta_seconds * 3.0f);
            if (main_menu_fade_alpha_ <= 0.01f) {
                main_menu_transition_out_ = false;
                PerformMainMenuAction_(pending_main_menu_action_);
                pending_main_menu_action_ = MainMenuAction::None;
            }
        } else {
            main_menu_fade_alpha_ = std::min(1.0f, main_menu_fade_alpha_ + delta_seconds * 2.0f);
        }
        for (int i = 0; i < kMainMenuItemCount; ++i) {
            const float target = (i == main_menu_index_) ? 1.0f : 0.0f;
            main_menu_hover_lerp_[i] += (target - main_menu_hover_lerp_[i]) * std::min(1.0f, delta_seconds * 12.0f);
        }
        return;
    }

    input_.BeginNewFrame();

    // 面板类输入优先（Esc/I/F/M/对话确认等）
    input_handler_.HandlePanelAction(runtime_.WorldState());
    // 游戏类输入（F3/F5/F6/F9/T/G/E 等）
    input_handler_.HandleGameAction(runtime_.WorldState(), runtime_.Systems());

    // 移动向量（面板打开时也允许移动，但对话打开时通常会被挡住）
    const sf::Vector2f move = input_handler_.GetMovementVector();
    if (move.x != 0.0f || move.y != 0.0f) {
        runtime_.OnPlayerMoved(delta_seconds * runtime_.TimeScale(), move);
    }

    runtime_.Update(delta_seconds * runtime_.TimeScale());

    // 更新循环调试面板
    runtime_.UpdateLoopDebugPanel();

    UpdatePixelHud_(delta_seconds);
}

void GameApp::UpdateUi_() {
    if (!pixel_hud_) {
        return;
    }
    pixel_hud_->UpdateMapMarkers(BuildRuntimeMapMarkers_(runtime_));
}

void GameApp::InitializeMainMenu_(const sf::Font* font, sf::Vector2u window_size) {
    main_menu_background_loaded_ = false;
    main_menu_.atlas_texture_id.clear();
    const std::string menu_bg_texture_id = "main_menu_tiny_town_bg";
    const std::string menu_bg_texture_path =
        CloudSeamanor::engine::atlas::kTinyTownTilemapPath;
    if (resources_ && std::filesystem::exists(menu_bg_texture_path)) {
        if (resources_->LoadTexture(menu_bg_texture_id, menu_bg_texture_path)) {
            auto& texture = resources_->GetTexture(menu_bg_texture_id);
            main_menu_background_sprite_ = std::make_unique<sf::Sprite>(texture);
            main_menu_background_loaded_ = true;
        }
        // 主菜单面板图集纹理（与 PixelGameHud 共享）
        const std::string atlas_id = "pixel_hud_ui_atlas";
        if (resources_->LoadTexture(atlas_id, menu_bg_texture_path)) {
            main_menu_.atlas_texture_id = atlas_id;
        }
    }
    main_menu_panel_.setFillColor(sf::Color(32, 26, 20, main_menu_background_loaded_ ? 206 : 240));
    main_menu_panel_.setOutlineThickness(2.0f);
    main_menu_panel_.setOutlineColor(sf::Color(110, 88, 60));

    if (font == nullptr) {
        main_menu_title_.reset();
        for (auto& item : main_menu_items_) {
            item.reset();
        }
        main_menu_save_preview_text_.reset();
        main_menu_status_text_.reset();
        show_main_menu_ = false;
        return;
    }

    main_menu_title_ = std::make_unique<sf::Text>(*font);
    {
        const auto* begin = reinterpret_cast<const char*>(u8"云海山庄");
        const auto* end = begin + (sizeof(u8"云海山庄") - 1);
        main_menu_title_->setString(sf::String::fromUtf8(begin, end));
    }
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
        main_menu_items_[i] = std::make_unique<sf::Text>(*font);
        main_menu_items_[i]->setString(menu_labels[static_cast<std::size_t>(i)]);
    }

    const CloudSeamanor::infrastructure::SaveSlotManager slot_manager;
    const auto slots = slot_manager.ReadAllMetadata();
    main_menu_has_save_ = std::any_of(slots.begin(), slots.end(), [](const auto& slot) {
        return slot.exists;
    });
    if (!main_menu_has_save_) {
        main_menu_items_[1]->setString(MakeUtf8(u8"继续游戏（无存档）"));
    }

    main_menu_save_preview_text_ = std::make_unique<sf::Text>(*font);
    if (main_menu_has_save_) {
        const std::string preview = BuildMainMenuRecentSavePreview_();
        main_menu_save_preview_text_->setString(sf::String::fromUtf8(preview.begin(), preview.end()));
    } else {
        const auto* begin = reinterpret_cast<const char*>(u8"最近存档：无");
        const auto* end = begin + (sizeof(u8"最近存档：无") - 1);
        main_menu_save_preview_text_->setString(sf::String::fromUtf8(begin, end));
    }
    main_menu_status_text_ = std::make_unique<sf::Text>(*font);
    {
        const auto* begin = reinterpret_cast<const char*>(u8"方向键/手柄十字键选择，回车/A确认");
        const auto* end = begin + (sizeof(u8"方向键/手柄十字键选择，回车/A确认") - 1);
        main_menu_status_text_->setString(sf::String::fromUtf8(begin, end));
    }
    main_menu_fade_alpha_ = 0.0f;
    main_menu_transition_out_ = false;
    pending_main_menu_action_ = MainMenuAction::None;
    UpdateMainMenuLayout_(window_size);
}

void GameApp::UpdateMainMenuLayout_(sf::Vector2u window_size) {
    (void)window_size;
    const float w = static_cast<float>(ScreenConfig::Width);
    const float h = static_cast<float>(ScreenConfig::Height);
    const float scale = 1.0f;

    if (main_menu_background_loaded_ && main_menu_background_sprite_) {
        const auto tex_size = main_menu_background_sprite_->getTexture().getSize();
        if (tex_size.x > 0 && tex_size.y > 0) {
            const float sx = w / static_cast<float>(tex_size.x);
            const float sy = h / static_cast<float>(tex_size.y);
            const float cover = std::max(sx, sy);
            const float draw_w = static_cast<float>(tex_size.x) * cover;
            const float draw_h = static_cast<float>(tex_size.y) * cover;
            main_menu_background_sprite_->setScale({cover, cover});
            main_menu_background_sprite_->setPosition({(w - draw_w) * 0.5f, (h - draw_h) * 0.5f});
        }
    }

    const sf::Vector2f panel_size{560.0f * scale, 410.0f * scale};
    main_menu_panel_.setSize(panel_size);
    main_menu_panel_.setPosition({(w - panel_size.x) * 0.5f, (h - panel_size.y) * 0.5f});
    main_menu_panel_.setOutlineThickness(std::max(1.0f, 2.0f * scale));

    if (main_menu_title_) {
        main_menu_title_->setCharacterSize(static_cast<unsigned>(std::round(18.0f * scale)));
        const auto title_bounds = main_menu_title_->getLocalBounds();
        main_menu_title_->setPosition({
            main_menu_panel_.getPosition().x + (panel_size.x - title_bounds.size.x) * 0.5f - title_bounds.position.x,
            main_menu_panel_.getPosition().y + 22.0f * scale
        });
    }
    for (int i = 0; i < 4; ++i) {
        auto& corner = main_menu_corner_blocks_[i];
        corner.setFillColor(sf::Color(120, 82, 46, 255));
        corner.setSize({14.0f * scale, 14.0f * scale});
    }
    main_menu_corner_blocks_[0].setPosition(main_menu_panel_.getPosition());
    main_menu_corner_blocks_[1].setPosition({main_menu_panel_.getPosition().x + panel_size.x - main_menu_corner_blocks_[1].getSize().x, main_menu_panel_.getPosition().y});
    main_menu_corner_blocks_[2].setPosition({main_menu_panel_.getPosition().x, main_menu_panel_.getPosition().y + panel_size.y - main_menu_corner_blocks_[2].getSize().y});
    main_menu_corner_blocks_[3].setPosition({main_menu_panel_.getPosition().x + panel_size.x - main_menu_corner_blocks_[3].getSize().x, main_menu_panel_.getPosition().y + panel_size.y - main_menu_corner_blocks_[3].getSize().y});

    for (int i = 0; i < kMainMenuItemCount; ++i) {
        main_menu_button_rects_[i] = sf::FloatRect{
            {main_menu_panel_.getPosition().x + 70.0f * scale,
             main_menu_panel_.getPosition().y + (96.0f + static_cast<float>(i) * 56.0f) * scale},
            {panel_size.x - 140.0f * scale, 46.0f * scale}
        };
        if (!main_menu_items_[i]) continue;
        main_menu_items_[i]->setCharacterSize(static_cast<unsigned>(std::round(14.0f * scale)));
        const auto b = main_menu_items_[i]->getLocalBounds();
        main_menu_items_[i]->setPosition({
            main_menu_button_rects_[i].position.x + (main_menu_button_rects_[i].size.x - b.size.x) * 0.5f - b.position.x,
            main_menu_button_rects_[i].position.y + (main_menu_button_rects_[i].size.y - b.size.y) * 0.5f - b.position.y - 2.0f * scale
        });
    }
    if (main_menu_save_preview_text_) {
        main_menu_save_preview_text_->setCharacterSize(static_cast<unsigned>(std::round(11.0f * scale)));
        const auto& rect = main_menu_button_rects_[1];
        main_menu_save_preview_text_->setPosition({rect.position.x + rect.size.x + 12.0f * scale, rect.position.y + 10.0f * scale});
    }
    if (main_menu_status_text_) {
        main_menu_status_text_->setCharacterSize(static_cast<unsigned>(std::round(10.0f * scale)));
        const auto b = main_menu_status_text_->getLocalBounds();
        main_menu_status_text_->setPosition({
            main_menu_panel_.getPosition().x + (panel_size.x - b.size.x) * 0.5f - b.position.x,
            main_menu_panel_.getPosition().y + panel_size.y - 24.0f * scale
        });
    }
}

void GameApp::PerformMainMenuAction_(MainMenuAction action) {
    const auto SetStatusUtf8 = [this](const char8_t* literal) {
        if (!main_menu_status_text_) return;
        const auto* begin = reinterpret_cast<const char*>(literal);
        const auto* end = begin;
        while (*end != '\0') {
            ++end;
        }
        main_menu_status_text_->setString(sf::String::fromUtf8(begin, end));
    };

    switch (action) {
    case MainMenuAction::StartGame:
        show_main_menu_ = false;
        break;
    case MainMenuAction::ContinueGame:
        if (!main_menu_has_save_) {
            runtime_.Callbacks().push_hint("当前没有可用存档。", 2.0f);
            SetStatusUtf8(u8"当前无存档：请先开始新游戏。");
            break;
        }
        runtime_.LoadGame();
        show_main_menu_ = false;
        break;
    case MainMenuAction::Settings:
        runtime_.Callbacks().push_hint("设置面板可在游戏内按 Esc 打开。", 2.4f);
        SetStatusUtf8(u8"设置：进入游戏后按 Esc 打开设置面板。");
        main_menu_transition_out_ = false;
        pending_main_menu_action_ = MainMenuAction::None;
        main_menu_fade_alpha_ = 1.0f;
        break;
    case MainMenuAction::About:
        runtime_.Callbacks().push_hint("云海山庄 v0.1.0 - 东方治愈像素农庄", 2.8f);
        SetStatusUtf8(u8"云海山庄 v0.1.0 - 东方治愈像素农庄");
        main_menu_transition_out_ = false;
        pending_main_menu_action_ = MainMenuAction::None;
        main_menu_fade_alpha_ = 1.0f;
        break;
    case MainMenuAction::ExitGame:
        pending_exit_ = true;
        break;
    default:
        break;
    }
}

void GameApp::HandleWindowResize_(const sf::Event::Resized& resize_event, sf::RenderWindow& window) {
    window.setView(ComputeIntegerScaleView(resize_event.size.x, resize_event.size.y));
    UpdateMainMenuLayout_({resize_event.size.x, resize_event.size.y});
    if (pixel_hud_) {
        pixel_hud_->SetUiScale(1.0f);
    }
}

void GameApp::ForwardEventToHud_(const sf::Event& event, sf::RenderWindow& window) {
    if (!pixel_hud_) {
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        (void)pixel_hud_->HandleKeyPressed(*key);
        return;
    }
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        const sf::Vector2f pos = window.mapPixelToCoords(move->position);
        pixel_hud_->HandleMouseMove(pos.x, pos.y);
        return;
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
        const sf::Vector2f pos = window.mapPixelToCoords(click->position);
        const bool consumed = pixel_hud_->HandleMouseClick(
            pos.x,
            pos.y,
            click->button);
        if (consumed && audio_) {
            (void)audio_->PlaySFX("ui_click");
        }
        return;
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        (void)pixel_hud_->HandleMouseWheel(
            static_cast<float>(wheel->position.x),
            static_cast<float>(wheel->position.y),
            wheel->delta);
    }
}

void GameApp::UpdatePixelHud_(float delta_seconds) {
    if (!pixel_hud_) {
        return;
    }

    HudPresenter::UpdateCoreViews(*pixel_hud_, runtime_, input_, delta_seconds);
    HudPanelPresenters::UpdateTeaGardenPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateFestivalPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateShopPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateMailPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateAchievementPanel(*pixel_hud_, runtime_);

    HudPanelPresenters::UpdateSpiritBeastPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateBuildingPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateContractPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateNpcDetailPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateNpcSchedulePanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateSocialPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateSpiritRealmPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateBeastiaryPanel(*pixel_hud_, runtime_);
    HudPanelPresenters::UpdateWorkshopPanel(*pixel_hud_, runtime_);

    const std::string pending_skill = runtime_.PendingSkillBranchSkill();
    std::string option_a = "稳态专精";
    std::string option_b = "爆发专精";
    if (pending_skill == "灵农") {
        option_a = "丰茂";
        option_b = "回春";
    } else if (pending_skill == "灵觅") {
        option_a = "寻珍";
        option_b = "识脉";
    } else if (pending_skill == "灵钓") {
        option_a = "观潮";
        option_b = "深流";
    } else if (pending_skill == "灵矿") {
        option_a = "震脉";
        option_b = "精炼";
    } else if (pending_skill == "灵卫") {
        option_a = "结界";
        option_b = "护阵";
    }
    pixel_hud_->UpdateSkillBranchOverlay(runtime_.HasPendingSkillBranchChoice(), pending_skill, option_a, option_b);
    pixel_hud_->UpdateFishingQteOverlay(runtime_.IsFishingQteActive(),
                                        runtime_.FishingQteProgress(),
                                        runtime_.FishingQteTargetCenter(),
                                        runtime_.FishingQteTargetWidth(),
                                        runtime_.FishingQteLabel());
    pixel_hud_->UpdateDiyPlacementOverlay(runtime_.IsDiyPlacementActive(),
                                          ItemDisplayName(runtime_.DiyPreviewObjectId()),
                                          runtime_.DiyCursorX(),
                                          runtime_.DiyCursorY(),
                                          runtime_.DiyRotation());

    if (const auto choice = pixel_hud_->ConsumeSkillBranchChoice(); choice.has_value()) {
        runtime_.CommitPendingSkillBranch(choice.value());
    }
    if (pixel_hud_->ConsumeFishingQteConfirm()) {
        runtime_.ResolveFishingQte();
    }
    const sf::Vector2i diy_move = pixel_hud_->ConsumeDiyMoveDelta();
    if (diy_move.x != 0 || diy_move.y != 0) {
        runtime_.MoveDiyCursor(diy_move.x, diy_move.y);
    }
    if (pixel_hud_->ConsumeDiyRotate()) {
        runtime_.RotateDiyPreview();
    }
    if (pixel_hud_->ConsumeDiyConfirm()) {
        runtime_.ConfirmDiyPlacement();
    }
    if (pixel_hud_->ConsumeDiyPickup()) {
        runtime_.PickupLastDiyObject();
    }
    HandleHudSideEffects_();
}

HudSideEffects::HudEffectContext GameApp::BuildHudEffectContext_() {
    HudSideEffects::HudEffectContext context;
    context.notify = [this](const std::string& msg, HudSideEffects::NotificationLevel level) {
        (void)level;
        if (pixel_hud_) {
            pixel_hud_->PushNotification(msg);
            return;
        }
        SetHintMessage(runtime_.WorldState(), msg, 3.0f);
    };
    context.play_sfx = [this](const std::string& sfx_id) {
        if (!audio_) return;
        (void)audio_->PlaySFX(sfx_id);
    };
    context.now_seconds = []() {
        using Clock = std::chrono::steady_clock;
        const auto now = Clock::now().time_since_epoch();
        return std::chrono::duration<float>(now).count();
    };
    const auto& cfg = runtime_.Config();
    context.allow_auto_open_festival_panel = cfg.GetFloat("hud_allow_auto_open_festival_panel", 1.0f) >= 0.5f;
    context.allow_auto_open_mail_panel = cfg.GetFloat("hud_allow_auto_open_mail_panel", 1.0f) >= 0.5f;
    context.allow_mail_arrival_notify = cfg.GetFloat("hud_allow_mail_arrival_notify", 1.0f) >= 0.5f;
    context.allow_achievement_notify = cfg.GetFloat("hud_allow_achievement_notify", 1.0f) >= 0.5f;
    context.allow_achievement_unlock_sfx = cfg.GetFloat("hud_allow_achievement_unlock_sfx", 1.0f) >= 0.5f;
    context.sfx_throttle_seconds = std::max(0.0f, cfg.GetFloat("hud_sfx_throttle_seconds", 0.25f));
    return context;
}

// ============================================================================
// 新系统辅助函数
// ============================================================================

void GameApp::TryStartFishing_() {
    // 获取当前地图ID - 使用 tmx_map 获取当前地图
    const std::string& map_id = runtime_.State().tmx_map ? "main_farm" : "main_farm";

    // 确定钓鱼地点
    std::string location_id = "main_farm";
    if (map_id.find("river") != std::string::npos) {
        location_id = "river";
    } else if (map_id.find("spirit_realm") != std::string::npos) {
        location_id = "spirit_realm";
    }

    // 检查玩家是否在水边（简化检测）
    // 实际应通过射线检测或区域判断
    SetHintMessage(runtime_.WorldState(), "开始钓鱼！按 A/D 键保持位置", 2.0f);

    // 计算难度
    auto& fishing_system = domain::FishingSystem::Instance();
    int difficulty = fishing_system.CalculateFinalDifficulty(1, location_id);

    // 获取钓鱼技能等级
    const int fishing_level = fishing_system.GetSkillLevel();

    // 开始钓鱼
    auto& fishing_ui = FishingUI::Instance();
    fishing_ui.StartFishing(difficulty);

    // 设置回调
    fishing_ui.SetOnCatchCallback([this](bool success, const std::string& fish_id) {
        if (success && !fish_id.empty()) {
            // 将鱼添加到背包 - 使用 MutableInventory
            runtime_.WorldState().MutableInventory().AddItem(fish_id, 1);

            // 获取鱼名
            const auto* fish_data = domain::FishingSystem::Instance().GetFishData(fish_id);
            std::string fish_name = fish_data ? fish_data->name_zh : fish_id;

            // 显示提示
            SetHintMessage(runtime_.WorldState(), "钓到了 " + fish_name + "！", 3.0f);
        } else {
            SetHintMessage(runtime_.WorldState(), "狡猾的鱼溜走了...", 2.0f);
        }
    });
}

void GameApp::HandleHudSideEffects_() {
    if (!pixel_hud_) {
        return;
    }
    const auto context = BuildHudEffectContext_();
    HudSideEffects::ApplyAll(*pixel_hud_, runtime_, ui_state_.hud_side_effects, context);
}

void GameApp::Render(sf::RenderWindow& window) {
    if (show_main_menu_) {
        window.clear(sf::Color(20, 18, 16));
        RenderMainMenu_(window);
        window.display();
        return;
    }

    window.clear(BgColor_(ui_layout_config_, runtime_.Systems().GetCloud().CurrentState()));
    if (runtime_.IsInBattleMode()) {
        runtime_.RenderBattle(window);
    } else {
        world_renderer_.Render(window, runtime_.WorldState());
        if (hud_renderer_) {
            hud_renderer_->Render(window, runtime_.WorldState());
        }
        if (pixel_hud_) {
            pixel_hud_->Render(window);
        }
    }

    // ============================================================================
    // 新系统UI渲染
    // ============================================================================
    FishingUI::Instance().Render(window);
    MysticMirrorUI::Instance().Render(window);
    ContractUI::Instance().Render(window);

    runtime_.RenderSceneTransition(window);
    runtime_.RenderLoopDebugPanel(window);
    loading_screen_.Render(window, pixel_hud_ ? pixel_hud_->MutableFontRenderer() : nullptr);
    window.display();
}

void GameApp::RunWithLoading_(sf::RenderWindow& window, const std::string& stage_text, const std::function<void()>& fn) {
    if (!fn) {
        Logger::Warning("RunWithLoading_: null callback received, skipping");
        return;
    }
    loading_screen_.SetStageText(stage_text);
    loading_screen_.SetVisible(true);

    window.clear(sf::Color(18, 18, 18));
    if (show_main_menu_) {
        RenderMainMenu_(window);
    } else {
        world_renderer_.Render(window, runtime_.WorldState());
        if (hud_renderer_) hud_renderer_->Render(window, runtime_.WorldState());
        if (pixel_hud_) pixel_hud_->Render(window);
    }
    loading_screen_.Render(window, pixel_hud_ ? pixel_hud_->MutableFontRenderer() : nullptr);
    window.display();
    sf::sleep(sf::milliseconds(16));

    fn();
    loading_screen_.SetVisible(false);
}

std::string GameApp::CloudStateKey_(const domain::CloudState& state) {
    using CS = domain::CloudState;
    switch (state) {
    case CS::Clear:      return "cloud_clear";
    case CS::Mist:       return "cloud_mist";
    case CS::DenseCloud: return "cloud_dense";
    case CS::Tide:       return "cloud_tide";
    }
    return "cloud_clear";
}

sf::Color GameApp::BgColor_(
    const infrastructure::UiLayoutConfig& layout,
    const domain::CloudState& state) {
    return adapter::PackedRgbaToColor(layout.GetCloudColor(CloudStateKey_(state)).background);
}

sf::Color GameApp::AuraColor_(
    const infrastructure::UiLayoutConfig& layout,
    const domain::CloudState& state) {
    return adapter::PackedRgbaToColor(layout.GetCloudColor(CloudStateKey_(state)).aura);
}

}  // namespace CloudSeamanor::engine
