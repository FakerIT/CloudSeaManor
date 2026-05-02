#pragma once

// ============================================================================
// 【PixelUiConfig】像素 UI 布局配置常量
// ============================================================================
// 屏幕分辨率：1280 × 720
//
// 所有坐标和尺寸基于该分辨率设计。
// 组件使用整数倍缩放以保持像素锐利感。
// ============================================================================

#include <SFML/System/Vector2.hpp>
#include <cstdint>

namespace CloudSeamanor::engine {

// ============================================================================
// 【ScreenConfig】屏幕配置
// ============================================================================
struct ScreenConfig {
    static constexpr unsigned int Width = 1280u;
    static constexpr unsigned int Height = 720u;
    static constexpr float CenterX = Width / 2.0f;
    static constexpr float CenterY = Height / 2.0f;
};

// ============================================================================
// 【TopRightCorner】右上角信息区
// ============================================================================
struct TopRightInfoConfig {
    static constexpr float PosX = 998.0f;   // 右上角 x
    static constexpr float PosY = 6.0f;      // 右上角 y
    static constexpr float Width = 276.0f;
    static constexpr float Height = 64.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
};

// ============================================================================
// 【BottomRightStatusConfig】右下角状态条
// ============================================================================
struct BottomRightStatusConfig {
    static constexpr float PosX = 998.0f;
    static constexpr float PosY = 658.0f;
    static constexpr float Width = 276.0f;
    static constexpr float Height = 58.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
};

// ============================================================================
// 【ToolbarConfig】底部工具栏（12 格热键栏）
// ============================================================================
struct ToolbarConfig {
    static constexpr float PosX = 332.0f;   // 居中: (1280 - 616) / 2
    static constexpr float PosY = 662.0f;
    static constexpr int SlotCount = 12;
    static constexpr float SlotSize = 48.0f;
    static constexpr float SlotSpacing = 4.0f;
    static constexpr float BorderWidth = 1.0f;
    static constexpr float TotalWidth = SlotSize * SlotCount + SlotSpacing * (SlotCount - 1);
    static constexpr float Height = SlotSize + 14.0f;  // slot + key hint
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr float HighlightPulseFrequency = 8.0f;
    static constexpr float HighlightOutlineThickness = 2.0f;
    static constexpr std::uint8_t HighlightAlphaBase = 120u;
    static constexpr float HighlightAlphaRange = 110.0f;
};

// ============================================================================
// 【DialogueBoxConfig】对话框
// ============================================================================
struct DialogueBoxConfig {
    static constexpr float PosX = 40.0f;
    static constexpr float PosY = 520.0f;    // 底部 1/5
    static constexpr float Width = 1200.0f;
    static constexpr float Height = 160.0f;
    static constexpr float AvatarSize = 48.0f;
    static constexpr float TextPadding = 16.0f;
    static constexpr int CharsPerLine = 38;
    static constexpr float TypingSpeedMs = 40.0f;  // 每字符 40ms
    static constexpr float ChoiceRowHeight = 36.0f;
    static constexpr float ChoiceAreaBottomOffset = 40.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
};

// ============================================================================
// 【QuestMenuConfig】任务面板（F 键呼出）
// ============================================================================
struct QuestMenuConfig {
    static constexpr float PosX = 820.0f;   // 屏幕中央偏右
    static constexpr float PosY = 150.0f;
    static constexpr float Width = 360.0f;
    static constexpr float Height = 420.0f;
    static constexpr float TitleBarHeight = 32.0f;
    static constexpr float RowHeight = 36.0f;
    static constexpr float ExpandedHeight = 56.0f;
    static constexpr float TextPadding = 16.0f;
    static constexpr float ListTopMargin = 4.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
};

static constexpr float MinTouchTargetSize = 44.0f;

// ============================================================================
// 【InventoryGridConfig】背包/菜单（I 键呼出）
// ============================================================================
struct InventoryGridConfig {
    static constexpr float PosX = 320.0f;    // 居中
    static constexpr float PosY = 120.0f;
    static constexpr float Width = 640.0f;
    static constexpr float Height = 480.0f;
    static constexpr float TabBarHeight = 40.0f;
    static constexpr float SlotSize = 48.0f;
    static constexpr float SlotSpacing = 4.0f;
    static constexpr int Columns = 8;
    static constexpr int Rows = 4;
    static constexpr float TextPadding = 16.0f;
    static constexpr float InfoPanelWidth = 200.0f;
    static constexpr float SlotBorderThickness = 1.0f;
    static constexpr float SelectionOutlineThickness = 2.0f;
    static constexpr float DisabledMarkHalfSize = 8.0f;
    static constexpr float DisabledMarkThickness = 1.0f;
    static constexpr int SocialColumns = 2;
    static constexpr float SocialCardHeight = 84.0f;
    static constexpr float SocialCardSpacing = 10.0f;
    static constexpr float SocialAvatarOffsetX = 8.0f;
    static constexpr float SocialAvatarOffsetY = 8.0f;
    static constexpr float SocialAvatarSize = 36.0f;
    static constexpr float SocialFavorBarOffsetX = 52.0f;
    static constexpr float SocialFavorBarOffsetY = 18.0f;
    static constexpr float SocialFavorBarHeight = 10.0f;
    static constexpr float SocialEventBarOffsetX = 52.0f;
    static constexpr float SocialEventBarOffsetY = 34.0f;
    static constexpr float SocialEventBarHeight = 8.0f;
    static constexpr float SocialBarRightPadding = 12.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
};

// ============================================================================
// 【MinimapConfig】迷你地图（M 键呼出）
// ============================================================================
struct MinimapConfig {
    static constexpr float Width = 512.0f;
    static constexpr float Height = 512.0f;
    static constexpr float PlayerDotRadius = 4.0f;
    static constexpr sf::Vector2f Position{
        (ScreenConfig::Width - Width) / 2.0f,
        (ScreenConfig::Height - Height) / 2.0f
    };
    static constexpr sf::Vector2f Size{Width, Height};
};

// ============================================================================
// 【PixelBorderConfig】像素边框配置
// ============================================================================
struct PixelBorderConfig {
    static constexpr float CornerBlockSize = 8.0f;
    static constexpr float BorderThickness = 1.0f;
    static constexpr float InnerShadowOffset = 1.0f;
    static constexpr float InnerShadowDepth = 2.0f;
    static constexpr float TitleBarHeight = 28.0f;
    static constexpr float InnerShadowThickness = 1.0f;
};

struct UiPanelConfig {
    static constexpr float DefaultWidth = 100.0f;
    static constexpr float DefaultHeight = 100.0f;
};

// ============================================================================
// 【StaminaBarConfig】体力条配置
// ============================================================================
struct StaminaBarConfig {
    static constexpr float PosX = 1002.0f;
    static constexpr float PosY = 664.0f;
    static constexpr float Width = 272.0f;
    static constexpr float Height = 12.0f;
    static constexpr sf::Vector2f Position{PosX, PosY};
    static constexpr sf::Vector2f Size{Width, Height};
    static constexpr float LowThreshold = 0.20f;   // <20% 触发低体力警告
    static constexpr float CriticalThreshold = 0.10f; // <10% 触发严重警告
};

// ============================================================================
// 【AnimationConfig】动画配置
// ============================================================================
struct AnimationConfig {
    static constexpr float FadeInDuration = 0.15f;    // 秒
    static constexpr float FadeOutDuration = 0.10f;
    static constexpr float BlinkInterval = 0.5f;       // 闪烁间隔
    static constexpr float TypewriterSpeedMs = 40.0f;  // 每字符 ms
};

// ============================================================================
// 【ControllerConfig】焦点导航配置
// ============================================================================
struct ControllerConfig {
    static constexpr float NavigationPadding = 4.0f;
    static constexpr float FocusRingThickness = 2.0f;
    static constexpr float FocusBreathAmplitude = 2.0f;
    static constexpr float FocusBreathPeriod = 0.3f;
};

// ============================================================================
// 【RenderLayer】渲染层次
// ============================================================================
enum class RenderLayer : std::uint8_t {
    BackgroundScene = 0,
    AuraOverlay = 1,
    DayNightGlow = 2,
    Minimap = 3,
    DialogueBox = 4,
    Inventory = 5,
    QuestMenu = 6,
    TopRightInfo = 7,
    BottomRightStatus = 8,
    Toolbar = 9,
    WorldTip = 10,
    Notification = 11,
    DebugInfo = 12,
};

}  // namespace CloudSeamanor::engine
