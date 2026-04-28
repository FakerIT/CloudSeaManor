cmake_minimum_required(VERSION 3.23)
project(CloudSeamanor_game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(CLOUDSEAMANOR_ENABLE_WARNINGS "Enable strict warnings" ON)
option(CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE "Temporarily allow deprecated AllDefine.hpp umbrella header" OFF)

set(_sfml_candidate_roots
    "${CMAKE_CURRENT_SOURCE_DIR}/../../03_参考与第三方资源/01_SFML引擎/SFML-3.0.2"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../05_参考与第三方资源/01_SFML引擎/SFML-3.0.2"
)

if(DEFINED ENV{SFML_ROOT} AND EXISTS "$ENV{SFML_ROOT}")
    set(SFML_ROOT "$ENV{SFML_ROOT}")
else()
    foreach(_candidate IN LISTS _sfml_candidate_roots)
        if(EXISTS "${_candidate}")
            set(SFML_ROOT "${_candidate}")
            break()
        endif()
    endforeach()
endif()

if(NOT SFML_ROOT)
    message(FATAL_ERROR
        "SFML not found. Set SFML_ROOT (cache or env), or place SFML-3.0.2 at one of: "
        "03_参考与第三方资源/01_SFML引擎/SFML-3.0.2 "
        "or 05_参考与第三方资源/01_SFML引擎/SFML-3.0.2"
    )
endif()
message(STATUS "Using SFML_ROOT: ${SFML_ROOT}")
list(PREPEND CMAKE_PREFIX_PATH "${SFML_ROOT}")
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

# ============================================================================
# 公开头文件列表
# ============================================================================
# 领域层头文件
set(CLOUDSEAMANOR_DOMAIN_HEADERS
  include/CloudSeamanor/MathTypes.hpp
  include/CloudSeamanor/CloudSystem.hpp
  include/CloudSeamanor/CloudGuardianContract.hpp
  include/CloudSeamanor/CropData.hpp
  include/CloudSeamanor/DynamicLifeSystem.hpp
  include/CloudSeamanor/DiarySystem.hpp
  include/CloudSeamanor/FestivalSystem.hpp
  include/CloudSeamanor/FestivalGameplayMvp.hpp
  include/CloudSeamanor/GameClock.hpp
  include/CloudSeamanor/HungerTable.hpp
  include/CloudSeamanor/HungerSystem.hpp
  include/CloudSeamanor/Interactable.hpp
  include/CloudSeamanor/Inventory.hpp
  include/CloudSeamanor/MainPlotSystem.hpp
  include/CloudSeamanor/PickupDrop.hpp
  include/CloudSeamanor/Player.hpp
  include/CloudSeamanor/ProfileHelper.hpp
  include/CloudSeamanor/RecipeData.hpp
  include/CloudSeamanor/SkillSystem.hpp
  include/CloudSeamanor/Stamina.hpp
  include/CloudSeamanor/TeaProcessingRecipe.hpp
  include/CloudSeamanor/BuffSystem.hpp
  include/CloudSeamanor/TeaGarden.hpp
  include/CloudSeamanor/TeaBush.hpp
  include/CloudSeamanor/TeaBushTable.hpp
  include/CloudSeamanor/TeaTable.hpp
  include/CloudSeamanor/TeaQualityCalculator.hpp
  include/CloudSeamanor/BuffEffectTable.hpp
  include/CloudSeamanor/WorkshopSystem.hpp
  include/CloudSeamanor/AtmosphereState.hpp
  include/CloudSeamanor/RelationshipSystem.hpp
  include/CloudSeamanor/SeedDropTable.hpp
)

# 引擎层/应用层头文件
set(CLOUDSEAMANOR_ENGINE_HEADERS
  include/CloudSeamanor/CloudSeaManor.hpp
  include/CloudSeamanor/FarmingSystem.hpp
  include/CloudSeamanor/GameApp.hpp
  include/CloudSeamanor/AudioManager.hpp
  include/CloudSeamanor/GameAppFarming.hpp
  include/CloudSeamanor/GameAppHud.hpp
  include/CloudSeamanor/GameAppNpc.hpp
  include/CloudSeamanor/GameAppRuntimeTypes.hpp
  include/CloudSeamanor/GameAppSave.hpp
  include/CloudSeamanor/GameAppScene.hpp
  include/CloudSeamanor/GameAppSpiritBeast.hpp
  include/CloudSeamanor/GameAppText.hpp
  include/CloudSeamanor/GameAppWeather.hpp
  include/CloudSeamanor/GameAppState.hpp
  include/CloudSeamanor/game_state/PlayerState.hpp
  include/CloudSeamanor/game_state/RenderState.hpp
  include/CloudSeamanor/GameRuntime.hpp
  include/CloudSeamanor/GameWorldState.hpp
  include/CloudSeamanor/GameWorldSystems.hpp
  include/CloudSeamanor/InteractionSystem.hpp
  include/CloudSeamanor/LevelUpSystem.hpp
  include/CloudSeamanor/PickupSystem.hpp
  include/CloudSeamanor/PlayerInteractRuntime.hpp
  include/CloudSeamanor/TargetHintRuntime.hpp
  include/CloudSeamanor/TextRenderUtils.hpp
  include/CloudSeamanor/InputManager.hpp
  include/CloudSeamanor/UISystem.hpp
  include/CloudSeamanor/DialogueEngine.hpp
  include/CloudSeamanor/EventBus.hpp
  include/CloudSeamanor/FestivalRuntimeData.hpp
  include/CloudSeamanor/PixelUiConfig.hpp
  include/CloudSeamanor/PixelArtStyle.hpp
  include/CloudSeamanor/PixelFontRenderer.hpp
  include/CloudSeamanor/PixelUiPanel.hpp
  include/CloudSeamanor/PixelProgressBar.hpp
  include/CloudSeamanor/PixelToolbar.hpp
  include/CloudSeamanor/PixelDialogueBox.hpp
  include/CloudSeamanor/PixelInventoryGrid.hpp
  include/CloudSeamanor/PixelQuestMenu.hpp
  include/CloudSeamanor/PixelMinimap.hpp
  include/CloudSeamanor/PixelCloudSeaForecastPanel.hpp
  include/CloudSeamanor/PixelPlayerStatusPanel.hpp
  include/CloudSeamanor/PixelDailyRecommendationPanel.hpp
  include/CloudSeamanor/PixelTutorialBubble.hpp
  include/CloudSeamanor/PixelTeaGardenPanel.hpp
  include/CloudSeamanor/PixelWorkshopPanel.hpp
  include/CloudSeamanor/PixelContractPanel.hpp
  include/CloudSeamanor/PixelNpcDetailPanel.hpp
  include/CloudSeamanor/PixelSpiritBeastPanel.hpp
  include/CloudSeamanor/PixelFestivalPanel.hpp
  include/CloudSeamanor/PixelSpiritRealmPanel.hpp
  include/CloudSeamanor/PixelBuildingPanel.hpp
  include/CloudSeamanor/PixelShopPanel.hpp
  include/CloudSeamanor/PixelMailPanel.hpp
  include/CloudSeamanor/PixelAchievementPanel.hpp
  include/CloudSeamanor/PixelBeastiaryPanel.hpp
  include/CloudSeamanor/LoadingScreen.hpp
  include/CloudSeamanor/PixelContextMenu.hpp
  include/CloudSeamanor/PixelNotificationBanner.hpp
  include/CloudSeamanor/PixelTooltip.hpp
  include/CloudSeamanor/PixelGameHud.hpp
  include/CloudSeamanor/PixelSettingsPanel.hpp
  include/CloudSeamanor/engine/input/InputHandler.hpp
  include/CloudSeamanor/engine/presentation/HudPresenter.hpp
  include/CloudSeamanor/engine/presentation/HudPanelPresenters.hpp
  include/CloudSeamanor/engine/presentation/HudSideEffects.hpp
  include/CloudSeamanor/engine/rendering/HudRenderer.hpp
  include/CloudSeamanor/engine/rendering/SceneVisualStore.hpp
  include/CloudSeamanor/engine/rendering/WorldRenderer.hpp
  include/CloudSeamanor/engine/rendering/PlayerVisualComponent.hpp
  include/CloudSeamanor/engine/rendering/UiPanelInitializer.hpp
  include/CloudSeamanor/engine/rendering/AtmosphereRenderer.hpp
  include/CloudSeamanor/engine/BattleEntities.hpp
  include/CloudSeamanor/engine/BattleField.hpp
  include/CloudSeamanor/engine/BattleUI.hpp
  include/CloudSeamanor/engine/BattleManager.hpp
  include/CloudSeamanor/engine/BattleRenderer.hpp
  include/CloudSeamanor/engine/BattleZoneLoader.hpp
  include/CloudSeamanor/engine/systems/PlayerMovementSystem.hpp
  include/CloudSeamanor/engine/systems/PickupSystemRuntime.hpp
  include/CloudSeamanor/engine/systems/CropGrowthSystem.hpp
  include/CloudSeamanor/engine/systems/AchievementSystem.hpp
  include/CloudSeamanor/engine/systems/DecorationSystem.hpp
  include/CloudSeamanor/engine/systems/NpcScheduleSystem.hpp
  include/CloudSeamanor/engine/systems/QuestManager.hpp
  include/CloudSeamanor/engine/systems/ShopSystem.hpp
  include/CloudSeamanor/engine/systems/NpcDeliverySystem.hpp
  include/CloudSeamanor/engine/systems/InnSystem.hpp
  include/CloudSeamanor/engine/systems/CoopSystem.hpp
  include/CloudSeamanor/engine/systems/PetSystem.hpp
  include/CloudSeamanor/engine/systems/SpiritBeastSystem.hpp
  include/CloudSeamanor/engine/systems/SpiritRealmManager.hpp
  include/CloudSeamanor/engine/systems/TutorialSystem.hpp
  include/CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp
  include/CloudSeamanor/NpcDialogueManager.hpp
)

# 基础设施层头文件
set(CLOUDSEAMANOR_INFRA_HEADERS
  include/CloudSeamanor/GameConfig.hpp
  include/CloudSeamanor/GameConstants.hpp
  include/CloudSeamanor/JsonValue.hpp
  include/CloudSeamanor/Logger.hpp
  include/CloudSeamanor/DataRegistry.hpp
  include/CloudSeamanor/Result.hpp
  include/CloudSeamanor/ResourceManager.hpp
  include/CloudSeamanor/UiLayoutConfig.hpp
  include/CloudSeamanor/TmxMap.hpp
  include/CloudSeamanor/SaveSlotManager.hpp
  include/CloudSeamanor/infrastructure/DialogueJsonParser.hpp
  include/CloudSeamanor/infrastructure/data/BattleDataLoader.hpp
  include/CloudSeamanor/infrastructure/AtmosphereDataLoader.hpp
  include/CloudSeamanor/infrastructure/ReferenceSaveDataSystem.hpp
)

set(CLOUDSEAMANOR_PUBLIC_HEADERS
  ${CLOUDSEAMANOR_DOMAIN_HEADERS}
  ${CLOUDSEAMANOR_ENGINE_HEADERS}
  ${CLOUDSEAMANOR_INFRA_HEADERS}
)

# ============================================================================
# 领域层源文件
# ============================================================================
set(CLOUDSEAMANOR_DOMAIN_SOURCES
  src/domain/CloudSystem.cpp
  src/domain/CloudGuardianContract.cpp
  src/domain/CropData.cpp
  src/domain/DynamicLifeSystem.cpp
  src/domain/DiarySystem.cpp
  src/domain/FestivalSystem.cpp
  src/domain/FestivalGameplayMvp.cpp
  src/domain/GameClock.cpp
  src/domain/HungerTable.cpp
  src/domain/HungerSystem.cpp
  src/domain/Inventory.cpp
  src/domain/MainPlotSystem.cpp
  src/domain/Player.cpp
  src/domain/RecipeData.cpp
  src/domain/SkillSystem.cpp
  src/domain/Stamina.cpp
  src/domain/ToolSystem.cpp
  src/domain/TeaTable.cpp
  src/domain/TeaBushTable.cpp
  src/domain/TeaQualityCalculator.cpp
  src/domain/BuffSystem.cpp
  src/domain/BuffEffectTable.cpp
  src/domain/TeaGarden.cpp
  src/domain/TeaBush.cpp
  src/domain/WorkshopSystem.cpp
  src/domain/AtmosphereState.cpp
  src/domain/RelationshipSystem.cpp
  src/domain/SeedDropTable.cpp
)

# ============================================================================
# 引擎层源文件
# ============================================================================
set(CLOUDSEAMANOR_ENGINE_SOURCES
  src/engine/FarmingSystem.cpp
  src/engine/GameApp.cpp
  src/engine/GameAppMenu.cpp
  src/engine/MainMenuController.cpp
  src/engine/SceneManager.cpp
  src/engine/GameAppFarming.cpp
  src/engine/GameAppHud.cpp
  src/engine/GameAppInput.cpp
  src/engine/GameAppNpc.cpp
  src/engine/GameAppSave.cpp
  src/engine/GameAppScene.cpp
  src/engine/GameAppSpiritBeast.cpp
  src/engine/GameAppText.cpp
  src/engine/GameAppWeather.cpp
  src/engine/GameRuntime.cpp
  src/engine/GameWorldState.cpp
  src/engine/FestivalRuntimeData.cpp
  src/engine/GameWorldSystems.cpp
  src/engine/InteractionSystem.cpp
  src/engine/Interactable.cpp
  src/engine/LevelUpSystem.cpp
  src/engine/PickupSystem.cpp
  src/engine/PickupDrop.cpp
  src/engine/PlayerInteractRuntime.cpp
  src/engine/TargetHintRuntime.cpp
  src/engine/TextRenderUtils.cpp
  src/engine/InputManager.cpp
  src/engine/UISystem.cpp
  src/engine/DialogueEngine.cpp
  src/engine/EventBus.cpp
  src/engine/UiLayoutSerializer.cpp
  src/engine/systems/PlayerMovementSystem.cpp
  src/engine/systems/PickupSystemRuntime.cpp
  src/engine/systems/CropGrowthSystem.cpp
  src/engine/systems/AchievementSystem.cpp
  src/engine/systems/DecorationSystem.cpp
  src/engine/systems/NpcScheduleSystem.cpp
  src/engine/systems/QuestManager.cpp
  src/engine/systems/ShopSystem.cpp
  src/engine/systems/NpcDeliverySystem.cpp
  src/engine/systems/InnSystem.cpp
  src/engine/systems/CoopSystem.cpp
  src/engine/systems/PetSystem.cpp
  src/engine/systems/SpiritBeastSystem.cpp
  src/engine/systems/SpiritRealmManager.cpp
  src/engine/systems/TutorialSystem.cpp
  src/engine/systems/WorkshopSystemRuntime.cpp
  src/engine/NpcDialogueManager.cpp
  src/engine/PixelArtStyle.cpp
  src/engine/PixelFontRenderer.cpp
  src/engine/PixelUiPanel.cpp
  src/engine/PixelProgressBar.cpp
  src/engine/PixelToolbar.cpp
  src/engine/PixelDialogueBox.cpp
  src/engine/PixelInventoryGrid.cpp
  src/engine/PixelQuestMenu.cpp
  src/engine/PixelMinimap.cpp
  src/engine/PixelCloudSeaForecastPanel.cpp
  src/engine/PixelPlayerStatusPanel.cpp
  src/engine/PixelDailyRecommendationPanel.cpp
  src/engine/PixelTutorialBubble.cpp
  src/engine/PixelTeaGardenPanel.cpp
  src/engine/PixelWorkshopPanel.cpp
  src/engine/PixelContractPanel.cpp
  src/engine/PixelNpcDetailPanel.cpp
  src/engine/PixelSpiritBeastPanel.cpp
  src/engine/PixelFestivalPanel.cpp
  src/engine/PixelSpiritRealmPanel.cpp
  src/engine/PixelBuildingPanel.cpp
  src/engine/PixelShopPanel.cpp
  src/engine/PixelMailPanel.cpp
  src/engine/PixelAchievementPanel.cpp
  src/engine/PixelBeastiaryPanel.cpp
  src/engine/LoadingScreen.cpp
  src/engine/PixelContextMenu.cpp
  src/engine/PixelNotificationBanner.cpp
  src/engine/PixelTooltip.cpp
  src/engine/PixelGameHud.cpp
  src/engine/PixelSettingsPanel.cpp
  src/engine/presentation/HudPresenter.cpp
  src/engine/presentation/HudPanelPresenters.cpp
  src/engine/presentation/HudSideEffects.cpp
  src/engine/input/InputHandler.cpp
  src/engine/rendering/HudRenderer.cpp
  src/engine/rendering/SceneVisualStore.cpp
  src/engine/rendering/FestivalDecorationSystem.cpp
  src/engine/rendering/WorldRenderer.cpp
  src/engine/rendering/PlayerVisualComponent.cpp
  src/engine/rendering/UiPanelInitializer.cpp
  src/engine/AtmosphereRenderer.cpp
  src/engine/AudioManager.cpp
  src/engine/BattleField.cpp
  src/engine/BattleUI.cpp
  src/engine/BattleManager.cpp
  src/engine/BattleRenderer.cpp
  src/engine/BattleZoneLoader.cpp
)

# ============================================================================
# 基础设施层源文件
# ============================================================================
set(CLOUDSEAMANOR_INFRASTRUCTURE_SOURCES
  src/infrastructure/GameConfig.cpp
  src/infrastructure/JsonValue.cpp
  src/infrastructure/Logger.cpp
  src/infrastructure/DataRegistry.cpp
  src/infrastructure/ResourceManager.cpp
  src/infrastructure/UiLayoutConfig.cpp
  src/infrastructure/TmxMap.cpp
  src/infrastructure/SaveSlotManager.cpp
  src/infrastructure/AtmosphereDataLoader.cpp
  src/infrastructure/DialogueJsonParser.cpp
  src/infrastructure/ReferenceSaveDataSystem.cpp
  src/infrastructure/ModLoader.cpp
  src/infrastructure/data/BattleDataLoader.cpp
)

# ============================================================================
# 应用入口源文件
# ============================================================================
set(CLOUDSEAMANOR_APP_SOURCES
  src/app/main.cpp
)

# ============================================================================
# 核心库
# ============================================================================
set(CLOUDSEAMANOR_CORE_SOURCES
  ${CLOUDSEAMANOR_DOMAIN_SOURCES}
  ${CLOUDSEAMANOR_ENGINE_SOURCES}
  ${CLOUDSEAMANOR_INFRASTRUCTURE_SOURCES}
)

add_library(CloudSeamanor_core
  ${CLOUDSEAMANOR_CORE_SOURCES}
  ${CLOUDSEAMANOR_PUBLIC_HEADERS}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${CLOUDSEAMANOR_PUBLIC_HEADERS}
  ${CLOUDSEAMANOR_DOMAIN_SOURCES}
  ${CLOUDSEAMANOR_ENGINE_SOURCES}
  ${CLOUDSEAMANOR_INFRASTRUCTURE_SOURCES}
)

target_include_directories(CloudSeamanor_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CloudSeamanor_core
  PUBLIC
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

if(CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE)
  target_compile_definitions(CloudSeamanor_core PUBLIC CLOUDSEAMANOR_ENABLE_LEGACY_ALLDEFINE=1)
endif()

if(MSVC)
  # /FS: allow multiple cl.exe write to the same PDB (fixes sporadic C1041 in parallel builds)
  target_compile_options(CloudSeamanor_core PUBLIC /utf-8 /FS)
endif()

if(CLOUDSEAMANOR_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(CloudSeamanor_core PRIVATE /W4 /permissive-)
  else()
    target_compile_options(CloudSeamanor_core PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

# ============================================================================
# 游戏可执行文件
# ============================================================================
add_executable(CloudSeamanor_game
  ${CLOUDSEAMANOR_APP_SOURCES}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${CLOUDSEAMANOR_APP_SOURCES}
)

set_target_properties(CloudSeamanor_core PROPERTIES FOLDER "Runtime/Core")
set_target_properties(CloudSeamanor_game PROPERTIES FOLDER "Runtime/App")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CloudSeamanor_game)

target_link_libraries(CloudSeamanor_game
  PRIVATE
    CloudSeamanor_core
)

if(WIN32)
  add_custom_command(
    TARGET CloudSeamanor_game
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:CloudSeamanor_game>
      $<TARGET_FILE_DIR:CloudSeamanor_game>
    COMMAND_EXPAND_LISTS
  )
endif()

# ============================================================================
# 复制资源文件到构建输出目录（确保运行时可以找到资源）
# ============================================================================
option(CLOUDSEAMANOR_COPY_ASSETS "Copy assets and configs to build output" ON)

if(CLOUDSEAMANOR_COPY_ASSETS)
  # 获取构建输出目录
  set(BUILD_OUTPUT_DIR $<TARGET_FILE_DIR:CloudSeamanor_game>)

  # 复制 assets 目录
  add_custom_command(TARGET CloudSeamanor_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/assets
      ${BUILD_OUTPUT_DIR}/assets
    COMMENT "Copying assets to build directory"
  )

  # 复制 configs 目录
  add_custom_command(TARGET CloudSeamanor_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/configs
      ${BUILD_OUTPUT_DIR}/configs
    COMMENT "Copying configs to build directory"
  )
endif()

# ============================================================================
# 测试目标
# ============================================================================
option(BUILD_TESTING "Build unit tests" OFF)

if(BUILD_TESTING)
  enable_testing()

  set(TEST_SOURCES
    tests/TestRunner.cpp
  )

  add_executable(CloudSeamanor_tests
    ${TEST_SOURCES}
  )

  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TEST_SOURCES})

  target_include_directories(CloudSeamanor_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )

  target_link_libraries(CloudSeamanor_tests
    PRIVATE
      CloudSeamanor_core
  )

  add_test(NAME CloudSeamanor_tests COMMAND CloudSeamanor_tests)

  if(MSVC)
    target_compile_options(CloudSeamanor_tests PRIVATE /utf-8 /FS $<$<CONFIG:Debug>:/Z7>)
  endif()

  set_target_properties(CloudSeamanor_tests PROPERTIES FOLDER "Tests")
endif()
