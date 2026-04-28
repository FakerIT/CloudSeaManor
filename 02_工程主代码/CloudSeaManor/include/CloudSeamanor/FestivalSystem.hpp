#pragma once

// ============================================================================
// 【FestivalSystem】节日与大型活动系统
// ============================================================================
// 统一管理游戏中的节日事件、预告和奖励发放。
//
// 主要职责：
// - 管理12个年度节日和云海大潮
// - 维护节日预告（节日前3天/大潮前7天）
// - 计算节日强度（由云海浓度、山庄状态决定）
// - 处理节日参与和奖励结算
//
// 与其他系统的关系：
// - 依赖：GameClock（日期）、CloudSystem（云海状态）
// - 被依赖：GameAppHud（节日提示）、GameAppSave（存档/读档）
//
// 设计原则：
// - 12个常驻节日 + 云海大潮（年度特殊）
// - 节日前3天预告，大潮前7天预告
// - 节日强度由云海浓度、灵气值、建筑状态共同决定
// - 错过不惩罚，可跨年补完
//
// 节日排布：
// - 春：山庄苏醒祭(8)、灵茶初芽节(15)、春风约会日(22)
// - 夏：云海灯会(12)、灵兽运动会(19)、夏夜温泉祭(26)
// - 秋：丰收宴(7)、灵兽大比(14)、秋忆祭(23)
// - 冬：雪雾守岁(15)、年终云海大典(28)
// ============================================================================

#include "CloudSeamanor/GameClock.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【FestivalType】节日类型枚举
// ============================================================================
enum class FestivalType {
    Normal,   // 常驻节日
    CloudTide, // 云海大潮
};

// ============================================================================
// 【Festival】节日定义
// ============================================================================
// 表示一个节日的完整信息。
struct Festival {
    std::string id;                  // 节日ID
    std::string name;                 // 节日名称
    std::string description;          // 节日描述
    FestivalType type = FestivalType::Normal; // 类型
    int season = 0;                  // 季节（0-3，春夏秋冬）
    int day = 1;                    // 日期（1-28）
    int notice_days = 3;            // 预告天数（节前3天，大潮7天）
    std::string activity;            // 主要活动
    std::string reward;             // 参与奖励
    bool participated = false;      // 今日是否已参加
};

// ============================================================================
// 【FestivalSystem】节日系统领域对象
// ============================================================================
// 管理年度节日和大型活动的生命周期。
//
// 设计决策：
// - 使用固定日期触发节日
// - 节日强度由云海浓度动态决定
// - 支持节日预告显示
// - 参与奖励可配置
//
// 使用示例：
// @code
// FestivalSystem festivals;
// festivals.Initialize();
// if (auto* today = festivals.GetTodayFestival()) {
//     festivals.Participate("festival_id");
// }
// @endcode
class FestivalSystem {
public:
    // ========================================================================
    // 【Initialize】初始化节日系统
    // ========================================================================
    // @note 首次运行时调用
    void Initialize(const std::string& csv_path = "assets/data/festival/festival_definitions.csv");

    // ========================================================================
    // 【Update】每日更新
    // ========================================================================
    // @param season 当前季节
    // @param day_in_season 当前季节第几天
    // @note 检查今日是否有节日，更新预告状态
    void Update(CloudSeamanor::domain::Season season, int day_in_season);

    // ========================================================================
    // 【Participate】参与节日
    // ========================================================================
    // @param festival_id 节日ID
    // @note 标记为已参与，发放奖励
    void Participate(const std::string& festival_id);

    // ========================================================================
    // 【SetParticipated】设置节日参与状态
    // ========================================================================
    void SetParticipated(const std::string& festival_id, bool value);

    // ========================================================================
    // 【IsNoticeVisible】节日预告是否可见
    // ========================================================================
    [[nodiscard]] bool IsNoticeVisible(const std::string& festival_id) const;

    // ========================================================================
    // 【GetTodayFestival】获取今日节日
    // ========================================================================
    [[nodiscard]] const Festival* GetTodayFestival() const;

    // ========================================================================
    // 【GetUpcomingFestivals】获取即将到来的节日
    // ========================================================================
    [[nodiscard]] std::vector<const Festival*> GetUpcomingFestivals(int max_count = 3) const;

    // ========================================================================
    // 【GetNoticeText】获取节日预告文本
    // ========================================================================
    [[nodiscard]] std::string GetNoticeText() const;

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const std::vector<Festival>& GetAllFestivals() const noexcept { return festivals_; }
    [[nodiscard]] const Festival* GetFestival(const std::string& id) const;

private:
    [[nodiscard]] bool LoadFromCsv_(const std::string& csv_path);
    [[nodiscard]] static std::vector<std::string> SplitCsvLine_(const std::string& line);
    [[nodiscard]] static int SeasonFromText_(const std::string& text);
    [[nodiscard]] static std::string Trim_(const std::string& text);

    // ========================================================================
    // 【CreateDefaultFestivals_】创建默认节日数据
    // ========================================================================
    [[nodiscard]] std::vector<Festival> CreateDefaultFestivals_() const;

    // ========================================================================
    // 成员变量
    // ========================================================================
    std::vector<Festival> festivals_;
    std::string today_festival_id_;  // 今日节日ID
    std::vector<std::string> notice_festival_ids_;  // 正在预告的节日ID
};

}  // namespace CloudSeamanor::domain
