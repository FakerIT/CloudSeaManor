#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameApp.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/RecipeData.hpp"
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
#include <string_view>

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

std::string PetDisplayName_(std::string_view pet_type) {
    if (pet_type == "cat") return "猫灵";
    if (pet_type == "dog") return "犬灵";
    if (pet_type == "bird") return "羽灵";
    return pet_type.empty() ? "未知灵兽" : std::string(pet_type);
}

std::string AchievementDisplayName_(std::string_view id) {
    if (id == "first_crop") return "初次收获";
    if (id == "ten_crops") return "小有规模";
    if (id == "gift_expert") return "送礼达人";
    if (id == "master_builder") return "大宅建成";
    if (id == "beast_bond") return "初次结缘";
    if (id == "beast_bond_max") return "灵兽羁绊满级";
    if (id == "beast_all_types") return "全类型灵兽结缘";
    if (id == "home_designer") return "家园设计师";
    if (id == "small_tycoon") return "小富豪";
    if (id == "first_pet") return "初次收养";
    if (id == "pet_all_types") return "全类型宠物收集";
    if (id == "pet_collected_cat") return "收集宠物: 猫灵";
    if (id == "pet_collected_dog") return "收集宠物: 犬灵";
    if (id == "pet_collected_bird") return "收集宠物: 羽灵";
    if (id == "beast_type_lively") return "灵兽性格: 活泼";
    if (id == "beast_type_lazy") return "灵兽性格: 慵懒";
    if (id == "beast_type_curious") return "灵兽性格: 好奇";
    return id.empty() ? "未知成就" : std::string(id);
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
            case PixelGameHud::UiEventType::Hover: (void)audio_->PlaySFX("ui_hover"); break;
            case PixelGameHud::UiEventType::Error: (void)audio_->PlaySFX("ui_error"); break;
            case PixelGameHud::UiEventType::Achievement: (void)audio_->PlaySFX("achievement_unlock"); break;
            }
        });
    }
    InitializeMainMenu_(font_loaded ? &resources_->GetFont(font_id) : nullptr);

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
    if (pixel_hud_) {
        const auto& cfg = runtime_.Config();
        PixelSettingsPanel::TextConfig settings_text;
        settings_text.slots_title = cfg.GetString("settings_slots_title", settings_text.slots_title);
        settings_text.save_slot_prefix = cfg.GetString("settings_save_slot_prefix", settings_text.save_slot_prefix);
        settings_text.load_slot_prefix = cfg.GetString("settings_load_slot_prefix", settings_text.load_slot_prefix);
        settings_text.bgm_prefix = cfg.GetString("settings_bgm_prefix", settings_text.bgm_prefix);
        settings_text.sfx_prefix = cfg.GetString("settings_sfx_prefix", settings_text.sfx_prefix);
        settings_text.display_mode_prefix = cfg.GetString("settings_display_mode_prefix", settings_text.display_mode_prefix);
        settings_text.fullscreen_text = cfg.GetString("settings_fullscreen_text", settings_text.fullscreen_text);
        settings_text.windowed_text = cfg.GetString("settings_windowed_text", settings_text.windowed_text);
        settings_text.operation_hint = cfg.GetString("settings_operation_hint", settings_text.operation_hint);
        settings_text.slot_prefix = cfg.GetString("settings_slot_prefix", settings_text.slot_prefix);
        settings_text.empty_slot_text = cfg.GetString("settings_empty_slot_text", settings_text.empty_slot_text);
        settings_text.has_save_text = cfg.GetString("settings_has_save_text", settings_text.has_save_text);
        settings_text.day_prefix = cfg.GetString("settings_day_prefix", settings_text.day_prefix);
        settings_text.no_preview_text = cfg.GetString("settings_no_preview_text", settings_text.no_preview_text);
        pixel_hud_->GetSettingsPanel().SetTextConfig(settings_text);
        pixel_hud_->ConfigureNotificationTimings(
            cfg.GetFloat("ui_notification_fade_in", 0.3f),
            cfg.GetFloat("ui_notification_hold", 3.0f),
            cfg.GetFloat("ui_notification_fade_out", 0.3f),
            cfg.GetFloat("ui_cloud_report_duration", 4.0f));
    }

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

        if (spirit_name_input_active_) {
            if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::Enter) {
                    if (!spirit_name_input_buffer_.empty()) {
                        runtime_.WorldState().GetSpiritBeast().custom_name = spirit_name_input_buffer_;
                        runtime_.WorldState().GetInteraction().dialogue_text =
                            "灵兽已命名为：" + spirit_name_input_buffer_;
                        SetHintMessage(runtime_.WorldState(), "命名完成：" + spirit_name_input_buffer_, 2.4f);
                    }
                    spirit_name_input_active_ = false;
                    spirit_name_input_buffer_.clear();
                    continue;
                }
                if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    spirit_name_input_active_ = false;
                    spirit_name_input_buffer_.clear();
                    SetHintMessage(runtime_.WorldState(), "已取消灵兽命名。", 1.8f);
                    continue;
                }
                if (key->scancode == sf::Keyboard::Scancode::Backspace) {
                    if (!spirit_name_input_buffer_.empty()) {
                        spirit_name_input_buffer_.pop_back();
                    }
                    SetHintMessage(runtime_.WorldState(), "命名中：" + spirit_name_input_buffer_, 1.2f);
                    continue;
                }
            }
            if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
                if (text->unicode >= 32 && text->unicode < 127 && spirit_name_input_buffer_.size() < 12) {
                    spirit_name_input_buffer_.push_back(static_cast<char>(text->unicode));
                    SetHintMessage(runtime_.WorldState(), "命名中：" + spirit_name_input_buffer_, 1.2f);
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
                default: break;
                }
            }
            continue;
        }

        ForwardEventToHud_(event);

        input_handler_.HandleEvent(event);
        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            auto& interaction = runtime_.WorldState().GetInteraction();
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
            if (key->scancode == sf::Keyboard::Scancode::J) {
                (void)runtime_.TryEnterBattleByPlayerPosition();
            } else if (key->scancode == sf::Keyboard::Scancode::F2) {
                spirit_name_input_active_ = true;
                spirit_name_input_buffer_ = runtime_.WorldState().GetSpiritBeast().custom_name;
                SetHintMessage(runtime_.WorldState(), "输入灵兽名称（Enter确认 / Esc取消）", 2.4f);
            }
        }
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

    auto& world_state = runtime_.WorldState();
    const int current_day = world_state.GetClock().Day();
    pixel_hud_->Update(delta_seconds, &world_state.GetInteraction().dialogue_engine);
    pixel_hud_->UpdateTopRightInfo(
        "第" + std::to_string(world_state.GetClock().Day()) + "天 " + world_state.GetClock().TimeText(),
        world_state.GetClock().DateText(),
        runtime_.Systems().GetCloud().CurrentStateText(),
        false);
    pixel_hud_->SetBottomRightHotkeyHints(
        input_.GetPrimaryKeyName(Action::Interact),
        input_.GetPrimaryKeyName(Action::UseTool));
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
    pixel_hud_->GetMinimap().SetLocationText(
        world_state.GetInSpiritRealm() ? "灵界 · 浅层入口" : "云海农场 · 主屋前");

    CloudForecastViewData forecast_view;
    const auto& cloud_system = runtime_.Systems().GetCloud();
    forecast_view.today_prefix = runtime_.Config().GetString("forecast_today_prefix", forecast_view.today_prefix);
    forecast_view.tomorrow_prefix = runtime_.Config().GetString("forecast_tomorrow_prefix", forecast_view.tomorrow_prefix);
    forecast_view.bonus_format_prefix = runtime_.Config().GetString("forecast_bonus_prefix", forecast_view.bonus_format_prefix);
    forecast_view.bonus_midfix = runtime_.Config().GetString("forecast_bonus_midfix", forecast_view.bonus_midfix);
    forecast_view.tide_countdown_prefix = runtime_.Config().GetString("forecast_tide_countdown_prefix", forecast_view.tide_countdown_prefix);
    forecast_view.tide_countdown_suffix = runtime_.Config().GetString("forecast_tide_countdown_suffix", forecast_view.tide_countdown_suffix);
    forecast_view.recommendations_title = runtime_.Config().GetString("forecast_recommendations_title", forecast_view.recommendations_title);
    forecast_view.today_state_text = cloud_system.CurrentStateText();
    forecast_view.tomorrow_state_text = cloud_system.ForecastStateText();
    forecast_view.tide_countdown_days = cloud_system.TideCountdownDays(world_state.GetClock().Day());
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
    status_view.player_name = runtime_.Config().GetString("player_name", "云海旅人");
    status_view.header_level_separator = runtime_.Config().GetString("player_status_level_separator", status_view.header_level_separator);
    status_view.manor_stage_prefix = runtime_.Config().GetString("player_status_manor_stage_prefix", status_view.manor_stage_prefix);
    status_view.total_gold_prefix = runtime_.Config().GetString("player_status_total_gold_prefix", status_view.total_gold_prefix);
    status_view.stamina_label = runtime_.Config().GetString("player_status_stamina_label", status_view.stamina_label);
    status_view.spirit_label = runtime_.Config().GetString("player_status_spirit_label", status_view.spirit_label);
    status_view.fatigue_label = runtime_.Config().GetString("player_status_fatigue_label", status_view.fatigue_label);
    status_view.contract_progress_prefix = runtime_.Config().GetString("player_status_contract_progress_prefix", status_view.contract_progress_prefix);
    status_view.contract_total = std::max(1, static_cast<int>(runtime_.Config().GetFloat("player_status_contract_total", static_cast<float>(status_view.contract_total))));
    status_view.player_level = runtime_.Systems().GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
    status_view.manor_stage = world_state.GetMainHouseRepair().level;
    status_view.total_gold = world_state.GetGold();
    status_view.stamina_ratio = world_state.GetStamina().Ratio();
    status_view.spirit_ratio = std::min(1.0f, std::max(0.0f, static_cast<float>(runtime_.Systems().GetCloud().SpiritEnergy()) / 200.0f));
    status_view.fatigue_ratio = std::min(1.0f, std::max(0.0f, 1.0f - world_state.GetStamina().Ratio()));
    status_view.contract_progress = runtime_.Systems().GetContracts().CompletedVolumeCount();
    pixel_hud_->UpdatePlayerStatus(status_view);

    TeaGardenPanelViewData tea_view;
    tea_view.cloud_state_text = cloud_system.CurrentStateText();
    tea_view.spirit_bonus = cloud_system.SpiritEnergyGain();
    tea_view.quality_bonus_percent = std::max(0, static_cast<int>((runtime_.CloudMultiplier() - 1.0f) * 100.0f));
    tea_view.cloud_state_prefix = runtime_.Config().GetString("tea_garden_cloud_state_prefix", tea_view.cloud_state_prefix);
    tea_view.spirit_bonus_prefix = runtime_.Config().GetString("tea_garden_spirit_bonus_prefix", tea_view.spirit_bonus_prefix);
    tea_view.quality_bonus_prefix = runtime_.Config().GetString("tea_garden_quality_bonus_prefix", tea_view.quality_bonus_prefix);
    tea_view.cloud_preview_prefix = runtime_.Config().GetString("tea_garden_cloud_preview_prefix", tea_view.cloud_preview_prefix);
    tea_view.plots_title = runtime_.Config().GetString("tea_garden_plots_title", tea_view.plots_title);
    tea_view.quality_hint_text = runtime_.Config().GetString("tea_garden_quality_hint_text", tea_view.quality_hint_text);
    tea_view.actions_text = runtime_.Config().GetString("tea_garden_actions_text", tea_view.actions_text);
    const auto bonus_pct = [](CloudSeamanor::domain::CloudState s) {
        return static_cast<int>((CloudGrowthMultiplier(s) - 1.0f) * 100.0f);
    };
    tea_view.cloud_preview_text =
        "晴+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Clear)) + "%  "
        "雾+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Mist)) + "%  "
        "浓云+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::DenseCloud)) + "%  "
        "大潮+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Tide)) + "%";
    const auto& plots = world_state.GetTeaPlots();
    const std::size_t tea_count = std::min<std::size_t>(3, plots.size());
    for (std::size_t i = 0; i < tea_count; ++i) {
        const auto& plot = plots[i];
        TeaGardenPlotLineViewData line;
        line.name = plot.crop_name.empty() ? ("地块" + std::to_string(i + 1)) : plot.crop_name;
        line.progress_percent = static_cast<int>(std::max(0.0f, std::min(1.0f, plot.growth)) * 100.0f);
        switch (plot.quality) {
        case CloudSeamanor::domain::CropQuality::Normal: line.quality_text = "普通"; break;
        case CloudSeamanor::domain::CropQuality::Fine: line.quality_text = "优质"; break;
        case CloudSeamanor::domain::CropQuality::Rare: line.quality_text = "珍品"; break;
        case CloudSeamanor::domain::CropQuality::Spirit: line.quality_text = "灵品"; break;
        case CloudSeamanor::domain::CropQuality::Holy: line.quality_text = "圣品"; break;
        }
        tea_view.plots.push_back(std::move(line));
    }
    pixel_hud_->UpdateTeaGardenPanel(tea_view);

    FestivalPanelViewData festival_view;
    const auto& festivals = runtime_.Systems().GetFestivals();
    const auto& all_festivals = festivals.GetAllFestivals();
    festival_view.total_events = static_cast<int>(all_festivals.size());
    festival_view.completed_events = 0;
    for (const auto& fest : all_festivals) {
        if (fest.participated) {
            ++festival_view.completed_events;
        }
    }
    if (const auto* today = festivals.GetTodayFestival()) {
        festival_view.active_name = today->name;
        festival_view.countdown_days = 0;
    } else {
        const auto upcoming = festivals.GetUpcomingFestivals(1);
        if (!upcoming.empty()) {
            festival_view.active_name =
                runtime_.Config().GetString("festival_next_prefix", "下一节日: ")
                + upcoming.front()->name;
            festival_view.countdown_days = std::max(0, upcoming.front()->day - world_state.GetClock().DayInSeason());
        }
    }
    const auto upcoming_three = festivals.GetUpcomingFestivals(3);
    festival_view.upcoming.clear();
    for (const auto* fest : upcoming_three) {
        if (fest == nullptr) continue;
        festival_view.upcoming.push_back(fest->name + "(第" + std::to_string(fest->day) + "天)");
    }
    festival_view.reward_text = runtime_.Config().GetString(
        "festival_reward_text", festival_view.reward_text);
    festival_view.upcoming_prefix = runtime_.Config().GetString(
        "festival_upcoming_prefix", festival_view.upcoming_prefix);
    festival_view.upcoming_empty_text = runtime_.Config().GetString(
        "festival_upcoming_empty_text", festival_view.upcoming_empty_text);
    festival_view.participation_text = runtime_.Config().GetString(
        "festival_participation_text", festival_view.participation_text);
    festival_view.selected_participation_text = runtime_.Config().GetString(
        "festival_selected_participation_text", festival_view.selected_participation_text);
    festival_view.actions_text = runtime_.Config().GetString(
        "festival_actions_text", festival_view.actions_text);
    pixel_hud_->UpdateFestivalPanel(festival_view);

    ShopPanelViewData shop_view;
    shop_view.items_header_text = runtime_.Config().GetString("shop_items_header_text", shop_view.items_header_text);
    shop_view.player_gold_prefix = runtime_.Config().GetString("shop_player_gold_prefix", shop_view.player_gold_prefix);
    shop_view.selected_item_empty_text = runtime_.Config().GetString("shop_selected_empty_text", shop_view.selected_item_empty_text);
    shop_view.actions_text = runtime_.Config().GetString("shop_actions_text", shop_view.actions_text);
    shop_view.player_gold = world_state.GetGold();
    shop_view.items.clear();
    const auto& prices = world_state.GetPriceTable();
    const std::size_t shop_count = std::min<std::size_t>(3, prices.size());
    for (std::size_t i = 0; i < shop_count; ++i) {
        const auto& entry = prices[i];
        ShopPanelItemViewData item;
        item.name = ItemDisplayName(entry.item_id);
        item.price = std::max(0, entry.buy_price);
        item.stock = std::max(0, world_state.GetInventory().CountOf(entry.item_id));
        shop_view.items.push_back(std::move(item));
    }
    if (!shop_view.items.empty()) {
        const auto& first_item = shop_view.items.front();
        const bool affordable = shop_view.player_gold >= first_item.price;
        const std::string selected_prefix = runtime_.Config().GetString("shop_selected_prefix", "选中商品:");
        const std::string price_prefix = runtime_.Config().GetString("shop_price_prefix", "单价:");
        const std::string stock_prefix = runtime_.Config().GetString("shop_stock_prefix", "库存:");
        const std::string affordable_text = runtime_.Config().GetString("shop_affordable_text", "可购买");
        const std::string unaffordable_text = runtime_.Config().GetString("shop_unaffordable_text", "金币不足");
        shop_view.selected_item_desc =
            selected_prefix + " " + first_item.name
            + "  " + price_prefix + std::to_string(first_item.price)
            + "  " + stock_prefix + std::to_string(first_item.stock)
            + "  " + (affordable ? affordable_text : unaffordable_text);
    } else {
        shop_view.selected_item_desc = runtime_.Config().GetString("shop_empty_text", "暂无可购买商品");
    }
    pixel_hud_->UpdateShopPanel(shop_view);

    MailPanelViewData mail_view;
    mail_view.list_title_text = runtime_.Config().GetString(
        "mail_list_title_text", mail_view.list_title_text);
    mail_view.empty_detail_text = runtime_.Config().GetString(
        "mail_empty_detail_text", mail_view.empty_detail_text);
    mail_view.unread_prefix_text = runtime_.Config().GetString(
        "mail_unread_prefix_text", mail_view.unread_prefix_text);
    mail_view.unread_suffix_text = runtime_.Config().GetString(
        "mail_unread_suffix_text", mail_view.unread_suffix_text);
    mail_view.actions_text = runtime_.Config().GetString(
        "mail_actions_text", mail_view.actions_text);
    const auto& mail_orders = world_state.GetMailOrders();
    const std::size_t mail_count = std::min<std::size_t>(3, mail_orders.size());
    const std::string mail_sender_text = runtime_.Config().GetString("mail_sender_text", "商会");
    const std::string mail_time_prefix = runtime_.Config().GetString("mail_time_prefix", "第");
    const std::string mail_time_suffix = runtime_.Config().GetString("mail_time_suffix", "天送达");
    const std::string mail_detail_prefix = runtime_.Config().GetString("mail_detail_prefix", "详情: 附件");
    const std::string mail_detail_eta_prefix = runtime_.Config().GetString("mail_detail_eta_prefix", "预计");
    const std::string mail_detail_eta_suffix = runtime_.Config().GetString("mail_detail_eta_suffix", "天后送达");
    const std::string mail_detail_empty = runtime_.Config().GetString("mail_detail_empty_text", "详情: 当前没有待处理邮件");
    for (std::size_t i = 0; i < mail_count; ++i) {
        const auto& order = mail_orders[i];
        MailPanelEntryViewData entry;
        entry.sender = mail_sender_text;
        entry.subject = ItemDisplayName(order.item_id) + " x" + std::to_string(order.count);
        entry.time_text = mail_time_prefix + std::to_string(order.deliver_day) + mail_time_suffix;
        mail_view.entries.push_back(std::move(entry));
    }
    mail_view.unread_count = static_cast<int>(mail_orders.size());
    if (!mail_orders.empty()) {
        const auto& first = mail_orders.front();
        const int remaining_days = std::max(0, first.deliver_day - world_state.GetClock().Day());
        mail_view.detail_text =
            mail_detail_prefix + " " + ItemDisplayName(first.item_id) + " x" + std::to_string(first.count)
            + "  " + mail_detail_eta_prefix + std::to_string(remaining_days) + mail_detail_eta_suffix;
    } else {
        mail_view.detail_text = mail_detail_empty;
    }
    pixel_hud_->UpdateMailPanel(mail_view);

    AchievementPanelViewData achievement_view;
    const auto& achievements = world_state.GetAchievements();
    achievement_view.progress_prefix = runtime_.Config().GetString(
        "achievement_progress_prefix", achievement_view.progress_prefix);
    achievement_view.legend_text = runtime_.Config().GetString(
        "achievement_legend_text", achievement_view.legend_text);
    achievement_view.unlocked_mark = runtime_.Config().GetString(
        "achievement_unlocked_mark", achievement_view.unlocked_mark);
    achievement_view.locked_mark = runtime_.Config().GetString(
        "achievement_locked_mark", achievement_view.locked_mark);
    achievement_view.unlock_banner_text = runtime_.Config().GetString(
        "achievement_unlock_banner_text", achievement_view.unlock_banner_text);
    achievement_view.total_count = static_cast<int>(achievements.size());
    achievement_view.unlocked_count = 0;
    for (const auto& [id, unlocked] : achievements) {
        if (unlocked) {
            ++achievement_view.unlocked_count;
            achievement_view.unlocked_titles.push_back(AchievementDisplayName_(id));
        } else {
            achievement_view.locked_titles.push_back(AchievementDisplayName_(id));
        }
    }
    if (achievement_view.total_count == 0) {
        achievement_view.total_count = 1;
        achievement_view.locked_titles.push_back(
            runtime_.Config().GetString("achievement_empty_text", "待解锁成就"));
    }
    pixel_hud_->UpdateAchievementPanel(achievement_view);

    SpiritBeastPanelViewData beast_view;
    const auto& beast = world_state.GetSpiritBeast();
    beast_view.beast_name = beast.custom_name;
    beast_view.favor = beast.favor;
    beast_view.dispatched = beast.dispatched_for_pest_control;
    beast_view.trait = beast.trait;
    beast_view.category_text = runtime_.Config().GetString("spirit_beast_category_text", beast_view.category_text);
    beast_view.cards_title = runtime_.Config().GetString("spirit_beast_cards_title", beast_view.cards_title);
    beast_view.influence_hint_text = runtime_.Config().GetString("spirit_beast_influence_hint_text", beast_view.influence_hint_text);
    beast_view.recruit_hint_text = runtime_.Config().GetString("spirit_beast_recruit_hint_text", beast_view.recruit_hint_text);
    beast_view.actions_text = runtime_.Config().GetString("spirit_beast_actions_text", beast_view.actions_text);
    beast_view.dispatch_in_progress_text = runtime_.Config().GetString("spirit_beast_dispatch_in_progress_text", beast_view.dispatch_in_progress_text);
    beast_view.dispatch_idle_text = runtime_.Config().GetString("spirit_beast_dispatch_idle_text", beast_view.dispatch_idle_text);
    beast_view.state_text = runtime_.Config().GetString("spirit_beast_state_idle_text", "待机");
    switch (beast.state) {
    case SpiritBeastState::Idle: beast_view.state_text = runtime_.Config().GetString("spirit_beast_state_idle_text", "待机"); break;
    case SpiritBeastState::Wander: beast_view.state_text = runtime_.Config().GetString("spirit_beast_state_wander_text", "巡游"); break;
    case SpiritBeastState::Follow: beast_view.state_text = runtime_.Config().GetString("spirit_beast_state_follow_text", "跟随"); break;
    case SpiritBeastState::Interact: beast_view.state_text = runtime_.Config().GetString("spirit_beast_state_interact_text", "互动"); break;
    }
    pixel_hud_->UpdateSpiritBeastPanel(beast_view);

    BuildingPanelViewData building_view;
    building_view.player_gold = world_state.GetGold();
    building_view.main_house_level = world_state.GetMainHouseRepair().level;
    building_view.greenhouse_unlocked = world_state.GetGreenhouseUnlocked();
    building_view.workshop_level = runtime_.Systems().GetWorkshop().Level();
    building_view.category_prefix = runtime_.Config().GetString(
        "building_category_prefix", building_view.category_prefix);
    building_view.list_title = runtime_.Config().GetString(
        "building_list_title", building_view.list_title);
    building_view.upgrade_requirement_text = runtime_.Config().GetString(
        "building_upgrade_requirement_text", building_view.upgrade_requirement_text);
    building_view.preview_text = runtime_.Config().GetString(
        "building_preview_text", building_view.preview_text);
    building_view.actions_text = runtime_.Config().GetString(
        "building_actions_text", building_view.actions_text);
    building_view.building_lines = {
        runtime_.Config().GetString(
            "building_line_main_house",
            "主屋 Lv" + std::to_string(building_view.main_house_level) + "   状态: 可升级   效果: 解锁更多系统"),
        runtime_.Config().GetString(
            "building_line_greenhouse",
            "温室 " + std::string(building_view.greenhouse_unlocked ? "已解锁" : "未解锁")
                + "   状态: " + (building_view.greenhouse_unlocked ? "可使用" : "需主屋升级")),
        runtime_.Config().GetString(
            "building_line_workshop",
            "工坊 Lv" + std::to_string(building_view.workshop_level) + "   状态: 可升级   效果: 队列 +1"),
    };
    pixel_hud_->UpdateBuildingPanel(building_view);

    ContractPanelViewData contract_view;
    const auto& contracts = runtime_.Systems().GetContracts();
    contract_view.volumes_line_prefix = runtime_.Config().GetString("contract_volumes_line_prefix", contract_view.volumes_line_prefix);
    contract_view.tracking_line_prefix = runtime_.Config().GetString("contract_tracking_line_prefix", contract_view.tracking_line_prefix);
    contract_view.tracking_name_separator = runtime_.Config().GetString("contract_tracking_name_separator", contract_view.tracking_name_separator);
    contract_view.bonus_prefix = runtime_.Config().GetString("contract_bonus_prefix", contract_view.bonus_prefix);
    contract_view.tasks_title = runtime_.Config().GetString("contract_tasks_title", contract_view.tasks_title);
    contract_view.chapter_reward_hint_text = runtime_.Config().GetString("contract_chapter_reward_hint_text", contract_view.chapter_reward_hint_text);
    contract_view.completed_volumes = contracts.CompletedVolumeCount();
    contract_view.total_volumes = static_cast<int>(contracts.Volumes().size());
    if (const auto* tracking = contracts.GetTrackingVolume()) {
        contract_view.tracking_volume_id = tracking->volume_id;
        contract_view.tracking_volume_name = tracking->name;
        contract_view.tracking_bonus = tracking->permanent_bonus_description;
        contract_view.task_lines.clear();
        const std::size_t count = std::min<std::size_t>(3, tracking->items.size());
        for (std::size_t i = 0; i < count; ++i) {
            const auto& item = tracking->items[i];
            const std::string prefix = item.completed ? "✓ " : "○ ";
            contract_view.task_lines.push_back(prefix + item.name + "  需求: " + std::to_string(item.required_count));
        }
    }
    pixel_hud_->UpdateContractPanel(contract_view);

    NpcDetailPanelViewData npc_view;
    const auto& npcs = world_state.GetNpcs();
    const int highlighted_npc = world_state.GetInteraction().highlighted_npc_index;
    const NpcActor* selected_npc = nullptr;
    if (highlighted_npc >= 0 && highlighted_npc < static_cast<int>(npcs.size())) {
        selected_npc = &npcs[static_cast<std::size_t>(highlighted_npc)];
    } else if (!npcs.empty()) {
        selected_npc = &npcs.front();
    }
    if (selected_npc != nullptr) {
        npc_view.name = selected_npc->display_name;
        npc_view.heart_level = selected_npc->heart_level;
        npc_view.favor = selected_npc->favor;
        npc_view.talked_today = selected_npc->daily_talked;
        npc_view.gifted_today = selected_npc->daily_gifted;
        npc_view.location = selected_npc->current_location.empty() ? "云海山庄" : selected_npc->current_location;
    }
    npc_view.title_suffix = runtime_.Config().GetString("npc_detail_title_suffix", npc_view.title_suffix);
    npc_view.location_prefix = runtime_.Config().GetString("npc_detail_location_prefix", npc_view.location_prefix);
    npc_view.favor_prefix = runtime_.Config().GetString("npc_detail_favor_prefix", npc_view.favor_prefix);
    npc_view.heart_event_text = runtime_.Config().GetString("npc_detail_heart_event_text", npc_view.heart_event_text);
    npc_view.talked_done_text = runtime_.Config().GetString("npc_detail_talked_done_text", npc_view.talked_done_text);
    npc_view.talked_todo_text = runtime_.Config().GetString("npc_detail_talked_todo_text", npc_view.talked_todo_text);
    npc_view.gifted_done_text = runtime_.Config().GetString("npc_detail_gifted_done_text", npc_view.gifted_done_text);
    npc_view.gifted_todo_text = runtime_.Config().GetString("npc_detail_gifted_todo_text", npc_view.gifted_todo_text);
    npc_view.event_hint_text = runtime_.Config().GetString("npc_detail_event_hint_text", npc_view.event_hint_text);
    npc_view.actions_text = runtime_.Config().GetString("npc_detail_actions_text", npc_view.actions_text);
    pixel_hud_->UpdateNpcDetailPanel(npc_view);

    SpiritRealmPanelViewData spirit_realm_view;
    spirit_realm_view.mode_text = runtime_.Config().GetString("spirit_realm_mode_text", "轻松");
    spirit_realm_view.max_count = world_state.GetSpiritRealmDailyMax();
    spirit_realm_view.in_spirit_realm = world_state.GetInSpiritRealm();
    spirit_realm_view.remaining_count = world_state.GetSpiritRealmDailyRemaining();
    spirit_realm_view.drop_bonus_percent = std::max(0, static_cast<int>((runtime_.CloudMultiplier() - 1.0f) * 100.0f));
    spirit_realm_view.mode_line_prefix = runtime_.Config().GetString(
        "spirit_realm_mode_prefix", spirit_realm_view.mode_line_prefix);
    spirit_realm_view.mode_options_text = runtime_.Config().GetString(
        "spirit_realm_mode_options_text", spirit_realm_view.mode_options_text);
    spirit_realm_view.remaining_line_prefix = runtime_.Config().GetString(
        "spirit_realm_remaining_prefix", spirit_realm_view.remaining_line_prefix);
    spirit_realm_view.drop_bonus_prefix = runtime_.Config().GetString(
        "spirit_realm_drop_bonus_prefix", spirit_realm_view.drop_bonus_prefix);
    spirit_realm_view.regions_title = runtime_.Config().GetString(
        "spirit_realm_regions_title", spirit_realm_view.regions_title);
    spirit_realm_view.default_region_line_1 = runtime_.Config().GetString(
        "spirit_realm_default_region_line_1", spirit_realm_view.default_region_line_1);
    spirit_realm_view.default_region_line_2 = runtime_.Config().GetString(
        "spirit_realm_default_region_line_2", spirit_realm_view.default_region_line_2);
    spirit_realm_view.default_region_line_3 = runtime_.Config().GetString(
        "spirit_realm_default_region_line_3", spirit_realm_view.default_region_line_3);
    spirit_realm_view.active_state_in_realm_text = runtime_.Config().GetString(
        "spirit_realm_active_state_in_realm_text", spirit_realm_view.active_state_in_realm_text);
    spirit_realm_view.active_state_unlocked_text = runtime_.Config().GetString(
        "spirit_realm_active_state_unlocked_text", spirit_realm_view.active_state_unlocked_text);
    spirit_realm_view.active_state_suffix = runtime_.Config().GetString(
        "spirit_realm_active_state_suffix", spirit_realm_view.active_state_suffix);
    spirit_realm_view.lock_hint_text = runtime_.Config().GetString(
        "spirit_realm_lock_hint_text", spirit_realm_view.lock_hint_text);
    spirit_realm_view.actions_text = runtime_.Config().GetString(
        "spirit_realm_actions_text", spirit_realm_view.actions_text);
    spirit_realm_view.region_lines = {
        runtime_.Config().GetString("spirit_realm_region_line_1", "浅层云径  Lv8   掉落: 灵尘/雾草"),
        runtime_.Config().GetString("spirit_realm_region_line_2", "潮汐裂谷  Lv15  状态: 🔒 需契约卷3"),
        runtime_.Config().GetString("spirit_realm_region_line_3", "霜岚祭坛  Lv22  状态: 🔒 需山庄等级5"),
    };
    pixel_hud_->UpdateSpiritRealmPanel(spirit_realm_view);

    BeastiaryPanelViewData beastiary_view;
    beastiary_view.filter_text = runtime_.Config().GetString(
        "beastiary_filter_text", beastiary_view.filter_text);
    beastiary_view.progress_prefix = runtime_.Config().GetString(
        "beastiary_progress_prefix", beastiary_view.progress_prefix);
    const std::array<std::string, 3> tracked_pet_types{"cat", "dog", "bird"};
    const auto& achievements_map = world_state.GetAchievements();
    beastiary_view.total_count = static_cast<int>(tracked_pet_types.size());
    beastiary_view.discovered_count = 0;
    for (const auto& type : tracked_pet_types) {
        const auto it = achievements_map.find("pet_collected_" + type);
        const bool discovered = (it != achievements_map.end() && it->second);
        if (discovered) {
            ++beastiary_view.discovered_count;
            beastiary_view.discovered_lines.push_back(
                PetDisplayName_(type) + "      已发现   区域: 云海山庄");
        } else {
            beastiary_view.undiscovered_lines.push_back(
                "???         未发现   条件: 解锁" + PetDisplayName_(type) + "认养");
        }
    }
    if (world_state.GetPetAdopted()) {
        beastiary_view.selected_detail =
            "选中详情: 已结缘灵兽 " + PetDisplayName_(world_state.GetPetType());
    } else {
        beastiary_view.selected_detail = "选中详情: 尚未结缘灵兽";
    }
    pixel_hud_->UpdateBeastiaryPanel(beastiary_view);

    WorkshopPanelViewData workshop_view;
    const auto& workshop = runtime_.Systems().GetWorkshop();
    const auto& machine = world_state.GetTeaMachine();
    workshop_view.auto_craft = machine.running;
    workshop_view.workshop_level = workshop.Level();
    workshop_view.unlocked_slots = workshop.UnlockedSlots();
    workshop_view.tabs_text = runtime_.Config().GetString("workshop_tabs_text", workshop_view.tabs_text);
    workshop_view.queue_title_text = runtime_.Config().GetString("workshop_queue_title_text", workshop_view.queue_title_text);
    workshop_view.queue_primary_prefix = runtime_.Config().GetString("workshop_queue_primary_prefix", workshop_view.queue_primary_prefix);
    workshop_view.queue_progress_suffix = runtime_.Config().GetString("workshop_queue_progress_suffix", workshop_view.queue_progress_suffix);
    workshop_view.empty_slot_line_2 = runtime_.Config().GetString("workshop_empty_slot_line_2", workshop_view.empty_slot_line_2);
    workshop_view.empty_slot_line_3 = runtime_.Config().GetString("workshop_empty_slot_line_3", workshop_view.empty_slot_line_3);
    workshop_view.stock_prefix = runtime_.Config().GetString("workshop_stock_prefix", workshop_view.stock_prefix);
    workshop_view.stock_tea_label = runtime_.Config().GetString("workshop_stock_tea_label", workshop_view.stock_tea_label);
    workshop_view.stock_wood_label = runtime_.Config().GetString("workshop_stock_wood_label", workshop_view.stock_wood_label);
    workshop_view.stock_crystal_label = runtime_.Config().GetString("workshop_stock_crystal_label", workshop_view.stock_crystal_label);
    workshop_view.actions_text = runtime_.Config().GetString("workshop_actions_text", workshop_view.actions_text);
    workshop_view.active_recipe = runtime_.Config().GetString("workshop_idle_recipe_text", "待命");
    const auto& machines = workshop.GetMachines();
    workshop_view.queue_lines.clear();
    const std::size_t line_count = std::min<std::size_t>(3, machines.size());
    for (std::size_t i = 0; i < line_count; ++i) {
        const auto& m = machines[i];
        std::string recipe_name = runtime_.Config().GetString("workshop_idle_recipe_text", "待命");
        if (!m.recipe_id.empty()) {
            if (const auto* recipe = workshop.GetRecipe(m.recipe_id)) {
                recipe_name = recipe->name;
            } else {
                recipe_name = m.recipe_id;
            }
        }
        const int machine_progress_pct = static_cast<int>(std::max(0.0f, std::min(100.0f, m.progress)));
        workshop_view.queue_lines.push_back(
            std::to_string(i + 1) + ") " + recipe_name + "      " + std::to_string(machine_progress_pct) + "%  "
            + (m.is_processing ? "加工中" : "空闲"));
    }
    while (workshop_view.queue_lines.size() < 3) {
        workshop_view.queue_lines.push_back(
            std::to_string(workshop_view.queue_lines.size() + 1) + ") 空槽位");
    }
    if (!machines.empty()) {
        const auto& first_machine = machines.front();
        if (!first_machine.recipe_id.empty()) {
            if (const auto* recipe = workshop.GetRecipe(first_machine.recipe_id)) {
                workshop_view.active_recipe = recipe->name;
            } else {
                workshop_view.active_recipe = first_machine.recipe_id;
            }
        }
    }
    workshop_view.queue_progress = machine.progress;
    workshop_view.queued_output = machine.queued_output;
    workshop_view.tea_leaf_stock = world_state.GetInventory().CountOf("TeaLeaf");
    workshop_view.wood_stock = world_state.GetInventory().CountOf("Wood");
    workshop_view.crystal_stock = world_state.GetInventory().CountOf("SpiritDust");
    pixel_hud_->UpdateWorkshopPanel(workshop_view);

    // UI-030: 节日期间自动显示节日面板（每天首次触发一次）。
    if (const auto* today_festival = runtime_.Systems().GetFestivals().GetTodayFestival()) {
        (void)today_festival;
        if (current_day != last_festival_auto_popup_day_) {
            if (!pixel_hud_->IsDialogueOpen() && !pixel_hud_->IsAnyPanelOpen()) {
                pixel_hud_->ToggleFestival();
                last_festival_auto_popup_day_ = current_day;
            }
        }
    }
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
