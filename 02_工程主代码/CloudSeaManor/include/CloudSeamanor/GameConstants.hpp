#pragma once

// ============================================================================
// 【GameConstants】游戏平衡常量
// ============================================================================
// 集中管理所有游戏平衡参数，替代散落在各 .cpp 中的魔法数字。
// 调整游戏平衡时只需修改此文件，无需搜索和修改多处源码。
//
// 分组：
// - Player:      玩家移动、体力
// - SpiritBeast: 灵兽行为参数
// - Npc:         NPC 好感度
// - Skill:       技能系统
// - Cloud:       云海系统
// - DynamicLife: 动态人生系统
// - Workshop:    工坊系统
// - Crop:        作物系统
// - Tool:        工具系统
// - Festival:    节日系统
// - Inventory:   背包系统
// - Initial:     初始物品
// ============================================================================

#include <cstdint>

namespace CloudSeamanor::GameConstants {

// ============================================================================
// Player - 玩家参数
// ============================================================================
namespace Player {
    constexpr float Speed = 240.0f;
    constexpr float StaminaMovePerSecond = 3.0f;
    constexpr float StaminaInteractCost = 6.0f;
    constexpr float StaminaRecoverPerSecond = 3.0f;
    constexpr float StaminaMax = 350.0f;
    constexpr float StaminaInitial = 120.0f;
    constexpr float LowStaminaWarningRatio = 0.2f;
    constexpr float LowStaminaRecoverRatio = 0.35f;
    constexpr float InteractExtraRange = 18.0f;
}

// ============================================================================
// SpiritBeast - 灵兽参数
// ============================================================================
namespace SpiritBeast {
    constexpr float FollowTriggerDistance = 8.0f;
    constexpr float FollowStartDistance = 140.0f;
    constexpr float FollowStopDistance = 200.0f;
    constexpr float FollowKeepDistance = 80.0f;
    constexpr float FollowSpeed = 120.0f;
    constexpr float WanderArrivalDistance = 6.0f;
    constexpr float WanderSpeed = 70.0f;
    constexpr float IdleDuration = 5.0f;
    constexpr float InteractDuration = 1.5f;
    constexpr float BodyRadius = 18.0f;
    constexpr int BodyPointCount = 24;
    constexpr float ParticleRadius = 6.0f;
    constexpr int ParticlePointCount = 20;
    constexpr float ParticleRiseSpeed = 32.0f;
    constexpr float ParticleLifetime = 1.2f;
    constexpr float ParticleGravity = 20.0f;
    constexpr int MaxParticles = 20;
    constexpr float StaminaRecoverAmount = 12.0f;
    constexpr float StaminaRecoverRatioThreshold = 0.45f;
    constexpr float AssistHarvestMultiplier = 1.2f;
}

// ============================================================================
// Npc - NPC 好感度参数
// ============================================================================
namespace Npc {
    constexpr float FavorLove = 10.0f;
    constexpr float FavorLike = 5.0f;
    constexpr float FavorDislike = -3.0f;
    constexpr float FavorNeutral = 1.0f;
    constexpr float FavorToDynamicLifeMultiplier = 3.0f;
    constexpr int FavorDailyThreshold = 4;
    constexpr float FavorDailyToDynamicLifeMultiplier = 2.0f;
    constexpr float DailyGiftDynamicLifePoints = 5.0f;
    constexpr int EveningHighFavorThreshold = 60;
    constexpr int HasTeaPackFavorThreshold = 50;
    constexpr int FavorNormalizeOffset = 20;
    constexpr float FavorNormalizeRange = 60.0f;
}

// ============================================================================
// Skill - 技能系统参数
// ============================================================================
namespace Skill {
    constexpr float InitialExpToNext = 100.0f;
    constexpr float ExpGrowthRate = 1.35f;
    constexpr int MaxLevel = 10;
    constexpr float CloudMultiplierCap = 1.2f;
    constexpr float PerLevelBonus = 0.05f;
    constexpr float MaxLevelExtraBonus = 0.5f;
    constexpr float SpiritFarmExpBase = 20.0f;
    constexpr float SpiritForageExpBase = 15.0f;
    constexpr float SkillBonusToBuffRatio = 0.1f;
}

// ============================================================================
// Cloud - 云海系统参数
// ============================================================================
namespace Cloud {
    constexpr int NewbieProtectionDays = 14;
    constexpr int NewbieCyclePeriod = 3;
    constexpr float SpiritEnergyNormalizeDivisor = 500.0f;
    constexpr float PlayerInfluenceNormalizeDivisor = 100.0f;
    constexpr float PlayerInfluenceMin = -0.5f;
    constexpr float PlayerInfluenceMax = 0.5f;

    constexpr float TideBaseProbability = 0.05f;
    constexpr float TideSpiritBonus = 0.10f;
    constexpr float TidePositiveBonus = 0.05f;

    constexpr float DenseBaseProbability = 0.20f;
    constexpr float DenseSpiritBonus = 0.15f;
    constexpr float DensePositiveBonus = 0.10f;

    constexpr float MistBaseProbability = 0.35f;
    constexpr float MistPositiveBonus = 0.10f;

    constexpr int WateredPlotInfluence = 5;
    constexpr int MainHouseRepairInfluence = 5;
    constexpr int NoBeastInteractionPenalty = -3;
}

// ============================================================================
// DynamicLife - 动态人生系统参数
// ============================================================================
namespace DynamicLife {
    constexpr float PlayerWeight = 0.7f;
    constexpr float CloudWeight = 0.2f;
    constexpr float EventWeight = 0.1f;

    constexpr float Stage1Threshold = 80.0f;
    constexpr float Stage2Threshold = 160.0f;
    constexpr float Stage3Threshold = 240.0f;

    constexpr float CloudBonusLow = 2.0f;
    constexpr float CloudBonusMid = 6.0f;
    constexpr float CloudBonusHigh = 14.0f;

    constexpr float CloudDensityMid = 0.4f;
    constexpr float CloudDensityHigh = 0.7f;
}

// ============================================================================
// Workshop - 工坊系统参数
// ============================================================================
namespace Workshop {
    constexpr float CloudSpeedBonus = 0.5f;
    constexpr float ProgressBase = 100.0f;
    constexpr int DefaultProcessTime = 60;
    constexpr float DefaultSuccessRate = 0.85f;
}

// ============================================================================
// Crop - 作物系统参数
// ============================================================================
namespace Crop {
    constexpr float DefaultGrowthTime = 80.0f;
    constexpr int DefaultStages = 4;
    constexpr int DefaultBaseHarvest = 2;
    constexpr int StaminaCost = 6;
    constexpr float QualityNormalMultiplier = 1.0f;
    constexpr float QualityFineMultiplier = 1.3f;
    constexpr float QualityRareMultiplier = 1.8f;
    constexpr float QualitySpiritMultiplier = 2.5f;
    constexpr float GrowthStageDivisor = 100.0f;
}

// ============================================================================
// Tool - 工具系统参数
// ============================================================================
namespace Tool {
    constexpr float InitialExpToNext = 50.0f;
    constexpr float ExpPerUse = 10.0f;
    constexpr float ExpThresholdTier2 = 50.0f;
    constexpr float ExpThresholdTier3 = 150.0f;
    constexpr float ExpThresholdTier4 = 400.0f;
    constexpr float ExpThresholdTier5 = 1000.0f;
    constexpr float ExpCapBeyondMaxTier = 99999.0f;
}

// ============================================================================
// Festival - 节日系统参数
// ============================================================================
namespace Festival {
    constexpr int NormalNoticeDays = 3;
    constexpr int TideNoticeDays = 7;
    constexpr int CrossSeasonDayOffset = 28;
}

// ============================================================================
// Inventory - 背包参数
// ============================================================================
namespace Inventory {
    constexpr int ToolbarSlotCount = 12;
    constexpr int BagColumns = 8;
    constexpr int BagRows = 4;
    constexpr int BagCapacity = BagColumns * BagRows;
}

// ============================================================================
// Runtime - 运行时参数
// ============================================================================
namespace Runtime {
    constexpr float DefaultTimeScale = 1.0f;
    constexpr float WorkshopProgressMax = 100.0f;
    constexpr float WorkshopProgressHidden = -1.0f;
    constexpr float SpiritForageExpGain = 15.0f;
}

// ============================================================================
// UI - UI 与动画参数
// ============================================================================
namespace Ui {
    constexpr unsigned int WindowWidth = 1280u;
    constexpr unsigned int WindowHeight = 720u;
    constexpr unsigned int TargetFps = 60u;

    namespace MainMenu {
        constexpr std::string_view Title = "云海山庄";
        constexpr std::string_view NewGameLabel = "新游戏";
        constexpr std::string_view ContinueLabel = "继续";
        constexpr std::string_view SettingsLabel = "设置";
    }

    namespace Pulse {
        constexpr float TimeScale = 2.4f;
        constexpr float Frequency = 1.35f;
        constexpr float Bias = 0.5f;
        constexpr float Amplitude = 0.5f;
        constexpr std::uint8_t AlphaBase = 180u;
        constexpr float AlphaRange = 60.0f;
    }

    namespace LevelUp {
        constexpr float FadeDuration = 0.5f;
        constexpr float MaxScaleOffset = 0.3f;
        constexpr float FloatDistance = 20.0f;
        constexpr float OverlayDuration = 2.5f;
    }

    namespace HintDuration {
        constexpr float Welcome = 5.2f;
        constexpr float SkillLevelUp = 3.2f;
        constexpr float CloudStateChanged = 3.6f;
        constexpr float LowStaminaWarning = 2.8f;
        constexpr float LowStaminaRecovered = 1.8f;
        constexpr float TutorialMove = 4.0f;
        constexpr float TutorialInteract = 3.4f;
        constexpr float TutorialCrop = 4.0f;
        constexpr float TutorialSave = 4.4f;
        constexpr float WorkshopOutput = 3.0f;
    }
}

// ============================================================================
// Battle - 战斗系统参数
// ============================================================================
namespace Battle {
    // 玩家能量
    constexpr float EnergyMax = 100.0f;
    constexpr float EnergyRecoverPerSecond = 5.0f;

    // 净化参数
    constexpr float CritMultiplier = 1.5f;
    constexpr float ElementAdvantageMultiplier = 1.5f;
    constexpr float HeartBonusPerLevel = 0.1f;

    // 敌人难度修正
    constexpr float ElitePollutionMultiplier = 2.0f;
    constexpr float EliteDifficultyModifier = 1.5f;
    constexpr float BossPollutionMultiplier = 3.0f;
    constexpr float BossDifficultyModifier = 2.0f;

    // 天气倍率
    constexpr float WeatherClear = 1.0f;
    constexpr float WeatherMist = 1.2f;
    constexpr float WeatherDense = 1.35f;
    constexpr float WeatherTide = 1.5f;

    // 战斗参数
    constexpr float RetreatDistance = 60.0f;       // 触发战斗距离
    constexpr float SpiritApproachSpeed = 60.0f;   // 灵体接近速度
    constexpr float ExhaustionDuration = 1800.0f;   // 战后疲劳时间（秒=30分钟）

    // UI
    constexpr float HealthBarWidth = 120.0f;
    constexpr float HealthBarHeight = 8.0f;
    constexpr int MaxSkillSlots = 4;
    constexpr int MaxLogLines = 6;
}

// ============================================================================
// Initial - 初始物品
// ============================================================================
namespace Initial {
    constexpr int TeaSeedCount = 4;
    constexpr int TurnipSeedCount = 2;
    constexpr int WoodCount = 5;
}

}  // namespace CloudSeamanor::GameConstants
