#pragma once

// ============================================================================
// 【FestivalMiniGameSystem】节日小游戏系统
// ============================================================================
// 提供节日小游戏的通用接口和基础实现。
// 支持：吃月饼、猜花、放风筝、采茶、茶艺表演
// ============================================================================

#include <string>
#include <vector>
#include <functional>
#include <random>

namespace CloudSeamanor::domain {

// ============================================================================
// 【MiniGameType】小游戏类型
// ============================================================================
enum class MiniGameType : std::uint8_t {
    None = 0,
    CatchMooncake,    // 吃月饼
    GuessFlower,      // 猜花
    FlyKite,          // 放风筝
    HarvestTea,       // 采茶
    TeaCeremony,      // 茶艺表演
    COUNT
};

// ============================================================================
// 【MiniGameConfig】小游戏配置
// ============================================================================
struct MiniGameConfig {
    std::string name;
    std::string description;
    float duration = 30.0f;           // 游戏时长（秒）
    int base_score = 100;             // 基础分数
    int perfect_score = 500;          // 满分
    std::string difficulty;            // 难度
};

// ============================================================================
// 【GameResult】游戏结果
// ============================================================================
struct GameResult {
    int score = 0;
    int rank = 5;                     // 1=S, 2=A, 3=B, 4=C, 5=D
    int gold_reward = 0;
    int item_id = 0;
    int item_count = 0;
    std::string rank_text;
    std::string message;
};

// ============================================================================
// 【MiniGameState】小游戏状态（内部使用）
// ============================================================================
struct MiniGameState {
    MiniGameType type = MiniGameType::None;
    float time_left = 0.0f;
    float total_time = 30.0f;
    int score = 0;
    bool running = false;
    bool ended = false;

    // 吃月饼游戏专用
    struct {
        float player_x = 0.0f;
        std::vector<float> item_y;
        std::vector<float> item_x;
    } catch_game;

    // 猜花游戏专用
    struct {
        std::vector<int> sequence;
        int player_index = 0;
        int correct_count = 0;
    } guess_game;

    // 放风筝游戏专用
    struct {
        float kite_x = 0.0f;
        float kite_y = 0.0f;
        float wind = 0.0f;
    } kite_game;

    // 采茶游戏专用
    struct {
        std::vector<std::pair<float, float>> tea_positions;
        int collected = 0;
    } harvest_game;

    // 茶艺游戏专用
    struct {
        std::vector<int> required_sequence;
        std::vector<int> player_sequence;
        int current_step = 0;
    } ceremony_game;
};

// ============================================================================
// 【FestivalMiniGameSystem】节日小游戏系统
// ============================================================================
class FestivalMiniGameSystem {
public:
    // ========================================================================
    // 单例
    // ========================================================================
    static FestivalMiniGameSystem& Instance();

    // ========================================================================
    // 初始化
    // ========================================================================
    void Initialize();

    // ========================================================================
    // 游戏控制
    // ========================================================================
    void StartGame(MiniGameType type);
    bool Update(float delta_seconds);
    bool HandleInput(int key);
    bool HandleClick(float x, float y);

    // ========================================================================
    // 状态查询
    // ========================================================================
    [[nodiscard]] bool IsGameRunning() const { return state_.running; }
    [[nodiscard]] MiniGameType GetCurrentGame() const { return state_.type; }
    [[nodiscard]] float GetTimeLeft() const { return state_.time_left; }
    [[nodiscard]] int GetScore() const { return state_.score; }
    [[nodiscard]] const MiniGameState& GetState() const { return state_; }

    // ========================================================================
    // 结果
    // ========================================================================
    GameResult EndGame();

    // ========================================================================
    // 配置
    // ========================================================================
    const MiniGameConfig& GetConfig(MiniGameType type) const;

private:
    FestivalMiniGameSystem() = default;
    ~FestivalMiniGameSystem() = default;
    FestivalMiniGameSystem(const FestivalMiniGameSystem&) = delete;
    FestivalMiniGameSystem& operator=(const FestivalMiniGameSystem&) = delete;

    // 游戏逻辑
    void UpdateCatchGame_(float delta);
    void UpdateGuessGame_(float delta);
    void UpdateKiteGame_(float delta);
    void UpdateHarvestGame_(float delta);
    void UpdateCeremonyGame_(float delta);

    // 结果计算
    int CalculateRank_(int score, int max_score) const;
    GameResult BuildResult_(MiniGameType type, int score, int rank) const;

    MiniGameState state_{};
    std::mt19937 rng_{std::random_device{}()};

    // 小游戏配置表
    std::unordered_map<MiniGameType, MiniGameConfig> configs_;
};

}  // namespace CloudSeamanor::domain
