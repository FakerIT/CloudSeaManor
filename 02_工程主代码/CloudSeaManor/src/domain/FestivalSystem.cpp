#include "CloudSeamanor/domain/FestivalSystem.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【Initialize】初始化节日系统
// ============================================================================
void FestivalSystem::Initialize(const std::string& csv_path) {
    if (!LoadFromCsv_(csv_path)) {
        festivals_ = CreateDefaultFestivals_();
        infrastructure::Logger::Warning("Festival CSV load failed, fallback to built-in festivals: " + csv_path);
    } else {
        infrastructure::Logger::Info("Festival CSV loaded: " + csv_path + ", count=" + std::to_string(festivals_.size()));
    }
    today_festival_id_.clear();
    notice_festival_ids_.clear();
}

// ============================================================================
// 【Update】每日更新
// ============================================================================
void FestivalSystem::Update(CloudSeamanor::domain::Season season, int day_in_season) {
    // 查找今日节日
    today_festival_id_.clear();
    for (const auto& fest : festivals_) {
        if (fest.season == static_cast<int>(season) && fest.day == day_in_season) {
            today_festival_id_ = fest.id;
            break;
        }
    }

    // 更新预告列表
    notice_festival_ids_.clear();
    for (const auto& fest : festivals_) {
        int days_until = fest.day - day_in_season;
        // 处理跨季节情况
        if (fest.season < static_cast<int>(season)) {
            days_until += 28;  // 假设每季节28天
        } else if (fest.season > static_cast<int>(season)) {
            days_until = -1;  // 已在过去
        }

        if (days_until > 0 && days_until <= fest.notice_days) {
            notice_festival_ids_.push_back(fest.id);
        }
    }
}

// ============================================================================
// 【Participate】参与节日
// ============================================================================
void FestivalSystem::Participate(const std::string& festival_id) {
    for (auto& fest : festivals_) {
        if (fest.id == festival_id) {
            fest.participated = true;
            break;
        }
    }
}

// ============================================================================
// 【SetParticipated】设置节日参与状态
// ============================================================================
void FestivalSystem::SetParticipated(const std::string& festival_id, bool value) {
    for (auto& fest : festivals_) {
        if (fest.id == festival_id) {
            fest.participated = value;
            break;
        }
    }
}

// ============================================================================
// 【IsNoticeVisible】节日预告是否可见
// ============================================================================
bool FestivalSystem::IsNoticeVisible(const std::string& festival_id) const {
    return std::find(notice_festival_ids_.begin(), notice_festival_ids_.end(), festival_id)
           != notice_festival_ids_.end();
}

// ============================================================================
// 【GetTodayFestival】获取今日节日
// ============================================================================
const Festival* FestivalSystem::GetTodayFestival() const {
    if (today_festival_id_.empty()) return nullptr;

    for (const auto& fest : festivals_) {
        if (fest.id == today_festival_id_) {
            return &fest;
        }
    }
    return nullptr;
}

// ============================================================================
// 【GetUpcomingFestivals】获取即将到来的节日
// ============================================================================
std::vector<const Festival*> FestivalSystem::GetUpcomingFestivals(int max_count) const {
    std::vector<const Festival*> upcoming;

    for (const auto& fest : festivals_) {
        if (IsNoticeVisible(fest.id)) {
            upcoming.push_back(&fest);
        }
    }

    if (static_cast<int>(upcoming.size()) > max_count) {
        upcoming.resize(max_count);
    }

    return upcoming;
}

// ============================================================================
// 【GetNoticeText】获取节日预告文本
// ============================================================================
std::string FestivalSystem::GetNoticeText() const {
    std::ostringstream oss;
    bool first = true;
    for (const auto* fest : GetUpcomingFestivals()) {
        if (!first) {
            oss << "、";
        }
        oss << fest->name;
        first = false;
    }

    const std::string text = oss.str();
    if (!text.empty()) {
        return "即将到来：" + text;
    }

    return "";
}

// ============================================================================
// 【GetFestival】获取指定节日
// ============================================================================
const Festival* FestivalSystem::GetFestival(const std::string& id) const {
    for (const auto& fest : festivals_) {
        if (fest.id == id) {
            return &fest;
        }
    }
    return nullptr;
}

// ============================================================================
// 【CreateDefaultFestivals_】创建默认节日数据
// ============================================================================
std::vector<Festival> FestivalSystem::CreateDefaultFestivals_() const {
    std::vector<Festival> fests;

    // ========================================================================
    // 春季节日
    // ========================================================================
    fests.push_back({"spring_awakening", "山庄苏醒祭", "庆祝山庄苏醒", FestivalType::Normal, 0, 8, 3, "祭祀", "木材x5"});
    fests.push_back({"spring_tea_bud", "灵茶初芽节", "庆祝灵茶发芽", FestivalType::Normal, 0, 15, 3, "采茶比赛", "灵茶x3"});
    fests.push_back({"spring_date", "春风约会日", "春季约会活动", FestivalType::Normal, 0, 22, 3, "约会", "好感+20"});

    // ========================================================================
    // 夏季节日
    // ========================================================================
    fests.push_back({"summer_lantern", "云海灯会", "云海上的灯会", FestivalType::Normal, 1, 12, 3, "放灯", "灯芯x5"});
    fests.push_back({"summer_beast_sports", "灵兽运动会", "灵兽们的运动比赛", FestivalType::Normal, 1, 19, 3, "比赛", "灵茶x5"});
    fests.push_back({"summer_hot_spring", "夏夜温泉祭", "温泉开放夜", FestivalType::Normal, 1, 26, 3, "温泉", "体力+50"});

    // ========================================================================
    // 秋季节日
    // ========================================================================
    fests.push_back({"autumn_harvest", "丰收宴", "庆祝丰收", FestivalType::Normal, 2, 7, 3, "宴席", "作物x10"});
    fests.push_back({"autumn_beast_contest", "灵兽大比", "灵兽展示大赛", FestivalType::Normal, 2, 14, 3, "比试", "灵茶x5"});
    fests.push_back({"autumn_memory", "秋忆祭", "回忆与感恩", FestivalType::Normal, 2, 23, 3, "祭祀", "好感+20"});

    // ========================================================================
    // 冬季节日
    // ========================================================================
    fests.push_back({"winter_guard", "雪雾守岁", "守岁迎新", FestivalType::Normal, 3, 15, 3, "守岁", "体力+100"});
    fests.push_back({"winter_grand", "年终云海大典", "年终盛大庆典", FestivalType::Normal, 3, 28, 3, "庆典", "灵茶x10"});

    // ========================================================================
    // 云海大潮（年度特殊）
    // ========================================================================
    fests.push_back({"cloud_tide", "云海大潮", "云海最盛大的时刻", FestivalType::CloudTide, 1, 21, 7, "潮祭", "灵气+50"});

    return fests;
}

bool FestivalSystem::LoadFromCsv_(const std::string& csv_path) {
    std::ifstream in(csv_path);
    if (!in.is_open()) {
        return false;
    }

    std::vector<Festival> loaded;
    std::string line;
    bool header_loaded = false;
    std::vector<std::string> header;
    while (std::getline(in, line)) {
        const std::string trimmed = Trim_(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const auto cols = SplitCsvLine_(trimmed);
        if (!header_loaded) {
            header_loaded = true;
            if (!cols.empty() && Trim_(cols[0]) == "id") {
                header.reserve(cols.size());
                for (const auto& col : cols) {
                    header.push_back(Trim_(col));
                }
                continue;
            }
        }

        auto column = [&](std::size_t index, const std::string& name) -> std::string {
            if (!header.empty()) {
                for (std::size_t i = 0; i < header.size(); ++i) {
                    if (header[i] == name) {
                        return i < cols.size() ? Trim_(cols[i]) : "";
                    }
                }
            }
            return index < cols.size() ? Trim_(cols[index]) : "";
        };

        if (cols.size() < 5) {
            continue;
        }

        Festival fest{};
        fest.id = column(0, "id");
        fest.name = column(1, "name");
        fest.season = SeasonFromText_(column(2, "season"));
        try {
            fest.day = std::clamp(std::stoi(column(3, "day")), 1, 28);
        } catch (...) {
            continue;
        }
        fest.description = column(4, "description");
        fest.activity = column(5, "activity");
        if (fest.activity.empty()) {
            fest.activity = "节日活动";
        }
        const std::string reward_type = column(6, "reward_type");
        const std::string reward_value = column(7, "reward_value");
        fest.reward = reward_type.empty() ? reward_value : reward_type + ":" + reward_value;
        const std::string notice_days_text = column(8, "notice_days");
        if (notice_days_text.empty()) {
            fest.notice_days = (fest.id == "cloud_tide_ritual" || fest.id == "cloud_tide") ? 7 : 3;
        } else {
            try {
                fest.notice_days = std::max(1, std::stoi(notice_days_text));
            } catch (...) {
                fest.notice_days = 3;
            }
        }
        const std::string type_text = column(9, "type");
        fest.type = (type_text == "cloud_tide" || fest.id == "cloud_tide_ritual" || fest.id == "cloud_tide")
            ? FestivalType::CloudTide
            : FestivalType::Normal;
        fest.participated = false;

        if (fest.id.empty() || fest.name.empty()) {
            continue;
        }
        loaded.push_back(std::move(fest));
    }

    if (loaded.empty()) {
        return false;
    }
    festivals_ = std::move(loaded);
    return true;
}

std::vector<std::string> FestivalSystem::SplitCsvLine_(const std::string& line) {
    std::vector<std::string> out;
    std::string current;
    bool in_quotes = false;
    for (char ch : line) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (ch == ',' && !in_quotes) {
            out.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    out.push_back(current);
    return out;
}

int FestivalSystem::SeasonFromText_(const std::string& text) {
    std::string lower;
    lower.reserve(text.size());
    for (unsigned char ch : text) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    if (lower == "spring") return 0;
    if (lower == "summer") return 1;
    if (lower == "autumn" || lower == "fall") return 2;
    if (lower == "winter") return 3;
    return 0;
}

std::string FestivalSystem::Trim_(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

}  // namespace CloudSeamanor::domain
