#include "CloudSeamanor/domain/FestivalMiniGameSystem.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::domain {

// ============================================================================
// 【单例】
// ============================================================================
FestivalMiniGameSystem& FestivalMiniGameSystem::Instance() {
    static FestivalMiniGameSystem instance;
    return instance;
}

// ============================================================================
// 【Initialize】
// ============================================================================
void FestivalMiniGameSystem::Initialize() {
    // 初始化小游戏配置
    configs_[MiniGameType::CatchMooncake] = {
        "吃月饼", "接住从天而降的月饼！",
        30.0f, 100, 500, "简单"
    };
    configs_[MiniGameType::GuessFlower] = {
        "猜花", "记住花朵出现的顺序！",
        45.0f, 100, 500, "中等"
    };
    configs_[MiniGameType::FlyKite] = {
        "放风筝", "控制风筝保持平衡！",
        30.0f, 100, 500, "困难"
    };
    configs_[MiniGameType::HarvestTea] = {
        "采茶大赛", "快速采集最多的茶叶！",
        20.0f, 150, 600, "简单"
    };
    configs_[MiniGameType::TeaCeremony] = {
        "茶艺表演", "按照正确的顺序完成茶艺！",
        60.0f, 200, 800, "困难"
    };
}

// ============================================================================
// 【StartGame】
// ============================================================================
void FestivalMiniGameSystem::StartGame(MiniGameType type) {
    if (type == MiniGameType::None) return;

    state_.type = type;
    state_.running = true;
    state_.ended = false;
    state_.score = 0;

    const auto& config = configs_[type];
    state_.time_left = config.duration;
    state_.total_time = config.duration;

    // 根据游戏类型初始化状态
    switch (type) {
        case MiniGameType::CatchMooncake:
            state_.catch_game.player_x = 320.0f;
            state_.catch_game.item_y.clear();
            state_.catch_game.item_x.clear();
            break;

        case MiniGameType::GuessFlower:
            state_.guess_game.sequence.clear();
            state_.guess_game.player_index = 0;
            state_.guess_game.correct_count = 0;
            // 生成随机序列
            for (int i = 0; i < 5; ++i) {
                state_.guess_game.sequence.push_back(
                    std::uniform_int_distribution<int>(0, 3)(rng_)
                );
            }
            break;

        case MiniGameType::FlyKite:
            state_.kite_game.kite_x = 320.0f;
            state_.kite_game.kite_y = 200.0f;
            state_.kite_game.wind = 0.0f;
            break;

        case MiniGameType::HarvestTea:
            state_.harvest_game.tea_positions.clear();
            state_.harvest_game.collected = 0;
            // 生成茶叶位置
            for (int i = 0; i < 10; ++i) {
                state_.harvest_game.tea_positions.emplace_back(
                    std::uniform_real_distribution<float>(50.0f, 590.0f)(rng_),
                    std::uniform_real_distribution<float>(100.0f, 400.0f)(rng_)
                );
            }
            break;

        case MiniGameType::TeaCeremony:
            state_.ceremony_game.required_sequence.clear();
            state_.ceremony_game.player_sequence.clear();
            state_.ceremony_game.current_step = 0;
            // 生成茶艺序列
            for (int i = 0; i < 6; ++i) {
                state_.ceremony_game.required_sequence.push_back(
                    std::uniform_int_distribution<int>(0, 2)(rng_)
                );
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// 【Update】
// ============================================================================
bool FestivalMiniGameSystem::Update(float delta_seconds) {
    if (!state_.running) return true;

    state_.time_left -= delta_seconds;
    if (state_.time_left <= 0.0f) {
        state_.time_left = 0.0f;
        state_.running = false;
        state_.ended = true;
        return false;
    }

    // 更新各游戏逻辑
    switch (state_.type) {
        case MiniGameType::CatchMooncake:
            UpdateCatchGame_(delta_seconds);
            break;
        case MiniGameType::GuessFlower:
            UpdateGuessGame_(delta_seconds);
            break;
        case MiniGameType::FlyKite:
            UpdateKiteGame_(delta_seconds);
            break;
        case MiniGameType::HarvestTea:
            UpdateHarvestGame_(delta_seconds);
            break;
        case MiniGameType::TeaCeremony:
            UpdateCeremonyGame_(delta_seconds);
            break;
        default:
            break;
    }

    return true;
}

// ============================================================================
// 【HandleInput】
// ============================================================================
bool FestivalMiniGameSystem::HandleInput(int key) {
    if (!state_.running) return false;

    switch (state_.type) {
        case MiniGameType::CatchMooncake:
            // 左右移动
            if (key == 0) {  // 左
                state_.catch_game.player_x = std::max(30.0f, state_.catch_game.player_x - 15.0f);
            } else if (key == 1) {  // 右
                state_.catch_game.player_x = std::min(610.0f, state_.catch_game.player_x + 15.0f);
            }
            return true;

        case MiniGameType::GuessFlower:
            // 记录猜测
            if (key >= 0 && key <= 3) {
                state_.guess_game.player_index = key;
            }
            return true;

        case MiniGameType::FlyKite:
            // 上下调整
            if (key == 0) {  // 上
                state_.kite_game.kite_y = std::max(50.0f, state_.kite_game.kite_y - 10.0f);
            } else if (key == 1) {  // 下
                state_.kite_game.kite_y = std::min(350.0f, state_.kite_game.kite_y + 10.0f);
            }
            return true;

        default:
            return false;
    }
}

// ============================================================================
// 【HandleClick】
// ============================================================================
bool FestivalMiniGameSystem::HandleClick(float x, float y) {
    if (!state_.running) return false;

    switch (state_.type) {
        case MiniGameType::CatchMooncake:
            // 吃月饼游戏不使用点击
            return false;

        case MiniGameType::HarvestTea: {
            // 采集茶叶
            const float click_radius = 40.0f;
            for (auto it = state_.harvest_game.tea_positions.begin();
                 it != state_.harvest_game.tea_positions.end();) {
                float dx = x - it->first;
                float dy = y - it->second;
                if (std::sqrt(dx*dx + dy*dy) < click_radius) {
                    state_.score += 10;
                    state_.harvest_game.collected++;
                    it = state_.harvest_game.tea_positions.erase(it);
                    return true;
                } else {
                    ++it;
                }
            }
            return false;
        }

        default:
            return false;
    }
}

// ============================================================================
// 【EndGame】
// ============================================================================
GameResult FestivalMiniGameSystem::EndGame() {
    state_.running = false;
    state_.ended = true;

    const auto& config = configs_[state_.type];
    int rank = CalculateRank_(state_.score, config.perfect_score);
    return BuildResult_(state_.type, state_.score, rank);
}

// ============================================================================
// 【GetConfig】
// ============================================================================
const MiniGameConfig& FestivalMiniGameSystem::GetConfig(MiniGameType type) const {
    static const MiniGameConfig default_config{"未知", "", 30.0f, 0, 0, "普通"};
    auto it = configs_.find(type);
    return it != configs_.end() ? it->second : default_config;
}

// ============================================================================
// 【UpdateCatchGame_】
// ============================================================================
void FestivalMiniGameSystem::UpdateCatchGame_(float delta) {
    // 月饼下落
    float fall_speed = 150.0f * delta;

    for (size_t i = 0; i < state_.catch_game.item_y.size();) {
        state_.catch_game.item_y[i] += fall_speed;

        // 检查是否被接住
        float py = state_.catch_game.item_y[i];
        float px = state_.catch_game.item_x[i];
        if (py >= 420.0f && py <= 450.0f &&
            std::abs(px - state_.catch_game.player_x) < 50.0f) {
            state_.score += 5;
            state_.catch_game.item_y.erase(state_.catch_game.item_y.begin() + i);
            state_.catch_game.item_x.erase(state_.catch_game.item_x.begin() + i);
        }
        // 落地消失
        else if (py > 480.0f) {
            state_.catch_game.item_y.erase(state_.catch_game.item_y.begin() + i);
            state_.catch_game.item_x.erase(state_.catch_game.item_x.begin() + i);
        }
        else {
            ++i;
        }
    }

    // 生成新的月饼
    if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < 0.1f) {
        state_.catch_game.item_y.push_back(50.0f);
        state_.catch_game.item_x.push_back(
            std::uniform_real_distribution<float>(30.0f, 610.0f)(rng_)
        );
    }
}

// ============================================================================
// 【UpdateGuessGame_】
// ============================================================================
void FestivalMiniGameSystem::UpdateGuessGame_(float delta) {
    (void)delta;
    // 猜花游戏主要是输入驱动，Update只做超时处理
}

// ============================================================================
// 【UpdateKiteGame_】
// ============================================================================
void FestivalMiniGameSystem::UpdateKiteGame_(float delta) {
    // 模拟风力变化
    state_.kite_game.wind += std::uniform_real_distribution<float>(-50.0f, 50.0f)(rng_) * delta;
    state_.kite_game.wind = std::clamp(state_.kite_game.wind, -100.0f, 100.0f);

    // 风筝随风飘动
    state_.kite_game.kite_x += state_.kite_game.wind * delta;

    // 边界限制
    if (state_.kite_game.kite_x < 50.0f || state_.kite_game.kite_x > 590.0f) {
        // 超出边界则扣分
        state_.score = std::max(0, state_.score - 2);
        state_.kite_game.wind = -state_.kite_game.wind * 0.5f;
    }

    // 在范围内获得分数
    if (state_.kite_game.kite_x >= 100.0f && state_.kite_game.kite_x <= 540.0f) {
        state_.score += 1;
    }
}

// ============================================================================
// 【UpdateHarvestGame_】
// ============================================================================
void FestivalMiniGameSystem::UpdateHarvestGame_(float delta) {
    (void)delta;
    // 采茶游戏主要是点击驱动
}

// ============================================================================
// 【UpdateCeremonyGame_】
// ============================================================================
void FestivalMiniGameSystem::UpdateCeremonyGame_(float delta) {
    (void)delta;
    // 茶艺游戏主要是输入驱动
}

// ============================================================================
// 【CalculateRank_】
// ============================================================================
int FestivalMiniGameSystem::CalculateRank_(int score, int max_score) const {
    float ratio = static_cast<float>(score) / static_cast<float>(max_score);
    if (ratio >= 0.95f) return 1;  // S
    if (ratio >= 0.80f) return 2;  // A
    if (ratio >= 0.60f) return 3;  // B
    if (ratio >= 0.40f) return 4;  // C
    return 5;  // D
}

// ============================================================================
// 【BuildResult_】
// ============================================================================
GameResult FestivalMiniGameSystem::BuildResult_(MiniGameType type, int score, int rank) const {
    GameResult result;
    result.score = score;
    result.rank = rank;

    const char* rank_texts[] = {"S", "A", "B", "C", "D"};
    result.rank_text = rank_texts[rank - 1];

    // 根据等级计算奖励
    const int base_rewards[] = {100, 75, 50, 25, 10};
    result.gold_reward = base_rewards[rank - 1] * 10;

    switch (rank) {
        case 1: result.message = "完美表现！获得额外奖励！"; break;
        case 2: result.message = "表现出色！"; break;
        case 3: result.message = "不错的表现！"; break;
        case 4: result.message = "还需多加练习！"; break;
        default: result.message = "继续加油！"; break;
    }

    // S级奖励物品
    if (rank == 1) {
        result.item_id = 1001;  // 金月饼
        result.item_count = 3;
    } else if (rank == 2) {
        result.item_id = 1002;  // 银月饼
        result.item_count = 1;
    }

    return result;
}

}  // namespace CloudSeamanor::domain
