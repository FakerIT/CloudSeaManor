#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/infrastructure/DialogueJsonParser.hpp"

#include <fstream>
#include <random>
#include <sstream>
#include <unordered_set>
#include <algorithm>

namespace CloudSeamanor::engine {

namespace {

// ============================================================================
// 【枚举解析工具】
// ============================================================================
std::optional<Season> ParseSeason(const std::string& s) {
    if (s.empty()) return std::nullopt;
    if (s == "春" || s == "Spring") return Season::Spring;
    if (s == "夏" || s == "Summer") return Season::Summer;
    if (s == "秋" || s == "Autumn") return Season::Autumn;
    if (s == "冬" || s == "Winter") return Season::Winter;
    return std::nullopt;
}

std::optional<DayPhase> ParseDayPhase(const std::string& s) {
    if (s.empty()) return std::nullopt;
    if (s == "清晨" || s == "Morning") return DayPhase::Morning;
    if (s == "午后" || s == "Afternoon") return DayPhase::Afternoon;
    if (s == "傍晚" || s == "Evening") return DayPhase::Evening;
    if (s == "夜晚" || s == "Night") return DayPhase::Night;
    return std::nullopt;
}

std::optional<NpcWeather> ParseNpcWeather(const std::string& s) {
    if (s.empty()) return std::nullopt;
    if (s == "晴" || s == "Clear") return NpcWeather::Clear;
    if (s == "薄雾" || s == "Mist") return NpcWeather::Mist;
    if (s == "浓云" || s == "DenseCloud") return NpcWeather::DenseCloud;
    if (s == "大潮" || s == "Tide") return NpcWeather::Tide;
    return std::nullopt;
}

// ============================================================================
// 【解析器工具函数】
// ============================================================================

// 移除字符串首尾空白
std::string Trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// 解析条件表达式字符串（如 "favor>=4,season=春,weather=浓云"）
// 支持格式：key=value, key>=value, key<=value
bool ParseCondition(const std::string& cond, const NpcDialogueContext& ctx, bool& out_result) {
    if (cond.empty() || cond == "*") {
        out_result = true;
        return true;
    }

    std::istringstream ss(cond);
    std::string token;
    bool all_match = true;

    while (std::getline(ss, token, ',')) {
        token = Trim(token);
        if (token.empty()) continue;

        bool matched = false;
        auto eq_pos = token.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = Trim(token.substr(0, eq_pos));
            std::string val = Trim(token.substr(eq_pos + 1));

            if (key == "season") {
                matched = (ctx.current_season
                    && CloudSeamanor::domain::GameClock::SeasonName(*ctx.current_season) == val);
            } else if (key == "weather") {
                matched = (ctx.current_weather && NpcWeatherName(*ctx.current_weather) == val);
            } else if (key == "time") {
                if (ctx.current_time_of_day) {
                    std::string phase_text;
                    switch (*ctx.current_time_of_day) {
                    case DayPhase::Morning: phase_text = "清晨"; break;
                    case DayPhase::Afternoon: phase_text = "午后"; break;
                    case DayPhase::Evening: phase_text = "傍晚"; break;
                    case DayPhase::Night: phase_text = "夜晚"; break;
                    }
                    matched = (phase_text == val);
                }
            } else if (key == "location") {
                matched = (ctx.current_location == val);
            } else if (key == "activity") {
                matched = (ctx.current_activity == val);
            }
        }

        auto ge_pos = token.find(">=");
        if (ge_pos != std::string::npos) {
            std::string key = Trim(token.substr(0, ge_pos));
            std::string val = Trim(token.substr(ge_pos + 2));
            int threshold = std::stoi(val);
            if (key == "favor") matched = (ctx.player_favor >= threshold);
            else if (key == "heart") matched = (ctx.npc_heart_level >= threshold);
            else if (key == "day") matched = (ctx.current_day >= threshold);
        }

        auto le_pos = token.find("<=");
        if (le_pos != std::string::npos) {
            std::string key = Trim(token.substr(0, le_pos));
            std::string val = Trim(token.substr(le_pos + 2));
            int threshold = std::stoi(val);
            if (key == "favor") matched = (ctx.player_favor <= threshold);
            else if (key == "heart") matched = (ctx.npc_heart_level <= threshold);
        }

        auto gt_pos = token.find('>');
        if (gt_pos != std::string::npos && token.find(">=") == std::string::npos) {
            std::string key = Trim(token.substr(0, gt_pos));
            std::string val = Trim(token.substr(gt_pos + 1));
            int threshold = std::stoi(val);
            if (key == "favor") matched = (ctx.player_favor > threshold);
            else if (key == "heart") matched = (ctx.npc_heart_level > threshold);
        }

        if (!matched) {
            all_match = false;
            break;
        }
    }

    out_result = all_match;
    return true;
}


// 解析日常对话 JSON
bool ParseDailyDialogueJson(const std::string& json,
                           const std::string& npc_id,
                           DailyDialoguePool& out_pool) {
    out_pool.npc_id = npc_id;

    auto find_section = [](const std::string& s, const std::string& key) -> std::string::size_type {
        return s.find("\"" + key + "\"");
    };

    auto extract_array = [](const std::string& s, std::string::size_type pos)
        -> std::pair<std::string, std::string::size_type> {
        auto start = s.find('[', pos);
        auto end = s.find(']', start);
        if (start == std::string::npos || end == std::string::npos) return {"", std::string::npos};
        return {s.substr(start + 1, end - start - 1), end + 1};
    };

    auto extract_string_field = [](const std::string& obj,
                                  const std::string& field) -> std::string {
        auto pos = obj.find("\"" + field + "\"");
        if (pos == std::string::npos) return "";
        auto start = obj.find('"', pos + field.size() + 2);
        auto end = obj.find('"', start + 1);
        if (start == std::string::npos || end == std::string::npos) return "";
        return obj.substr(start + 1, end - start - 1);
    };

    auto extract_int_field = [](const std::string& obj,
                                const std::string& field,
                                int default_val) -> int {
        auto pos = obj.find("\"" + field + "\"");
        if (pos == std::string::npos) return default_val;
        auto start = pos + field.size() + 3;
        while (start < obj.size() && (obj[start] == ' ' || obj[start] == ':')) ++start;
        auto end = start;
        while (end < obj.size() && std::isdigit(static_cast<unsigned char>(obj[end]))) ++end;
        if (end == start) return default_val;
        return std::stoi(obj.substr(start, end - start));
    };

    auto parse_entries = [&](const std::string& arr_str,
                            std::vector<DailyDialogueEntry>& out_entries) {
        std::string::size_type item_pos = 0;
        while (item_pos < arr_str.size()) {
            while (item_pos < arr_str.size()
                   && std::isspace(static_cast<unsigned char>(arr_str[item_pos]))) ++item_pos;
            if (item_pos >= arr_str.size() || arr_str[item_pos] != '{') break;

            int braces = 1;
            std::string::size_type item_end = item_pos + 1;
            while (item_end < arr_str.size() && braces > 0) {
                if (arr_str[item_end] == '{') ++braces;
                else if (arr_str[item_end] == '}') --braces;
                ++item_end;
            }

            std::string item = arr_str.substr(item_pos, item_end - item_pos);
            DailyDialogueEntry entry;
            entry.id = extract_string_field(item, "id");
            entry.text = extract_string_field(item, "text");
            entry.favor_min = extract_int_field(item, "favor_min", 0);
            entry.favor_max = extract_int_field(item, "favor_max", 999);
            entry.season = ParseSeason(extract_string_field(item, "season"));
            entry.time_of_day = ParseDayPhase(extract_string_field(item, "time_of_day"));
            entry.weather = ParseNpcWeather(extract_string_field(item, "weather"));
            entry.heart_min = extract_int_field(item, "heart_min", 0);
            entry.heart_max = extract_int_field(item, "heart_max", 10);

            if (!entry.id.empty() && !entry.text.empty()) {
                out_entries.push_back(entry);
            }
            item_pos = item_end;
        }
    };

    for (const std::string& section : {"greetings", "small_talks", "farewells"}) {
        auto sec_pos = find_section(json, section);
        if (sec_pos == std::string::npos) continue;

        auto [arr_content, _e] = extract_array(json, sec_pos);
        if (section == "greetings") {
            parse_entries(arr_content, out_pool.greetings);
        } else if (section == "small_talks") {
            parse_entries(arr_content, out_pool.small_talks);
        } else if (section == "farewells") {
            parse_entries(arr_content, out_pool.farewells);
        }
    }

    return !out_pool.greetings.empty() || !out_pool.small_talks.empty();
}

// 云海状态到天气字符串
std::string CloudStateToWeatherString(domain::CloudState state) {
    switch (state) {
    case domain::CloudState::Clear:      return "晴";
    case domain::CloudState::Mist:       return "薄雾";
    case domain::CloudState::DenseCloud: return "浓云";
    case domain::CloudState::Tide:       return "大潮";
    }
    return "晴";
}

// 时段索引到字符串
std::string TimeOfDayToString(int hour) {
    if (hour >= 6 && hour < 12) return "清晨";
    if (hour >= 12 && hour < 18) return "午后";
    if (hour >= 18 && hour < 22) return "傍晚";
    return "夜晚";
}

// 季节索引到字符串
std::string SeasonIndexToString(int idx) {
    switch (idx) {
    case 0: return "春";
    case 1: return "夏";
    case 2: return "秋";
    case 3: return "冬";
    }
    return "春";
}

} // anonymous namespace

namespace {
void LoadStageDialoguesFromCsv_(
    const std::string& path,
    std::unordered_map<std::string, std::vector<StageDialogueEntry>>& out_map) {
    out_map.clear();
    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("NpcId,", 0) == 0) {
            continue;
        }
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            fields.push_back(part);
        }
        if (fields.size() < 6 || fields[0].empty()) {
            continue;
        }
        StageDialogueEntry entry;
        entry.stage = std::max(0, std::atoi(fields[1].c_str()));
        entry.priority = std::max(0, std::atoi(fields[3].c_str()));
        entry.text = fields[5];
        if (entry.text.empty()) {
            continue;
        }
        out_map[fields[0]].push_back(std::move(entry));
    }
    for (auto& [_, rows] : out_map) {
        std::sort(rows.begin(), rows.end(), [](const StageDialogueEntry& a, const StageDialogueEntry& b) {
            return a.priority > b.priority;
        });
    }
}
} // namespace

// ============================================================================
// 【NpcDialogueManager】构造函数
// ============================================================================
NpcDialogueManager::NpcDialogueManager(const std::string& data_root)
    : data_root_(data_root) {
}

// ============================================================================
// 【SetCallbacks】设置回调
// ============================================================================
void NpcDialogueManager::SetCallbacks(NpcDialogueManagerCallbacks callbacks) {
    callbacks_ = std::move(callbacks);
}

// ============================================================================
// 【SetNpcList】设置 NPC 列表引用
// ============================================================================
void NpcDialogueManager::SetNpcList(const std::vector<NpcActor>* npcs) {
    npc_list_ = npcs;
}

// ============================================================================
// 【LoadDailyDialogue】加载日常对话池
// ============================================================================
bool NpcDialogueManager::LoadDailyDialogue(const std::string& npc_id) {
    if (loaded_npc_ids_.contains(npc_id)) {
        return true;  // 已加载
    }

    std::string path = data_root_ + "/daily_dialogue/npc_daily_" + npc_id + ".json";
    std::ifstream file(path);
    if (!file.is_open()) {
        DailyDialoguePool pool;
        pool.npc_id = npc_id;
        pool.greetings.push_back(DailyDialogueEntry{
            npc_id + "_greet", "你好，$[PLAYER_NAME]。今天也辛苦了。"
        });
        pool.small_talks.push_back(DailyDialogueEntry{
            npc_id + "_talk", "山庄最近越来越热闹了。"
        });
        pool.farewells.push_back(DailyDialogueEntry{
            npc_id + "_bye", "路上小心，明天见。"
        });
        daily_pools_[npc_id] = std::move(pool);
        loaded_npc_ids_.insert(npc_id);
        infrastructure::Logger::Warning("NpcDialogueManager: 缺失日常对话，使用兜底池：" + path);
        return true;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    DailyDialoguePool pool;
    if (!ParseDailyDialogueJson(content, npc_id, pool)) {
        infrastructure::Logger::Warning("NpcDialogueManager: 解析日常对话失败：" + path);
        return false;
    }

    daily_pools_[npc_id] = pool;
    loaded_npc_ids_.insert(npc_id);
    infrastructure::Logger::Info("NpcDialogueManager: 已加载 " + npc_id +
                                 " 日常对话（问候:" + std::to_string(pool.greetings.size()) +
                                 " 闲聊:" + std::to_string(pool.small_talks.size()) +
                                 " 告别:" + std::to_string(pool.farewells.size()) + "）。");
    return true;
}

// ============================================================================
// 【LoadHeartEventDialogue】加载心事件对话树
// ============================================================================
std::vector<DialogueNode> NpcDialogueManager::LoadHeartEventDialogue(
    const std::string& npc_id, int heart_level) {
    std::string key = npc_id + "_h" + std::to_string(heart_level);
    if (heart_event_trees_.contains(key)) {
        return heart_event_trees_[key];
    }

    std::string path = data_root_ + "/dialogue/npc_heart_" + npc_id +
                       "_h" + std::to_string(heart_level) + ".json";
    std::ifstream file(path);
    if (!file.is_open()) {
        // C-2: 对缺失文件提供运行时兜底，保证完整心事件链可执行。
        DialogueNode start;
        start.id = npc_id + "_h" + std::to_string(heart_level) + "_start";
        start.speaker = npc_id;
        start.text = "这是 " + npc_id + " 的 " + std::to_string(heart_level) + " 心事件（占位剧情）。";
        DialogueChoice c;
        c.id = "ok";
        c.text = "继续";
        c.next_node_id = npc_id + "_h" + std::to_string(heart_level) + "_end";
        start.choices.push_back(c);
        DialogueNode end;
        end.id = c.next_node_id;
        end.speaker = npc_id;
        end.text = "剧情暂未写完，但关系推进已生效。";
        std::vector<DialogueNode> fallback{start, end};
        heart_event_trees_[key] = fallback;
        infrastructure::Logger::Warning("NpcDialogueManager: 心事件文件缺失，使用兜底剧情：" + path);
        return fallback;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    auto result = LoadDialogueFromJson(content);
    if (!result.success) {
        infrastructure::Logger::Warning("NpcDialogueManager: 解析心事件对话失败：" + path);
        return {};
    }

    heart_event_trees_[key] = result.nodes;
    infrastructure::Logger::Info("NpcDialogueManager: 已加载 " + npc_id +
                                 " " + std::to_string(heart_level) + "心事件对话（" +
                                 std::to_string(result.nodes.size()) + " 节点）。");
    return result.nodes;
}

// ============================================================================
// 【MarkHeartEventComplete】标记心事件为已完成
// ============================================================================
void NpcDialogueManager::MarkHeartEventComplete(const std::string& npc_id,
                                                const std::string& event_id) {
    std::string key = HeartEventKey_(npc_id, event_id);
    heart_event_completed_[key] = true;
    infrastructure::Logger::LogNpcHeartEvent(
        "NpcDialogueManager: completed " + npc_id + " / " + event_id);
}

// ============================================================================
// 【SaveState】保存心事件完成状态
// ============================================================================
void NpcDialogueManager::SaveState(std::vector<std::string>& lines) const {
    for (const auto& [key, completed] : heart_event_completed_) {
        if (completed) {
            lines.push_back("heart_event|" + key);
        }
    }
}

// ============================================================================
// 【LoadState】加载心事件完成状态
// ============================================================================
void NpcDialogueManager::LoadState(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        if (line.rfind("heart_event|", 0) == 0) {
            std::string key = line.substr(13);
            heart_event_completed_[key] = true;
        }
    }
}

// ============================================================================
// 【LoadDialogueFromJson】从 JSON 文本加载对话树
// ============================================================================
DialogueLoadResult NpcDialogueManager::LoadDialogueFromJson(const std::string& json_content) {
    DialogueLoadResult result;
    result.nodes = CloudSeamanor::infrastructure::ConvertDialogueNodes(
        CloudSeamanor::infrastructure::ParseDialogueNodes(json_content));
    if (result.nodes.empty()) {
        result.error_message = "未找到对话节点";
        return result;
    }
    result.success = true;
    return result;
}

// ============================================================================
// 【InitializeHeartEvents】初始化心事件表
// ============================================================================
void NpcDialogueManager::InitializeHeartEvents() {
    // 阿茶（acha）的 5 个心事件
    npc_heart_events_["acha"] = {
        HeartEventEntry{"acha_h1", "春季清晨进茶园", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h1.json",
                        50, 1, false, Season::Spring, std::nullopt, DayPhase::Morning, false, 10, ""},
        HeartEventEntry{"acha_h2", "春季白天进茶园", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h2.json",
                        200, 2, false, Season::Spring, std::nullopt, std::nullopt, false, 20, ""},
        HeartEventEntry{"acha_h3", "夏季傍晚在茶田", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h3.json",
                        600, 3, false, Season::Summer, std::nullopt, DayPhase::Evening, false, 25, ""},
        HeartEventEntry{"acha_h4", "夏季雨天去亭子", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h4.json",
                        200, 4, false, Season::Summer, std::nullopt, std::nullopt, false, 30, ""},
        HeartEventEntry{"acha_h5", "夏末傍晚在茶田", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h5.json",
                        300, 5, false, Season::Summer, std::nullopt, DayPhase::Evening, false, 40, ""},
        HeartEventEntry{"acha_h6", "秋季浓云海夜间", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h6.json",
                        400, 6, false, Season::Autumn, NpcWeather::DenseCloud, DayPhase::Evening, false, 50, ""},
        HeartEventEntry{"acha_h7", "秋季午后在茶田", "farm", "TeaField",
                        "assets/data/dialogue/npc_heart_acha_h7.json",
                        550, 7, false, Season::Autumn, std::nullopt, DayPhase::Afternoon, false, 55, ""},
        HeartEventEntry{"acha_h8", "冬季雪天", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_acha_h8.json",
                        700, 8, false, Season::Winter, std::nullopt, std::nullopt, false, 60, ""},
        HeartEventEntry{"acha_h10", "大潮日回主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_acha_h10.json",
                        1000, 10, false, std::nullopt, NpcWeather::Tide, std::nullopt, false, 80, ""},
    };

    // 小满（xiaoman）的 5 个心事件
    npc_heart_events_["xiaoman"] = {
        HeartEventEntry{"xiaoman_h1", "春季白天在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h1.json",
                        50, 1, false, Season::Spring, std::nullopt, std::nullopt, false, 10, ""},
        HeartEventEntry{"xiaoman_h2", "春季白天在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h2.json",
                        200, 2, false, Season::Spring, std::nullopt, std::nullopt, false, 20, ""},
        HeartEventEntry{"xiaoman_h3", "夏季午后在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h3.json",
                        600, 3, false, Season::Summer, std::nullopt, DayPhase::Afternoon, false, 25, ""},
        HeartEventEntry{"xiaoman_h4", "夏季雨天在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h4.json",
                        200, 4, false, Season::Summer, std::nullopt, std::nullopt, false, 30, ""},
        HeartEventEntry{"xiaoman_h5", "夏季傍晚在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h5.json",
                        300, 5, false, Season::Summer, std::nullopt, DayPhase::Evening, false, 40, ""},
        HeartEventEntry{"xiaoman_h6", "秋季浓云海在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h6.json",
                        400, 6, false, Season::Autumn, NpcWeather::DenseCloud, DayPhase::Afternoon, false, 50, ""},
        HeartEventEntry{"xiaoman_h7", "秋季夜晚在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h7.json",
                        550, 7, false, Season::Autumn, std::nullopt, DayPhase::Night, false, 55, ""},
        HeartEventEntry{"xiaoman_h8", "冬季在灵境花园", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h8.json",
                        700, 8, false, Season::Winter, std::nullopt, std::nullopt, false, 60, ""},
        HeartEventEntry{"xiaoman_h10", "任意季节大潮日", "farm", "SpiritGarden",
                        "assets/data/dialogue/npc_heart_xiaoman_h10.json",
                        1000, 10, false, std::nullopt, NpcWeather::Tide, std::nullopt, false, 80, "beast_work_stamina_half"},
    };

    // 晚星（wanxing）的 5 个心事件
    npc_heart_events_["wanxing"] = {
        HeartEventEntry{"wanxing_h1", "春季夜晚在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h1.json",
                        50, 1, false, Season::Spring, std::nullopt, DayPhase::Night, false, 10, ""},
        HeartEventEntry{"wanxing_h2", "春季白天在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h2.json",
                        200, 2, false, Season::Spring, std::nullopt, std::nullopt, false, 20, ""},
        HeartEventEntry{"wanxing_h3", "秋季夜晚在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h3.json",
                        600, 3, false, Season::Autumn, std::nullopt, DayPhase::Night, false, 25, ""},
        HeartEventEntry{"wanxing_h4", "夏季在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h4.json",
                        200, 4, false, std::nullopt, std::nullopt, std::nullopt, false, 30, ""},
        HeartEventEntry{"wanxing_h5", "夏季夜晚观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h5.json",
                        300, 5, false, Season::Summer, std::nullopt, DayPhase::Night, false, 40, ""},
        HeartEventEntry{"wanxing_h6", "秋季浓云海夜间在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h6.json",
                        400, 6, false, Season::Autumn, NpcWeather::DenseCloud, DayPhase::Night, false, 50, ""},
        HeartEventEntry{"wanxing_h7", "秋季傍晚在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h7.json",
                        550, 7, false, Season::Autumn, std::nullopt, DayPhase::Evening, false, 55, ""},
        HeartEventEntry{"wanxing_h8", "冬季大潮日在观星台", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h8.json",
                        700, 8, false, Season::Winter, NpcWeather::Tide, std::nullopt, false, 60, ""},
        HeartEventEntry{"wanxing_h10", "任意大潮日", "farm", "Observatory",
                        "assets/data/dialogue/npc_heart_wanxing_h10.json",
                        1000, 10, false, std::nullopt, NpcWeather::Tide, std::nullopt, false, 80, ""},
    };

    // 林伯（lin）的 5 个心事件
    npc_heart_events_["lin"] = {
        HeartEventEntry{"lin_h1", "春季午后在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h1.json",
                        50, 1, false, Season::Spring, std::nullopt, DayPhase::Afternoon, false, 10, ""},
        HeartEventEntry{"lin_h2", "春季白天在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h2.json",
                        200, 2, false, Season::Spring, std::nullopt, std::nullopt, false, 20, ""},
        HeartEventEntry{"lin_h3", "秋季傍晚在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h3.json",
                        600, 3, false, Season::Autumn, std::nullopt, DayPhase::Evening, false, 25, ""},
        HeartEventEntry{"lin_h4", "夏季在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h4.json",
                        200, 4, false, Season::Summer, std::nullopt, std::nullopt, false, 30, ""},
        HeartEventEntry{"lin_h5", "夏季午后在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h5.json",
                        300, 5, false, Season::Summer, std::nullopt, DayPhase::Afternoon, false, 40, ""},
        HeartEventEntry{"lin_h6", "秋季在码头", "farm", "Dock",
                        "assets/data/dialogue/npc_heart_lin_h6.json",
                        400, 6, false, Season::Autumn, std::nullopt, DayPhase::Afternoon, false, 50, ""},
        HeartEventEntry{"lin_h7", "秋季夜晚在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h7.json",
                        550, 7, false, Season::Autumn, std::nullopt, DayPhase::Night, false, 55, ""},
        HeartEventEntry{"lin_h8", "冬季在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h8.json",
                        700, 8, false, Season::Winter, std::nullopt, std::nullopt, false, 60, ""},
        HeartEventEntry{"lin_h10", "任意大潮日在主屋", "farm", "MainHouse",
                        "assets/data/dialogue/npc_heart_lin_h10.json",
                        1000, 10, false, std::nullopt, NpcWeather::Tide, std::nullopt, false, 80, ""},
    };

    // C-2: 扩展 NPC（9 位）接入 h1~h8 占位链。
    const std::vector<std::string> extra_npcs{
        "song", "yu", "mo", "qiao", "he", "ning", "an", "shu", "yan"
    };
    for (const auto& id : extra_npcs) {
        std::vector<HeartEventEntry> entries;
        for (int h = 1; h <= 8; ++h) {
            entries.push_back(HeartEventEntry{
                id + "_h" + std::to_string(h),
                "扩展 NPC 心事件",
                "farm",
                "VillageCenter",
                "assets/data/dialogue/npc_heart_" + id + "_h" + std::to_string(h) + ".json",
                h * 120,
                h,
                false,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                false,
                8 + h,
                ""
            });
        }
        npc_heart_events_[id] = std::move(entries);
    }

    infrastructure::Logger::Info("NpcDialogueManager: 已注册 " +
                                 std::to_string(npc_heart_events_.size()) +
                                 " 个 NPC 的心事件表。");
}

// ============================================================================
// 【MatchesCondition_】检查条件匹配
// ============================================================================
bool NpcDialogueManager::MatchesCondition_(const DailyDialogueEntry& entry,
                                          const NpcDialogueContext& ctx) const {
    // 好感度范围
    if (ctx.player_favor < entry.favor_min || ctx.player_favor > entry.favor_max) {
        return false;
    }

    // 心级范围
    if (ctx.npc_heart_level < entry.heart_min || ctx.npc_heart_level > entry.heart_max) {
        return false;
    }

    // 季节
    if (entry.season && ctx.current_season && *entry.season != *ctx.current_season) {
        return false;
    }

    // 时段
    if (entry.time_of_day && ctx.current_time_of_day && *entry.time_of_day != *ctx.current_time_of_day) {
        return false;
    }

    // 天气
    if (entry.weather && ctx.current_weather && *entry.weather != *ctx.current_weather) {
        return false;
    }

    // 婚后条件
    if (entry.is_spouse_only) {
        // 必须是当前玩家的配偶才能看到
        if (!ctx.is_married || ctx.spouse_id != ctx.npc_id) {
            return false;
        }
    } else if (entry.is_married && !ctx.is_married) {
        // 婚后专属对话，但玩家未婚
        return false;
    }

    return true;
}

// ============================================================================
// 【SelectMatchingEntry_】从池中选择匹配条目
// ============================================================================
std::optional<std::size_t> NpcDialogueManager::SelectMatchingEntry_(
    const std::vector<DailyDialogueEntry>& pool,
    const NpcDialogueContext& ctx) const {
    const auto favor_distance_score = [&](const DailyDialogueEntry& entry) {
        if (ctx.player_favor < entry.favor_min) return (entry.favor_min - ctx.player_favor);
        if (ctx.player_favor > entry.favor_max) return (ctx.player_favor - entry.favor_max);
        return 0;
    };
    const auto heart_distance_score = [&](const DailyDialogueEntry& entry) {
        if (ctx.npc_heart_level < entry.heart_min) return (entry.heart_min - ctx.npc_heart_level);
        if (ctx.npc_heart_level > entry.heart_max) return (ctx.npc_heart_level - entry.heart_max);
        return 0;
    };

    // 收集匹配条目并按“条件特异性”评分，优先天气/时段/季节等更具体的条目。
    std::vector<std::pair<std::size_t, int>> candidates;
    for (std::size_t i = 0; i < pool.size(); ++i) {
        if (MatchesCondition_(pool[i], ctx)) {
            int score = 0;
            if (pool[i].weather) score += 4;
            if (pool[i].time_of_day) score += 3;
            if (pool[i].season) score += 2;
            if (pool[i].heart_min > 0) score += 1;
            // 配偶专属对话优先级最高
            if (pool[i].is_spouse_only) score += 10;
            // 婚后对话优先级较高
            else if (pool[i].is_married) score += 5;
            // 好感/心级越贴合当前阶段，越优先。
            score += std::max(0, 5 - favor_distance_score(pool[i]));
            score += std::max(0, 3 - heart_distance_score(pool[i]));
            candidates.emplace_back(i, score);
        }
    }

    if (candidates.empty()) {
        // 放宽匹配：先按季节/时段/天气匹配，再按好感/心级距离选择最近条目。
        std::vector<std::pair<std::size_t, int>> relaxed;
        for (std::size_t i = 0; i < pool.size(); ++i) {
            bool season_ok = !pool[i].season || !ctx.current_season || *pool[i].season == *ctx.current_season;
            bool time_ok = !pool[i].time_of_day || !ctx.current_time_of_day || *pool[i].time_of_day == *ctx.current_time_of_day;
            bool weather_ok = !pool[i].weather || !ctx.current_weather || *pool[i].weather == *ctx.current_weather;
            if (!season_ok || !time_ok || !weather_ok) {
                continue;
            }
            const int distance = favor_distance_score(pool[i]) * 2 + heart_distance_score(pool[i]);
            relaxed.emplace_back(i, distance);
        }
        if (relaxed.empty()) {
            return std::nullopt;
        }
        int best_distance = relaxed.front().second;
        for (const auto& [_, distance] : relaxed) {
            if (distance < best_distance) {
                best_distance = distance;
            }
        }
        std::vector<std::size_t> best_indices;
        for (const auto& [index, distance] : relaxed) {
            if (distance == best_distance) {
                best_indices.push_back(index);
            }
        }
        if (best_indices.empty()) {
            return std::nullopt;
        }
        std::uniform_int_distribution<std::size_t> dist(0, best_indices.size() - 1);
        return best_indices[dist(Rng())];
    }

    int max_score = candidates.front().second;
    for (const auto& [_, score] : candidates) {
        if (score > max_score) {
            max_score = score;
        }
    }
    std::vector<std::size_t> best_indices;
    for (const auto& [index, score] : candidates) {
        if (score == max_score) {
            best_indices.push_back(index);
        }
    }
    if (best_indices.empty()) {
        std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
        return candidates[dist(Rng())].first;
    }
    std::uniform_int_distribution<std::size_t> dist(0, best_indices.size() - 1);
    return best_indices[dist(Rng())];
}

// ============================================================================
// 【DialogueEntryToNode】将日常对话条目转换为 DialogueNode
// ============================================================================
DialogueNode NpcDialogueManager::DialogueEntryToNode(
    const DailyDialogueEntry& entry,
    const NpcDialogueContext& ctx) const {
    DialogueNode node;
    node.id = entry.id;
    node.speaker = ctx.npc_name;
    std::string text = ReplaceExtendedTokens(entry.text, ctx);
    switch (ctx.npc_mood) {
    case NpcMood::Happy:
        text = "（心情不错）" + text;
        break;
    case NpcMood::Sad:
        text = "（有些低落）" + text;
        break;
    case NpcMood::Angry:
        text = "（有点不耐烦）" + text;
        break;
    case NpcMood::Normal:
    default:
        break;
    }
    node.text = std::move(text);
    return node;
}

// ============================================================================
// 【ReplaceExtendedTokens】扩展文本替换
// ============================================================================
std::string NpcDialogueManager::ReplaceExtendedTokens(
    const std::string& raw,
    const NpcDialogueContext& ctx) const {
    std::string result = raw;

    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        std::string::size_type pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(result, "$[PLAYER_NAME]", ctx.player_name);
    replace_all(result, "$[FARM_NAME]", ctx.farm_name);
    replace_all(result, "$[SEASON]",
                ctx.current_season ? CloudSeamanor::domain::GameClock::SeasonName(*ctx.current_season) : "");
    replace_all(result, "$[TIME]",
                ctx.current_time_of_day
                    ? (*ctx.current_time_of_day == DayPhase::Morning ? "清晨"
                        : *ctx.current_time_of_day == DayPhase::Afternoon ? "午后"
                        : *ctx.current_time_of_day == DayPhase::Evening ? "傍晚"
                        : "夜晚")
                    : "");
    replace_all(result, "$[WEATHER]",
                ctx.current_weather ? NpcWeatherName(*ctx.current_weather) : "");
    replace_all(result, "$[NPC_NAME]", ctx.npc_name);
    replace_all(result, "$[LOCATION]", ctx.current_location);
    replace_all(result, "$[ACTIVITY]", ctx.current_activity);
    replace_all(result, "$[ITEM_NAME]", ctx.recent_item);
    replace_all(result, "$[SPOUSE_CALL]", ctx.spouse_call);

    // 数值变量
    auto replace_int = [&](const std::string& token, int value) {
        replace_all(result, token, std::to_string(value));
    };
    replace_int("$[FAVOR]", ctx.player_favor);
    replace_int("$[HEART]", ctx.npc_heart_level);
    replace_int("$[DAY]", ctx.current_day);
    replace_int("$[SPIRIT]", ctx.spirit_energy);
    replace_int("$[CLOUD_LEVEL]", ctx.cloud_level);

    // 布尔变量
    if (!ctx.has_item && result.find("$[IF_ITEM]") != std::string::npos) {
        // 移除 $IF_ITEM...$FI_ITEM 块
        std::string::size_type if_pos = 0;
        while ((if_pos = result.find("$[IF_ITEM]", if_pos)) != std::string::npos) {
            auto fi_pos = result.find("$[FI_ITEM]", if_pos);
            if (fi_pos != std::string::npos) {
                result.erase(if_pos, fi_pos - if_pos + 10);
            } else {
                result.erase(if_pos, 10);
                break;
            }
        }
    }

    return result;
}

// ============================================================================
// 【GetGreeting】获取问候语
// ============================================================================
std::vector<DialogueNode> NpcDialogueManager::GetGreeting(
    const std::string& npc_id,
    const NpcDialogueContext& ctx) {
    std::vector<DialogueNode> result;
    LoadDailyDialogue(npc_id);

    auto it = daily_pools_.find(npc_id);
    if (it == daily_pools_.end() || it->second.greetings.empty()) {
        return result;
    }

    const auto idx = SelectMatchingEntry_(it->second.greetings, ctx);
    if (idx) {
        result.push_back(DialogueEntryToNode(it->second.greetings[*idx], ctx));
    }
    return result;
}

// ============================================================================
// 【GetSmallTalk】获取闲聊语
// ============================================================================
std::vector<DialogueNode> NpcDialogueManager::GetSmallTalk(
    const std::string& npc_id,
    const NpcDialogueContext& ctx) {
    std::vector<DialogueNode> result;
    LoadDailyDialogue(npc_id);

    auto it = daily_pools_.find(npc_id);
    if (it == daily_pools_.end() || it->second.small_talks.empty()) {
        return result;
    }

    const auto idx = SelectMatchingEntry_(it->second.small_talks, ctx);
    if (idx) {
        result.push_back(DialogueEntryToNode(it->second.small_talks[*idx], ctx));
    }
    return result;
}

// ============================================================================
// 【GetFarewell】获取告别语
// ============================================================================
std::vector<DialogueNode> NpcDialogueManager::GetFarewell(
    const std::string& npc_id,
    const NpcDialogueContext& ctx) {
    std::vector<DialogueNode> result;
    LoadDailyDialogue(npc_id);

    auto it = daily_pools_.find(npc_id);
    if (it == daily_pools_.end() || it->second.farewells.empty()) {
        return result;
    }

    const auto idx = SelectMatchingEntry_(it->second.farewells, ctx);
    if (idx) {
        result.push_back(DialogueEntryToNode(it->second.farewells[*idx], ctx));
    }
    return result;
}

// ============================================================================
// 【Rng】线程安全的随机数生成器
// ============================================================================
std::mt19937& NpcDialogueManager::Rng() const {
    thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}
// ============================================================================
std::string NpcDialogueManager::HeartEventKey_(
    const std::string& npc_id,
    const std::string& event_id) {
    return npc_id + "|" + event_id;
}

// ============================================================================
// 【CheckHeartEventTrigger】检查心事件触发
// ============================================================================
const HeartEventEntry* NpcDialogueManager::CheckHeartEventTrigger(
    const std::string& npc_id,
    const NpcDialogueContext& ctx) {
    auto npc_it = npc_heart_events_.find(npc_id);
    if (npc_it == npc_heart_events_.end()) {
        return nullptr;
    }

    for (const auto& event : npc_it->second) {
        // 检查是否已完成（且不可重复）
        std::string key = HeartEventKey_(npc_id, event.event_id);
        if (!event.repeatable && heart_event_completed_.contains(key)) {
            continue;
        }

        // 检查好感度阈值
        if (ctx.player_favor < event.favor_threshold) continue;

        // 检查心级阈值
        if (ctx.npc_heart_level < event.heart_threshold) continue;

        // 检查季节
        if (event.season && ctx.current_season && *event.season != *ctx.current_season) continue;

        // 检查天气
        if (event.weather && ctx.current_weather && *event.weather != *ctx.current_weather) continue;

        // 检查时段
        if (event.time_of_day && ctx.current_time_of_day && *event.time_of_day != *ctx.current_time_of_day) continue;

        // 所有条件满足，触发此事件
        return &event;
    }

    return nullptr;
}

// ============================================================================
// 【CompleteHeartEvent】标记心事件完成
// ============================================================================
void NpcDialogueManager::CompleteHeartEvent(
    const std::string& npc_id,
    const std::string& event_id) {
    std::string key = HeartEventKey_(npc_id, event_id);
    heart_event_completed_[key] = true;

    // 查找事件获取奖励
    auto npc_it = npc_heart_events_.find(npc_id);
    if (npc_it != npc_heart_events_.end()) {
        for (const auto& event : npc_it->second) {
            if (event.event_id == event_id) {
                if (event.reward_favor > 0 && callbacks_.on_favor_change) {
                    // 查找 NPC 索引
                    if (npc_list_) {
                        for (std::size_t i = 0; i < npc_list_->size(); ++i) {
                            if ((*npc_list_)[i].id == npc_id) {
                                callbacks_.on_favor_change(static_cast<int>(i),
                                                          event.reward_favor);
                                break;
                            }
                        }
                    }
                }

                if (callbacks_.log_info) {
                    callbacks_.log_info("心事件完成: " + npc_id + " " + event_id +
                                       "，奖励: " + event.reward_flag);
                }
                break;
            }
        }
    }
}

// ============================================================================
// 【GetNpcHeartEvents】获取 NPC 所有心事件
// ============================================================================
std::vector<HeartEventEntry> NpcDialogueManager::GetNpcHeartEvents(
    const std::string& npc_id) const {
    auto it = npc_heart_events_.find(npc_id);
    if (it == npc_heart_events_.end()) {
        return {};
    }
    return it->second;
}

// ============================================================================
// 【SetHeartEventCompleted】设置心事件完成状态
// ============================================================================
void NpcDialogueManager::SetHeartEventCompleted(
    const std::string& npc_id,
    const std::string& event_id,
    bool completed) {
    heart_event_completed_[HeartEventKey_(npc_id, event_id)] = completed;
}

// ============================================================================
// 【SelectDailyDialogue】选择日常对话
// ============================================================================
std::vector<DialogueNode> NpcDialogueManager::SelectDailyDialogue(
    const std::string& npc_id,
    const NpcDialogueContext& ctx) {
    std::vector<DialogueNode> sequence;
    if (!stage_dialogues_loaded_) {
        LoadStageDialoguesFromCsv_(data_root_ + "/npc/npc_development_dialogue.csv", stage_dialogues_);
        stage_dialogues_loaded_ = true;
    }
    if (const auto it = stage_dialogues_.find(npc_id); it != stage_dialogues_.end()) {
        const auto stage_it = std::find_if(it->second.begin(), it->second.end(), [&](const StageDialogueEntry& e) {
            return e.stage == ctx.npc_stage;
        });
        if (stage_it != it->second.end()) {
            DialogueNode stage_node;
            stage_node.id = npc_id + "_stage_line";
            stage_node.speaker = ctx.npc_name;
            stage_node.text = ReplaceExtendedTokens(stage_it->text, ctx);
            sequence.push_back(std::move(stage_node));
        }
    }

    const std::unordered_map<std::string, std::pair<Season, int>> birthdays = {
        {"acha", {Season::Spring, 8}},
        {"xiaoman", {Season::Summer, 15}},
        {"wanxing", {Season::Winter, 21}},
        {"lin", {Season::Autumn, 12}},
    };
    auto birthday_it = birthdays.find(npc_id);
    if (birthday_it != birthdays.end() && ctx.current_season
        && *ctx.current_season == birthday_it->second.first
        && ctx.current_day_in_season == birthday_it->second.second) {
        DialogueNode birthday_node;
        birthday_node.id = npc_id + "_birthday_special";
        birthday_node.speaker = ctx.npc_name;
        birthday_node.text = ReplaceExtendedTokens("今天是我的生日，$[PLAYER_NAME]，谢谢你还记得。", ctx);
        sequence.push_back(std::move(birthday_node));
    } else if (ctx.current_day_in_season == 7 || ctx.current_day_in_season == 14
        || ctx.current_day_in_season == 21 || ctx.current_day_in_season == 28) {
        DialogueNode festival_node;
        festival_node.id = npc_id + "_festival_special";
        festival_node.speaker = ctx.npc_name;
        festival_node.text = ReplaceExtendedTokens("今天是节日，山庄比平时更热闹。", ctx);
        sequence.push_back(std::move(festival_node));
    }

    auto greetings = GetGreeting(npc_id, ctx);
    auto small_talks = GetSmallTalk(npc_id, ctx);
    auto farewells = GetFarewell(npc_id, ctx);

    sequence.insert(sequence.end(), greetings.begin(), greetings.end());
    sequence.insert(sequence.end(), small_talks.begin(), small_talks.end());
    sequence.insert(sequence.end(), farewells.begin(), farewells.end());

    // 为多段对话设置 next 链接
    for (std::size_t i = 0; i + 1 < sequence.size(); ++i) {
        if (sequence[i].choices.empty()) {
            DialogueChoice auto_continue;
            auto_continue.id = "auto";
            auto_continue.text = "[继续]";
            auto_continue.next_node_id = sequence[i + 1].id;
            sequence[i].choices.push_back(auto_continue);
        }
    }

    return sequence;
}

// ============================================================================
// 【IsDailyDialogueLoaded】检查是否已加载
// ============================================================================
bool NpcDialogueManager::IsDailyDialogueLoaded(const std::string& npc_id) const {
    return loaded_npc_ids_.contains(npc_id);
}

// ============================================================================
// 【GetLoadedNpcIds】获取已加载 NPC ID
// ============================================================================
std::vector<std::string> NpcDialogueManager::GetLoadedNpcIds() const {
    return std::vector<std::string>(loaded_npc_ids_.begin(),
                                     loaded_npc_ids_.end());
}

// ============================================================================
// 【BuildNpcDialogueContext】构建 NPC 对话上下文
// ============================================================================
NpcDialogueContext BuildNpcDialogueContext(
    const NpcActor& npc,
    const CloudSeamanor::domain::GameClock& clock,
    const CloudSeamanor::domain::CloudState cloud_state,
    const std::string& player_name,
    const std::string& farm_name,
    bool is_married,
    const std::string& spouse_id,
    const std::string& spouse_call) {
    NpcDialogueContext ctx;
    ctx.npc_id = npc.id;
    ctx.npc_name = npc.display_name;
    ctx.player_name = player_name;
    ctx.farm_name = farm_name;
    ctx.player_favor = npc.favor;
    ctx.npc_heart_level = npc.heart_level;
    ctx.npc_stage = npc.development_stage;
    ctx.current_day = clock.Day();
    ctx.current_day_in_season = clock.DayInSeason();
    ctx.current_hour = clock.Hour();
    ctx.current_season = clock.Season();
    ctx.current_weather = static_cast<NpcWeather>(static_cast<int>(cloud_state));
    ctx.current_time_of_day = clock.CurrentDayPhase();
    ctx.current_location = npc.current_location;
    ctx.current_activity = npc.current_activity;
    ctx.npc_mood = npc.mood;
    ctx.recent_item = "茶包";
    // 婚姻状态
    ctx.is_married = is_married;
    ctx.spouse_id = spouse_id;
    ctx.spouse_call = spouse_call;
    return ctx;
}

} // namespace CloudSeamanor::engine
