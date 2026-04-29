#pragma once

// ============================================================================
// 【InputManager】输入管理器
// ============================================================================
// 统一管理游戏输入：按键到动作的映射、帧级状态查询、移动向量合成。
//
// 主要职责：
// - 定义游戏动作枚举（移动/交互/背包/冲刺等）
// - 键位绑定与默认配置
// - 帧级按键状态（Pressed / JustPressed）
// - 移动向量合成（键盘 + 手柄预留）
// - 可配置键位映射
//
// 设计原则：
// - 不决定按键触发什么逻辑（由场景/系统决定）
// - 不直接修改游戏状态
// - 每帧必须调用 BeginNewFrame 更新上一帧状态
//
// 使用示例：
// @code
// InputManager input;
// input.BeginNewFrame();
// while (window.pollEvent(event)) {
//     input.HandleEvent(event);
// }
// if (input.IsActionJustPressed(Action::Interact)) {
//     // 处理交互...
// }
// @endcode
// ============================================================================

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【Action】游戏动作枚举
// ============================================================================
enum class Action : std::uint8_t {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Interact,      // E
    OpenInventory,  // I
    OpenQuestMenu, // F：任务面板
    OpenMap,       // M：地图
    Cancel,        // Esc
    Confirm,       // Enter
    Sprint,        // Shift
    UseTool,       // Q
    EatFood,       // H：快捷食用
    OpenMenu,      // Esc（快捷菜单）
    DebugToggle,   // F3：调试面板开关
    CloudToggle,   // F5：云海调试切换
    QuickSave,     // F6
    QuickLoad,     // F9
    AdvanceDialog, // E（对话中推进）
    GiftNpc,       // G
    Sleep,         // T
    HotbarSlot1,
    HotbarSlot2,
    HotbarSlot3,
    HotbarSlot4,
    HotbarSlot5,
    HotbarSlot6,
    HotbarSlot7,
    HotbarSlot8,
    HotbarSlot9,
    HotbarSlot10,
    HotbarSlot11,
    HotbarSlot12,
    Count
};

// ============================================================================
// 【InputState】当前帧输入状态快照
// ============================================================================
struct InputState {
    std::array<bool, static_cast<std::size_t>(Action::Count)> pressed{};

    [[nodiscard]] bool IsActionPressed(Action action) const {
        return pressed[static_cast<std::size_t>(action)];
    }
};

// ============================================================================
// 【InputManager】输入管理器类
// ============================================================================
class InputManager {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    InputManager();

    // ========================================================================
    // 【每帧调用】
    // ========================================================================

    /**
     * @brief 开始新的一帧。重置 JustPressed 状态。
     * @note 必须在 ProcessEvents 开始时调用！
     */
    void BeginNewFrame();

    /**
     * @brief 处理单个 SFML 事件。
     */
    void HandleEvent(const sf::Event& event);

    // ========================================================================
    // 【状态查询】
    // ========================================================================

    /**
     * @brief 当前帧按住该动作。
     */
    [[nodiscard]] bool IsActionPressed(Action action) const;

    /**
     * @brief 本帧刚刚按下该动作（上一帧未按下，本帧按下）。
     * @note 仅在调用 BeginNewFrame 后的一帧内返回 true。
     */
    [[nodiscard]] bool IsActionJustPressed(Action action) const;

    /**
     * @brief 获取当前帧的输入快照。
     */
    [[nodiscard]] const InputState& GetCurrentState() const { return current_; }

    // ========================================================================
    // 【移动向量】
    // ========================================================================

    /**
     * @brief 获取 2D 移动向量（已归一化）。
     * @return 单位向量（长度为 1.0）或零向量。
     */
    [[nodiscard]] sf::Vector2f GetMovementVector() const;

    // ========================================================================
    // 【键位绑定】
    // ========================================================================

    /**
     * @brief 将键盘按键绑定到游戏动作。
     */
    void BindKey(sf::Keyboard::Key key, Action action);

    /**
     * @brief 从配置文件加载键位。
     * @param path 配置文件路径（key=value 格式）
     * @return true 表示加载成功
     */
    bool LoadFromFile(const std::string& path);

    /**
     * @brief 重置为默认键位。
     */
    void ResetToDefaults();

    /**
     * @brief 获取动作对应的默认键位。
     */
    [[nodiscard]] sf::Keyboard::Key GetDefaultKey(Action action) const;
    [[nodiscard]] sf::Keyboard::Key GetPrimaryKey(Action action) const;
    [[nodiscard]] std::string GetPrimaryKeyName(Action action) const;

    // ========================================================================
    // 【手柄支持（预留）】
    // ========================================================================

    /**
     * @brief 获取手柄左摇杆方向向量（已归一化）。
     */
    [[nodiscard]] sf::Vector2f GetGamepadVector() const;

    /**
     * @brief 检查手柄是否已连接。
     */
    [[nodiscard]] bool IsGamepadConnected() const;

private:
    // 动作到按键的绑定表（支持一键多动作）
    std::array<std::vector<sf::Keyboard::Key>, static_cast<std::size_t>(Action::Count)> action_bindings_;

    // 当前帧状态
    InputState current_;

    // 上一帧状态（用于计算 JustPressed）
    InputState previous_;

    // 按当前按键状态重建 current_，用于支持事件遗漏时的兜底一致性。
    void RebuildCurrentState_();
};

// ============================================================================
// 【ActionDisplayName】动作显示名称
// ============================================================================
[[nodiscard]] const char* ActionDisplayName(Action action);

// ============================================================================
// 【KeyName】将按键枚举转换为可读文本
// ============================================================================
[[nodiscard]] std::string KeyName(sf::Keyboard::Key key);

}  // namespace CloudSeamanor::engine
