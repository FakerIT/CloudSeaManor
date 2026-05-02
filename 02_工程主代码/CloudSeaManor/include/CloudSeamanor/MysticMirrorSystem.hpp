#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace CloudSeamanor::domain {

// ============================================================================
// 【FortuneTip】每日运势数据
// ============================================================================
struct FortuneTip {
    std::string id;
    std::string season;         // spring|summer|autumn|winter|all
    std::string weather;        // any|clear|lightMist|heavyCloud|heavyRain|snow|tide
    std::string tide_type;      // normal|tide
    int favor_min = 0;         // 最低好感度要求
    std::string text_zh;
    std::string text_en;
};

// ============================================================================
// 【RecipeTip】食谱提示数据
// ============================================================================
struct RecipeTip {
    std::string id;
    std::string season;
    std::string weather;
    std::string tide_type;
    std::string recipe_id;       // 空表示推荐类，非空表示解锁类
    std::string text_zh;
    std::string text_en;
};

// ============================================================================
// 【WeatherForecast】天气预报数据
// ============================================================================
struct WeatherForecast {
    std::string weather_code;
    std::string forecast_zh;
    std::string forecast_en;
    std::string activity_hint;
};

// ============================================================================
// 【MysticMirrorState】观云镜状态
// ============================================================================
struct MysticMirrorState {
    int view_count = 0;                    // 今日查看次数
    std::string today_fortune_id;          // 今日运势ID
    std::string today_recipe_tip_id;       // 今日食谱提示ID
    std::vector<std::string> history_ids;  // 历史记录ID
    int total_views = 0;                   // 累计查看次数
};

// ============================================================================
// 【MysticMirrorViewData】观云镜界面数据
// ============================================================================
struct MysticMirrorViewData {
    std::string fortune_text;
    std::string weather_today;
    std::string weather_tomorrow;
    std::string weather_day_after;
    int tide_day_of_month = -1;
    std::string recipe_text;
    std::string recipe_ingredients;
    float cloud_density = 0.0f;
    std::string aura_status;
};

// ============================================================================
// 【MysticMirrorSystem】观云镜系统
// ============================================================================
class MysticMirrorSystem {
public:
    // ========================================================================
    // 单例
    // ========================================================================
    static MysticMirrorSystem& Instance();

    // ========================================================================
    // 初始化
    // ========================================================================
    bool Initialize();
    void LoadFortuneData(const std::string& csv_path);
    void LoadRecipeTipsData(const std::string& csv_path);

    // ========================================================================
    // 每日刷新（在日切时调用）
    // ========================================================================
    void RefreshDaily();

    // ========================================================================
    // 查看观云镜
    // ========================================================================
    MysticMirrorViewData GetViewData() const;
    void RecordView();

    // ========================================================================
    // 查询接口
    // ========================================================================
    const FortuneTip* GetTodayFortune() const;
    const RecipeTip* GetTodayRecipeTip() const;
    std::vector<const WeatherForecast*> GetWeatherForecast(int days = 3) const;

    // ========================================================================
    // 存档
    // ========================================================================
    void LoadState(const MysticMirrorState& state);
    const MysticMirrorState& GetState() const { return state_; }

private:
    MysticMirrorSystem() = default;
    ~MysticMirrorSystem() = default;
    MysticMirrorSystem(const MysticMirrorSystem&) = delete;
    MysticMirrorSystem& operator=(const MysticMirrorSystem&) = delete;

    // 数据存储
    std::vector<FortuneTip> fortune_tips_;
    std::vector<RecipeTip> recipe_tips_;
    std::map<std::string, WeatherForecast> weather_forecasts_;

    // 玩家状态
    MysticMirrorState state_;
    int current_day_ = 0;  // 用于检测日期变化

    // 辅助方法
    const FortuneTip* SelectFortune() const;
    const RecipeTip* SelectRecipeTip() const;
    float CalculateCloudDensity() const;
    std::string GetAuraStatus(float density) const;
    bool MatchesCondition(const FortuneTip& tip, const std::string& season,
                         const std::string& weather, const std::string& tide_type, int favor) const;
};

}  // namespace CloudSeamanor::domain
