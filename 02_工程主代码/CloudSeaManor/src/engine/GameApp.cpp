#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameApp.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"
#include "CloudSeamanor/PixelQuestMenu.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <algorithm>
#include <array>
#include <filesystem>

namespace CloudSeamanor::engine {

namespace {

bool IsAutoFontSpec(const std::string& value) {
    return value.empty() || value == "default" || value == "auto";
}

sf::View ComputeIntegerScaleView(unsigned int window_w, unsigned int window_h) {
    const float base_w = static_cast<float>(ScreenConfig::Width);
    const float base_h = static_cast<float>(ScreenConfig::Height);
    const float sx = static_cast<float>(window_w) / base_w;
    const float sy = static_cast<float>(window_h) / base_h;
    const float raw = std::min(sx, sy);
    const float int_scale = std::max(1.0f, std::floor(raw));

    const float vp_w = (base_w * int_scale) / static_cast<float>(window_w);
    const float vp_h = (base_h * int_scale) / static_cast<float>(window_h);
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

}  // namespace

GameApp::~GameApp() = default;

int GameApp::Run() {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 0;

    sf::RenderWindow window(
        sf::VideoMode({1280u, 720u}),
        "云海山庄 / Cloud Sea Manor",
        sf::Style::Titlebar | sf::Style::Close,
        sf::State::Windowed,
        settings);
    window.setVerticalSyncEnabled(true);
    window.setView(ComputeIntegerScaleView(window.getSize().x, window.getSize().y));
    window_ptr_ = &window;

    resources_ = std::make_unique<CloudSeamanor::infrastructure::ResourceManager>();
    resources_->PreloadBundle("core");

    (void)ui_layout_config_.LoadFromFile("configs/ui_layout.json");

    audio_ = std::make_unique<CloudSeamanor::engine::audio::AudioManager>();
    audio_->Initialize();
    (void)audio_->LoadConfig("configs/audio.json");

    ui_system_ = std::make_unique<UISystem>();
    hud_renderer_ = std::make_unique<HudRenderer>(*ui_system_);
    pixel_hud_ = std::make_unique<PixelGameHud>();

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
        pixel_hud_->Initialize(resources_->GetFont(font_id), &ui_layout_config_);
        pixel_hud_->SetUiScale(ComputeIntegerScale(window.getSize().x, window.getSize().y));
        pixel_hud_->SetUiEventCallback([this](PixelGameHud::UiEventType event_type) {
            if (!audio_) return;
            switch (event_type) {
            case PixelGameHud::UiEventType::Open: (void)audio_->PlaySFX("ui_open"); break;
            case PixelGameHud::UiEventType::Close: (void)audio_->PlaySFX("ui_close"); break;
            case PixelGameHud::UiEventType::Select: (void)audio_->PlaySFX("ui_select"); break;
            case PixelGameHud::UiEventType::Error: (void)audio_->PlaySFX("ui_error"); break;
            case PixelGameHud::UiEventType::Achievement: (void)audio_->PlaySFX("achievement_unlock"); break;
            }
        });
    }
    InitializeMainMenu_(font_loaded ? &resources_->GetFont(font_id) : nullptr);

    GameRuntimeCallbacks cbs;
    cbs.push_hint = [this](const std::string& msg, float dur) {
        SetHintMessage(runtime_.WorldState(), msg, dur);
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
    RunWithLoading_("正在加载地图与系统", [this, &cbs]() {
        runtime_.Initialize(
            "configs/gameplay.cfg",
            "assets/data/Schedule_Data.csv",
            "assets/data/Gift_Preference.json",
            "assets/data/NPC_Texts.json",
            "assets/maps/prototype_farm.tmx",
            cbs);
    });
    SetupInputCallbacks_();

    sf::Clock frame_clock;
    while (window.isOpen()) {
        ProcessEvents(window);
        const float dt = frame_clock.restart().asSeconds();
        Update(dt);
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
            continue;
        }

        ForwardEventToHud_(event);

        input_handler_.HandleEvent(event);
    }
}

void GameApp::Update(float delta_seconds) {
    loading_screen_.Update(delta_seconds);
    if (audio_) {
        audio_->Update(delta_seconds);
    }
    if (show_main_menu_) {
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

    UpdatePixelHud_(delta_seconds);
}

void GameApp::UpdateUi_() {
    // 当前版本：UI 的逐帧更新在 Update() 中统一执行；这里保留接口以兼容回调。
}

void GameApp::InitializeMainMenu_(const sf::Font* font) {
    main_menu_panel_.setSize({560.0f, 380.0f});
    main_menu_panel_.setPosition({360.0f, 170.0f});
    main_menu_panel_.setFillColor(sf::Color(32, 26, 20, 240));
    main_menu_panel_.setOutlineThickness(2.0f);
    main_menu_panel_.setOutlineColor(sf::Color(110, 88, 60));

    if (font == nullptr) {
        main_menu_title_.reset();
        for (auto& item : main_menu_items_) {
            item.reset();
        }
        show_main_menu_ = false;
        return;
    }

    main_menu_title_ = std::make_unique<sf::Text>(*font);
    main_menu_title_->setCharacterSize(52u);
    {
        const auto* begin = reinterpret_cast<const char*>(u8"云海山庄");
        const auto* end = begin + (sizeof(u8"云海山庄") - 1);
        main_menu_title_->setString(sf::String::fromUtf8(begin, end));
    }
    main_menu_title_->setPosition({520.0f, 220.0f});

    const auto MakeUtf8 = [](const char8_t* literal) -> sf::String {
        const auto* begin = reinterpret_cast<const char*>(literal);
        const auto* end = begin;
        while (*end != '\0') {
            ++end;
        }
        return sf::String::fromUtf8(begin, end);
    };
    const std::array<sf::String, 3> menu_labels{
        MakeUtf8(u8"开始游戏"),
        MakeUtf8(u8"继续游戏"),
        MakeUtf8(u8"设置"),
    };

    for (int i = 0; i < 3; ++i) {
        main_menu_items_[i] = std::make_unique<sf::Text>(*font);
        main_menu_items_[i]->setCharacterSize(34u);
        main_menu_items_[i]->setString(menu_labels[static_cast<std::size_t>(i)]);
        main_menu_items_[i]->setPosition({560.0f, 320.0f + static_cast<float>(i) * 58.0f});
    }
}

void GameApp::HandleWindowResize_(const sf::Event::Resized& resize_event, sf::RenderWindow& window) {
    window.setView(ComputeIntegerScaleView(resize_event.size.x, resize_event.size.y));
    if (pixel_hud_) {
        pixel_hud_->SetUiScale(ComputeIntegerScale(resize_event.size.x, resize_event.size.y));
    }
}

void GameApp::ForwardEventToHud_(const sf::Event& event) {
    if (!pixel_hud_) {
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        (void)pixel_hud_->HandleKeyPressed(*key);
        return;
    }
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        pixel_hud_->HandleMouseMove(static_cast<float>(move->position.x), static_cast<float>(move->position.y));
        return;
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
        const bool consumed = pixel_hud_->HandleMouseClick(
            static_cast<float>(click->position.x),
            static_cast<float>(click->position.y),
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

    const auto& world_state = runtime_.WorldState();
    pixel_hud_->Update(delta_seconds, &world_state.GetInteraction().dialogue_engine);
    pixel_hud_->UpdateTopRightInfo(
        "第" + std::to_string(world_state.GetClock().Day()) + "天 " + world_state.GetClock().TimeText(),
        world_state.GetClock().DateText(),
        runtime_.Systems().GetCloud().CurrentStateText(),
        false);
    pixel_hud_->UpdateStaminaBar(world_state.GetStamina().Ratio(), world_state.GetStamina().Current(), world_state.GetStamina().Max());
    pixel_hud_->UpdateCoins(world_state.GetGold());

    std::vector<Quest> quests;
    quests.reserve(world_state.GetRuntimeQuests().size());
    for (const auto& runtime_quest : world_state.GetRuntimeQuests()) {
        Quest quest;
        quest.id = runtime_quest.id;
        quest.title = runtime_quest.title;
        quest.description = runtime_quest.description;
        quest.objective = runtime_quest.objective;
        quest.reward_text = runtime_quest.reward;
        if (runtime_quest.state == QuestState::NotTaken) {
            quest.status = QuestStatus::Available;
        } else if (runtime_quest.state == QuestState::InProgress) {
            quest.status = QuestStatus::Active;
        } else {
            quest.status = QuestStatus::Completed;
        }

        if (runtime_quest.id == "daily_commission_tea") {
            quest.progress_max = 3;
            quest.progress_current = std::min(3, world_state.GetInventory().CountOf("TeaLeaf"));
        } else if (runtime_quest.id == "tool_upgrade_intro") {
            quest.progress_max = 2;
            quest.progress_current = std::min(2, world_state.GetMainHouseRepair().level);
        }
        quests.push_back(std::move(quest));
    }
    pixel_hud_->UpdateQuests(quests);

    pixel_hud_->GetMinimap().SetWorldBounds(world_state.GetConfig().world_bounds);
    pixel_hud_->GetMinimap().UpdatePlayerPosition(CloudSeamanor::adapter::ToSf(world_state.GetPlayer().GetPosition()));
    pixel_hud_->GetMinimap().SetLocationText("云海农场 · 主屋前");

    CloudForecastViewData forecast_view;
    const auto& cloud_system = runtime_.Systems().GetCloud();
    forecast_view.today_state_text = cloud_system.CurrentStateText();
    forecast_view.tomorrow_state_text = cloud_system.IsForecastVisible() ? cloud_system.ForecastStateText() : "22:00后公布";
    forecast_view.tide_countdown_days = (cloud_system.ForecastState() == CloudSeamanor::domain::CloudState::Tide) ? 1 : 2;
    forecast_view.crop_bonus_percent = static_cast<int>((runtime_.CloudMultiplier() - 1.0f) * 100.0f);
    forecast_view.spirit_bonus = cloud_system.SpiritEnergyGain();
    forecast_view.recommendations = BuildDailyRecommendations(
        world_state.GetClock(),
        cloud_system.CurrentState(),
        cloud_system,
        world_state.GetMainHouseRepair(),
        world_state.GetInventory(),
        world_state.GetTeaMachine(),
        world_state.GetSpiritBeast(),
        world_state.GetSpiritBeastWateredToday(),
        world_state.GetTeaPlots(),
        world_state.GetNpcs());
    pixel_hud_->UpdateCloudForecast(forecast_view);

    PlayerStatusViewData status_view;
    status_view.player_name = "云海旅人";
    status_view.player_level = runtime_.Systems().GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
    status_view.manor_stage = world_state.GetMainHouseRepair().level;
    status_view.total_gold = world_state.GetGold();
    status_view.stamina_ratio = world_state.GetStamina().Ratio();
    status_view.spirit_ratio = std::min(1.0f, std::max(0.0f, static_cast<float>(runtime_.Systems().GetCloud().SpiritEnergy()) / 200.0f));
    status_view.fatigue_ratio = std::min(1.0f, std::max(0.0f, 1.0f - world_state.GetStamina().Ratio()));
    status_view.contract_progress = runtime_.Systems().GetContracts().CompletedVolumeCount();
    pixel_hud_->UpdatePlayerStatus(status_view);
}

void GameApp::Render(sf::RenderWindow& window) {
    if (show_main_menu_) {
        window.clear(sf::Color(20, 18, 16));
        RenderMainMenu_(window);
        window.display();
        return;
    }

    window.clear(BgColor_(ui_layout_config_, runtime_.Systems().GetCloud().CurrentState()));
    world_renderer_.Render(window, runtime_.WorldState());
    if (hud_renderer_) {
        hud_renderer_->Render(window, runtime_.WorldState());
    }
    if (pixel_hud_) {
        pixel_hud_->Render(window);
    }
    runtime_.RenderSceneTransition(window);
    loading_screen_.Render(window, pixel_hud_ ? pixel_hud_->GetFontRenderer() : nullptr);
    window.display();
}

void GameApp::RunWithLoading_(const std::string& stage_text, const std::function<void()>& fn) {
    if (window_ptr_ == nullptr || fn == nullptr) {
        if (fn) fn();
        return;
    }
    loading_screen_.SetStageText(stage_text);
    loading_screen_.SetVisible(true);

    window_ptr_->clear(sf::Color(18, 18, 18));
    if (show_main_menu_) {
        RenderMainMenu_(*window_ptr_);
    } else {
        world_renderer_.Render(*window_ptr_, runtime_.WorldState());
        if (hud_renderer_) hud_renderer_->Render(*window_ptr_, runtime_.WorldState());
        if (pixel_hud_) pixel_hud_->Render(*window_ptr_);
    }
    loading_screen_.Render(*window_ptr_, pixel_hud_ ? pixel_hud_->GetFontRenderer() : nullptr);
    window_ptr_->display();
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
