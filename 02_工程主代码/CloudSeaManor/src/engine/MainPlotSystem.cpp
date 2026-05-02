// ============================================================================
// 【MainPlotSystem】主线剧情系统实现
// ============================================================================
#include "CloudSeamanor/engine/MainPlotSystem.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

using CloudSeamanor::engine::DialogueCallbacks;
using CloudSeamanor::engine::DialogueChoice;
using CloudSeamanor::engine::DialogueNode;
using CloudSeamanor::engine::DialogueState;
using CloudSeamanor::engine::Event;
using CloudSeamanor::engine::GlobalEventBus;

// ============================================================================
// 【PlotCondition::IsSatisfied】条件判断
// ============================================================================
bool PlotCondition::IsSatisfied(
    int day,
    Season season,
    int cloud_level,
    const std::string& location,
    const std::unordered_set<std::string>& flags,
    const std::unordered_map<std::string, int>& npc_hearts,
    PlotRoute current_route) const {

    switch (type) {
        case PlotConditionType::DayRange:
            return day >= int_value && day <= int_value2;
        case PlotConditionType::Season:
            return season == SeasonFromString(str_value);
        case PlotConditionType::ChapterCompleted:
            return flags.contains("chapter_completed_" + str_value);
        case PlotConditionType::CloudLevel:
            return cloud_level >= int_value;
        case PlotConditionType::HeartCount: {
            auto it = npc_hearts.find(str_value);
            return it != npc_hearts.end() && it->second >= int_value;
        }
        case PlotConditionType::TotalHeartCount: {
            int total = 0;
            for (const auto& [npc, heart] : npc_hearts) {
                total += heart;
            }
            return total >= int_value;
        }
        case PlotConditionType::FestivalJoined:
            return flags.contains("festival_joined_" + str_value);
        case PlotConditionType::Location:
            return location == str_value;
        case PlotConditionType::ItemOwned:
            return flags.contains("item_owned_" + str_value);
        case PlotConditionType::FlagSet:
            return flags.contains(str_value);
        case PlotConditionType::RouteSelected:
            return current_route == PlotRouteFromString(str_value);
        case PlotConditionType::CloudState:
            return cloud_level >= int_value;
        case PlotConditionType::AllNpcHeartCount: {
            if (npc_hearts.empty()) return false;
            int total = 0;
            for (const auto& [npc, heart] : npc_hearts) {
                total += heart;
            }
            float avg = static_cast<float>(total) / npc_hearts.size();
            return avg >= static_cast<float>(int_value);
        }
        default:
            return true;
    }
}

// ============================================================================
// 【MainPlotSystem::MainPlotSystem】构造函数
// ============================================================================
MainPlotSystem::MainPlotSystem() = default;

// ============================================================================
// 【MainPlotSystem::Initialize】初始化
// ============================================================================
void MainPlotSystem::Initialize(const std::string& data_root) {
    data_root_ = data_root;
    LoadChapterData_();
    LoadPlotData_();
}

// ============================================================================
// 【MainPlotSystem::SetCallbacks】设置回调
// ============================================================================
void MainPlotSystem::SetCallbacks(PlotCallbacks callbacks) {
    callbacks_ = std::move(callbacks);

    DialogueCallbacks de_callbacks;
    de_callbacks.on_complete = [this]() {};
    de_callbacks.on_node_change = [this](const DialogueNode&) {};
    de_callbacks.on_text_update = [](const std::string&) {};
    de_callbacks.on_choices_change = [](const std::vector<DialogueChoice>&) {};
    de_callbacks.on_favor_change = [](int, int) {};

    dialogue_engine_.SetCallbacks(std::move(de_callbacks));
}

// ============================================================================
// 【MainPlotSystem::SetGameClock】注入GameClock
// ============================================================================
void MainPlotSystem::SetGameClock(const GameClock* clock) {
    clock_ = clock;
}

// ============================================================================
// 【MainPlotSystem::SetCloudSystem】注入CloudSystem
// ============================================================================
void MainPlotSystem::SetCloudSystem(const CloudSystem* cloud) {
    cloud_system_ = cloud;
}

// ============================================================================
// 【MainPlotSystem::SetNpcHeartGetter】注入NPC心级查询
// ============================================================================
void MainPlotSystem::SetNpcHeartGetter(
    const std::function<int(const std::string&)>& getter) {
    npc_heart_getter_ = getter;
}

// ============================================================================
// 【MainPlotSystem::LoadChapterData_】加载章节数据
// ============================================================================
void MainPlotSystem::LoadChapterData_() {
    std::string index_path = data_root_ + "/plot/chapter_index.json";
    std::ifstream index_file(index_path);
    if (index_file.is_open()) {
        std::stringstream ss;
        ss << index_file.rdbuf();
        std::string json = ss.str();
        index_file.close();

        auto parsed = ParseChaptersFromJson_(json);
        if (!parsed.empty()) {
            chapters_ = std::move(parsed);
            for (const auto& ch : chapters_) {
                chapter_index_[ch.id] = &ch;
            }
            return;
        }
    }

    // 兜底：内嵌章节数据（ch1-ch10）
    chapters_ = {
        ChapterEntry{
            .id = "ch1",
            .chapter_number = 1,
            .title = "归山",
            .subtitle = "春季·第1年",
            .description = "玩家返乡接手荒废山庄，面对村民的质疑。",
            .unlock_conditions = {},
            .prologue_plot_id = "plot_ch1_prologue",
            .epilogue_plot_id = "",
            .plot_ids = {"plot_ch1_prologue", "plot_ch1_first_meet",
                         "plot_ch1_first_work", "plot_ch1_awakening_festival",
                         "plot_ch1_night_talk", "plot_ch1_spirit_beast",
                         "plot_ch1_cloud_rift"},
            .completed = false,
            .target_cloud_level = 100,
            .target_heart_count = 0,
            .next_chapter_id = "ch2",
        },
        ChapterEntry{
            .id = "ch2",
            .chapter_number = 2,
            .title = "云海初醒",
            .subtitle = "夏季·第1年",
            .description = "云海系统正式激活，茶九透露上界身份。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch1"},
            },
            .prologue_plot_id = "plot_ch2_prologue",
            .plot_ids = {"plot_ch2_prologue", "plot_ch2_tea_secret",
                         "plot_ch2_chajiou_secret", "plot_ch2_golden_bud",
                         "plot_ch2_upper_world", "plot_ch2_cloud_tide",
                         "plot_ch2_relic", "plot_ch2_ancient_text"},
            .completed = false,
            .target_cloud_level = 300,
            .target_heart_count = 0,
            .next_chapter_id = "ch3",
        },
        ChapterEntry{
            .id = "ch3",
            .chapter_number = 3,
            .title = "界桥裂痕",
            .subtitle = "秋季·第1年~春季·第2年",
            .description = "秋末界桥危机，玩家首次成功修复界桥。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch2"},
            },
            .prologue_plot_id = "plot_ch3_prologue",
            .plot_ids = {"plot_ch3_prologue", "plot_ch3_festival",
                         "plot_ch3_cloud_rift", "plot_ch3_fix_realm",
                         "plot_ch3_autumn_memory", "plot_ch3_rebuild"},
            .completed = false,
            .target_cloud_level = 500,
            .target_heart_count = 0,
            .next_chapter_id = "ch4",
        },
        ChapterEntry{
            .id = "ch4",
            .chapter_number = 4,
            .title = "人心与灵气",
            .subtitle = "夏季·第2年",
            .description = "山庄繁荣与感情的抉择。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch3"},
            },
            .prologue_plot_id = "plot_ch4_prologue",
            .plot_ids = {"plot_ch4_prologue", "plot_ch4_tourists",
                         "plot_ch4_seal_array", "plot_ch4_love_choice",
                         "plot_ch4_village_declaration"},
            .completed = false,
            .target_cloud_level = 700,
            .target_heart_count = 3,
            .next_chapter_id = "ch5",
        },
        ChapterEntry{
            .id = "ch5",
            .chapter_number = 5,
            .title = "抉择之潮",
            .subtitle = "秋季·第2年",
            .description = "真相揭开，三方势力争夺界桥控制权。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch4"},
            },
            .prologue_plot_id = "plot_ch5_prologue",
            .plot_ids = {"plot_ch5_prologue", "plot_ch5_three_paths", "plot_ch5_preparation"},
            .completed = false,
            .target_cloud_level = 0,
            .target_heart_count = 0,
            .next_chapter_id = "ch6",
        },
        ChapterEntry{
            .id = "ch6",
            .chapter_number = 6,
            .title = "山庄繁盛",
            .subtitle = "冬季·第2年~春季·第3年",
            .description = "山庄全面繁荣，准备最终决战。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch5"},
            },
            .prologue_plot_id = "",
            .plot_ids = {},
            .completed = false,
            .target_cloud_level = 850,
            .target_heart_count = 0,
            .next_chapter_id = "ch7",
        },
        ChapterEntry{
            .id = "ch7",
            .chapter_number = 7,
            .title = "界桥动摇",
            .subtitle = "夏季·第3年",
            .description = "深层真相揭露，界桥核心碎片收集。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch6"},
            },
            .prologue_plot_id = "",
            .plot_ids = {},
            .completed = false,
            .target_cloud_level = 0,
            .target_heart_count = 0,
            .next_chapter_id = "ch8",
        },
        ChapterEntry{
            .id = "ch8",
            .chapter_number = 8,
            .title = "太初遗迹",
            .subtitle = "秋季·第3年",
            .description = "太初云海祭解锁，终局迷宫探索。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch7"},
            },
            .prologue_plot_id = "",
            .plot_ids = {},
            .completed = false,
            .target_cloud_level = 0,
            .target_heart_count = 0,
            .next_chapter_id = "ch9",
        },
        ChapterEntry{
            .id = "ch9",
            .chapter_number = 9,
            .title = "最终抉择",
            .subtitle = "冬季·第3年",
            .description = "终局选择执行，完成各路线结局收束。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch8"},
            },
            .prologue_plot_id = "",
            .plot_ids = {},
            .completed = false,
            .target_cloud_level = 0,
            .target_heart_count = 0,
            .next_chapter_id = "ch10",
        },
        ChapterEntry{
            .id = "ch10",
            .chapter_number = 10,
            .title = "云海归心",
            .subtitle = "跨年",
            .description = "达成对应结局，新生与归宿。",
            .unlock_conditions = {
                PlotCondition{.type = PlotConditionType::ChapterCompleted, .str_value = "ch9"},
            },
            .prologue_plot_id = "",
            .plot_ids = {},
            .completed = false,
            .target_cloud_level = 0,
            .target_heart_count = 0,
            .next_chapter_id = "",
        },
    };

    for (const auto& ch : chapters_) {
        chapter_index_[ch.id] = &ch;
    }
}

// ============================================================================
// 【MainPlotSystem::LoadPlotData_】加载剧情数据
// ============================================================================
void MainPlotSystem::LoadPlotData_() {
    for (const auto& chapter : chapters_) {
        std::string ch_dir = data_root_ + "/plot/" + chapter.id;
        std::string index_file_path = ch_dir + "/index.json";

        std::ifstream idx_file(index_file_path);
        if (!idx_file.is_open()) continue;

        std::stringstream ss;
        ss << idx_file.rdbuf();
        std::string idx_json = ss.str();
        idx_file.close();

        // 解析index.json中的文件引用列表
        std::vector<std::pair<std::string, std::string>> file_list;
        {
            std::string::size_type pos = 0;
            while ((pos = idx_json.find("\"file\"", pos)) != std::string::npos) {
                auto start = idx_json.find('"', pos);
                auto end = idx_json.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    file_list.emplace_back(idx_json.substr(start + 1, end - start - 1), "");
                }
                ++pos;
            }
        }

        // 加载每个引用的剧情JSON文件
        for (const auto& file_ref : file_list) {
            std::string plot_file_path = ch_dir + "/" + file_ref.first;
            std::ifstream plot_file(plot_file_path);
            if (!plot_file.is_open()) continue;

            std::stringstream ps;
            ps << plot_file.rdbuf();
            std::string plot_json = ps.str();
            plot_file.close();

            auto parsed = ParsePlotsFromJson_(plot_json);
            for (auto& plot : parsed) {
                plot.chapter_id = chapter.id;
                all_plots_.push_back(std::move(plot));
            }
        }
    }

    for (const auto& plot : all_plots_) {
        plot_index_[plot.id] = &plot;
    }
}

// ============================================================================
// 【MainPlotSystem::ParseChaptersFromJson_】解析章节JSON
// ============================================================================
std::vector<ChapterEntry> MainPlotSystem::ParseChaptersFromJson_(
    const std::string& json) const {

    std::vector<ChapterEntry> result;

    auto find_key = [](const std::string& s, const std::string& key) -> std::string::size_type {
        std::string q = "\"" + key + "\"";
        auto pos = s.find(q);
        return pos;
    };

    auto extract_string = [](const std::string& s, std::string::size_type pos) -> std::pair<std::string, std::string::size_type> {
        if (pos == std::string::npos || pos >= s.size()) return {"", std::string::npos};
        auto start = s.find('"', pos);
        if (start == std::string::npos) return {"", std::string::npos};
        auto end = s.find('"', start + 1);
        if (end == std::string::npos) return {"", std::string::npos};
        return {s.substr(start + 1, end - start - 1), end + 1};
    };

    auto extract_int = [&](const std::string& s, std::string::size_type pos) -> std::pair<int, std::string::size_type> {
        if (pos == std::string::npos) return {0, std::string::npos};
        std::string::size_type end = pos;
        while (end < s.size() && (std::isdigit(static_cast<unsigned char>(s[end])) || s[end] == '-')) ++end;
        if (end == pos) return {0, pos};
        try { return {std::stoi(s.substr(pos, end - pos)), end}; }
        catch (const std::invalid_argument&) { return {0, pos}; }
        catch (const std::out_of_range&) { return {0, pos}; }
        catch (const std::exception&) { return {0, pos}; }
    };

    auto find_section = [&](const std::string& s, const std::string& key) -> std::string::size_type {
        return find_key(s, key);
    };

    auto chapters_pos = find_section(json, "chapters");
    if (chapters_pos == std::string::npos) return {};

    auto arr_start = json.find('[', chapters_pos);
    if (arr_start == std::string::npos) return {};

    std::string::size_type pos = arr_start + 1;
    while (pos < json.size()) {
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
        if (pos >= json.size() || json[pos] != '{') break;

        int brace_count = 1;
        std::string::size_type obj_end = pos + 1;
        while (obj_end < json.size() && brace_count > 0) {
            if (json[obj_end] == '{') ++brace_count;
            else if (json[obj_end] == '}') --brace_count;
            ++obj_end;
        }

        std::string obj = json.substr(pos, obj_end - pos);
        ChapterEntry ch;

        auto [id_val, _] = extract_string(obj, find_key(obj, "id"));
        ch.id = id_val;

        auto [num_val, _2] = extract_int(obj, find_key(obj, "chapter_number"));
        ch.chapter_number = num_val;

        auto [title_val, _3] = extract_string(obj, find_key(obj, "title"));
        ch.title = title_val;

        auto [desc_val, _4] = extract_string(obj, find_key(obj, "description"));
        ch.description = desc_val;

        auto [timeline_val, _5] = extract_string(obj, find_key(obj, "timeline"));
        ch.subtitle = timeline_val;

        auto [goal_val, _6] = extract_string(obj, find_key(obj, "chapter_goal"));
        (void)_6;
        (void)goal_val;

        auto [target_val, _7] = extract_int(obj, find_key(obj, "target_cloud"));
        ch.target_cloud_level = target_val;

        pos = obj_end;
    }

    return result;
}

// ============================================================================
// 【MainPlotSystem::ParsePlotsFromJson_】解析剧情JSON
// ============================================================================
std::vector<PlotEntry> MainPlotSystem::ParsePlotsFromJson_(
    const std::string& json) const {

    std::vector<PlotEntry> result;

    auto find_key = [](const std::string& s, const std::string& key) -> std::string::size_type {
        std::string q = "\"" + key + "\"";
        return s.find(q);
    };

    auto extract_string = [](const std::string& s, std::string::size_type pos) -> std::pair<std::string, std::string::size_type> {
        if (pos == std::string::npos || pos >= s.size()) return {"", std::string::npos};
        auto start = s.find('"', pos);
        if (start == std::string::npos) return {"", std::string::npos};
        auto end = s.find('"', start + 1);
        if (end == std::string::npos) return {"", std::string::npos};
        return {s.substr(start + 1, end - start - 1), end + 1};
    };

    auto extract_int = [&](const std::string& s, std::string::size_type pos) -> std::pair<int, std::string::size_type> {
        if (pos == std::string::npos) return {0, std::string::npos};
        std::string::size_type end = pos;
        while (end < s.size() && (std::isdigit(static_cast<unsigned char>(s[end])) || s[end] == '-')) ++end;
        if (end == pos) return {0, pos};
        try { return {std::stoi(s.substr(pos, end - pos)), end}; }
        catch (const std::invalid_argument&) { return {0, pos}; }
        catch (const std::out_of_range&) { return {0, pos}; }
        catch (const std::exception&) { return {0, pos}; }
    };

    auto find_section = [&](const std::string& s, const std::string& key) -> std::string::size_type {
        return find_key(s, key);
    };

    auto parse_condition_type = [](const std::string& type_text) -> PlotConditionType {
        if (type_text == "day_range") return PlotConditionType::DayRange;
        if (type_text == "season") return PlotConditionType::Season;
        if (type_text == "chapter_completed") return PlotConditionType::ChapterCompleted;
        if (type_text == "cloud_level") return PlotConditionType::CloudLevel;
        if (type_text == "heart_count") return PlotConditionType::HeartCount;
        if (type_text == "total_heart_count") return PlotConditionType::TotalHeartCount;
        if (type_text == "festival_joined") return PlotConditionType::FestivalJoined;
        if (type_text == "location") return PlotConditionType::Location;
        if (type_text == "item_owned") return PlotConditionType::ItemOwned;
        if (type_text == "flag_set") return PlotConditionType::FlagSet;
        if (type_text == "route_selected") return PlotConditionType::RouteSelected;
        if (type_text == "cloud_state") return PlotConditionType::CloudState;
        if (type_text == "all_npc_heart_count") return PlotConditionType::AllNpcHeartCount;
        return PlotConditionType::FlagSet;
    };

    auto parse_conditions = [&](const std::string& obj, const std::string& section_key)
        -> std::vector<PlotCondition> {
        std::vector<PlotCondition> conditions;
        const auto section_pos = find_key(obj, section_key);
        if (section_pos == std::string::npos) {
            return conditions;
        }
        const auto arr_start = obj.find('[', section_pos);
        if (arr_start == std::string::npos) {
            return conditions;
        }
        const auto arr_end = obj.find(']', arr_start);
        if (arr_end == std::string::npos) {
            return conditions;
        }
        const std::string array_text = obj.substr(arr_start + 1, arr_end - arr_start - 1);
        std::string::size_type pos2 = 0;
        while (pos2 < array_text.size()) {
            while (pos2 < array_text.size() && std::isspace(static_cast<unsigned char>(array_text[pos2]))) ++pos2;
            if (pos2 >= array_text.size() || array_text[pos2] != '{') break;
            int brace_count = 1;
            std::string::size_type cond_end = pos2 + 1;
            while (cond_end < array_text.size() && brace_count > 0) {
                if (array_text[cond_end] == '{') ++brace_count;
                else if (array_text[cond_end] == '}') --brace_count;
                ++cond_end;
            }
            const std::string cond_obj = array_text.substr(pos2, cond_end - pos2);

            PlotCondition cond{};
            auto [type_val, _t] = extract_string(cond_obj, find_key(cond_obj, "type"));
            auto [value_val, _v] = extract_string(cond_obj, find_key(cond_obj, "value"));
            auto [value2_val, _v2] = extract_string(cond_obj, find_key(cond_obj, "value2"));
            cond.type = parse_condition_type(type_val);
            cond.str_value = value_val;
            cond.str_value2 = value2_val;

            const auto int_pos = find_key(cond_obj, "int_value");
            if (int_pos != std::string::npos) {
                auto [n, _n] = extract_int(cond_obj, int_pos);
                cond.int_value = n;
            } else if (!value_val.empty()) {
                auto [n, _n] = extract_int(value_val, 0);
                cond.int_value = n;
            }
            conditions.push_back(std::move(cond));
            pos2 = cond_end;
        }
        return conditions;
    };

    auto arr_start = json.find('[');
    if (arr_start == std::string::npos) return {};

    std::string::size_type pos = arr_start + 1;
    while (pos < json.size()) {
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
        if (pos >= json.size() || json[pos] != '{') break;

        int brace_count = 1;
        std::string::size_type obj_end = pos + 1;
        while (obj_end < json.size() && brace_count > 0) {
            if (json[obj_end] == '{') ++brace_count;
            else if (json[obj_end] == '}') --brace_count;
            ++obj_end;
        }

        std::string obj = json.substr(pos, obj_end - pos);

        auto [id_val, _] = extract_string(obj, find_key(obj, "id"));
        if (id_val.empty()) {
            pos = obj_end;
            continue;
        }

        PlotEntry plot;
        plot.id = id_val;

        auto [title_val, _2] = extract_string(obj, find_key(obj, "title"));
        plot.title = title_val;

        auto [desc_val, _3] = extract_string(obj, find_key(obj, "description"));
        plot.description = desc_val;

        auto [start_val, _4] = extract_string(obj, find_key(obj, "start_node_id"));
        plot.start_node_id = start_val;

        auto [next_val, _5] = extract_string(obj, find_key(obj, "next_plot_id"));
        plot.next_plot_id = next_val;

        auto [prio_val, _6] = extract_int(obj, find_key(obj, "priority"));
        plot.priority = prio_val;
        plot.trigger_conditions = parse_conditions(obj, "trigger_conditions");
        plot.enter_conditions = parse_conditions(obj, "enter_conditions");

        auto nodes = ParseNodesFromJson_(obj);
        plot.nodes = std::move(nodes);

        result.push_back(std::move(plot));
        pos = obj_end;
    }

    return result;
}

// ============================================================================
// 【MainPlotSystem::ParseNodesFromJson_】解析节点JSON
// ============================================================================
std::vector<PlotNode> MainPlotSystem::ParseNodesFromJson_(
    const std::string& json) const {

    std::vector<PlotNode> result;

    auto find_key = [](const std::string& s, const std::string& key) -> std::string::size_type {
        std::string q = "\"" + key + "\"";
        return s.find(q);
    };

    auto extract_string = [](const std::string& s, std::string::size_type pos) -> std::pair<std::string, std::string::size_type> {
        if (pos == std::string::npos || pos >= s.size()) return {"", std::string::npos};
        auto start = s.find('"', pos);
        if (start == std::string::npos) return {"", std::string::npos};
        auto end = s.find('"', start + 1);
        if (end == std::string::npos) return {"", std::string::npos};
        return {s.substr(start + 1, end - start - 1), end + 1};
    };

    auto extract_int = [&](const std::string& s, std::string::size_type pos) -> std::pair<int, std::string::size_type> {
        if (pos == std::string::npos) return {0, std::string::npos};
        std::string::size_type end = pos;
        while (end < s.size() && (std::isdigit(static_cast<unsigned char>(s[end])) || s[end] == '-')) ++end;
        if (end == pos) return {0, pos};
        try { return {std::stoi(s.substr(pos, end - pos)), end}; }
        catch (const std::invalid_argument&) { return {0, pos}; }
        catch (const std::out_of_range&) { return {0, pos}; }
        catch (const std::exception&) { return {0, pos}; }
    };

    auto extract_bool = [&](const std::string& s, std::string::size_type pos) -> std::pair<bool, std::string::size_type> {
        if (pos == std::string::npos) return {false, pos};
        auto end = pos;
        while (end < s.size() && std::isspace(static_cast<unsigned char>(s[end]))) ++end;
        if (end >= s.size()) return {false, pos};
        return {s.substr(end, 4) == "true", end + 4};
    };

    auto nodes_pos = find_key(json, "nodes");
    if (nodes_pos == std::string::npos) return {};

    auto arr_start = json.find('[', nodes_pos);
    if (arr_start == std::string::npos) return {};

    std::string::size_type pos = arr_start + 1;
    while (pos < json.size()) {
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
        if (pos >= json.size() || json[pos] != '{') break;

        int brace_count = 1;
        std::string::size_type obj_end = pos + 1;
        while (obj_end < json.size() && brace_count > 0) {
            if (json[obj_end] == '{') ++brace_count;
            else if (json[obj_end] == '}') --brace_count;
            ++obj_end;
        }

        std::string obj = json.substr(pos, obj_end - pos);
        PlotNode node;

        auto [id_val, _] = extract_string(obj, find_key(obj, "id"));
        if (id_val.empty()) {
            pos = obj_end;
            continue;
        }
        node.id = id_val;

        auto [speaker_val, _2] = extract_string(obj, find_key(obj, "speaker"));
        node.speaker = speaker_val;

        auto [text_val, _3] = extract_string(obj, find_key(obj, "text"));
        node.text = text_val;

        auto [next_val, _4] = extract_string(obj, find_key(obj, "next_node_id"));
        node.next_node_id = next_val;

        auto [cutscene_pos, _5] = extract_bool(obj, find_key(obj, "is_cutscene"));
        node.is_cutscene = cutscene_pos;

        auto [cloud_pos, _6] = extract_int(obj, find_key(obj, "cloud_delta"));
        node.cloud_delta = cloud_pos;

        auto [flag_pos, _7] = extract_string(obj, find_key(obj, "flag_to_set"));
        node.flag_to_set = flag_pos;

        auto [region_pos, _8] = extract_string(obj, find_key(obj, "unlock_region"));
        node.unlock_region = region_pos;

        auto [npc_pos, _9] = extract_string(obj, find_key(obj, "unlock_npc"));
        node.unlock_npc = npc_pos;

        auto [chflag_pos, _10] = extract_string(obj, find_key(obj, "chapter_complete_flag"));
        node.chapter_complete_flag = chflag_pos;

        auto [event_pos, _11] = extract_string(obj, find_key(obj, "trigger_event"));
        node.trigger_event = event_pos;

        auto [choice_next, _12] = extract_string(obj, find_key(obj, "choice_next"));
        if (!choice_next.empty()) {
            DialogueChoice c;
            c.id = "choice_" + std::to_string(node.choices.size());
            c.text = choice_next;
            c.next_node_id = choice_next;
            node.choices.push_back(c);
        }

        result.push_back(std::move(node));
        pos = obj_end;
    }

    return result;
}

// ============================================================================
// 【MainPlotSystem::Update】帧更新
// ============================================================================
void MainPlotSystem::Update(float delta_seconds) {
    if (IsPlaying()) {
        dialogue_engine_.Update(delta_seconds);

        if (dialogue_engine_.State() == DialogueState::Completed) {
            if (!current_node_id_.empty()) {
                const auto* plot = CurrentPlot();
                if (plot) {
                    for (const auto& node : plot->nodes) {
                        if (node.id == current_node_id_) {
                            ExecuteNodeEffect_(node);
                            break;
                        }
                    }
                }
            }

            if (!current_node_id_.empty()) {
                const auto* plot = CurrentPlot();
                if (plot) {
                    for (const auto& node : plot->nodes) {
                        if (node.id == current_node_id_ && !node.next_node_id.empty()) {
                            EnterNode_(node.next_node_id);
                            return;
                        }
                    }
                }
            }

            OnPlotComplete_();
        }
    }
}

// ============================================================================
// 【MainPlotSystem::CheckPlotTriggers】每日剧情触发检查
// ============================================================================
void MainPlotSystem::CheckPlotTriggers() {
    if (IsPlaying()) return;

    triggered_this_day_.clear();

    const auto triggerable = GetTriggerablePlots_();
    for (const auto* plot : triggerable) {
        if (CanTriggerPlot_(*plot) && CanEnterPlot_(*plot)) {
            if (callbacks_.on_plot_start) {
                callbacks_.on_plot_start(plot->id);
            }
            StartPlot(plot->id);
            return;
        }
    }
}

// ============================================================================
// 【MainPlotSystem::OnLocationEntered】进入地点触发
// ============================================================================
void MainPlotSystem::OnLocationEntered(const std::string& map_id,
                                      const std::string& anchor_id) {
    if (IsPlaying()) return;

    const auto triggerable = GetTriggerablePlots_();
    for (const auto* plot : triggerable) {
        if (plot->map_id == map_id && plot->anchor_id == anchor_id) {
            if (CanTriggerPlot_(*plot) && CanEnterPlot_(*plot)) {
                if (callbacks_.on_plot_start) {
                    callbacks_.on_plot_start(plot->id);
                }
                StartPlot(plot->id);
                return;
            }
        }
    }
}

// ============================================================================
// 【MainPlotSystem::OnGameEvent】游戏事件触发
// ============================================================================
void MainPlotSystem::OnGameEvent(
    const std::string& event_type,
    const std::unordered_map<std::string, std::string>& event_data) {
    if (event_type == "flag_set") {
        auto it = event_data.find("flag");
        if (it != event_data.end()) {
            flags_.insert(it->second);
            if (callbacks_.on_flag_set) {
                callbacks_.on_flag_set(it->second);
            }
        }
    }

    if (event_type == "chapter_complete") {
        auto it = event_data.find("chapter_id");
        if (it != event_data.end()) {
            completed_chapters_.insert(it->second);
            flags_.insert("chapter_completed_" + it->second);
        }
    }

    CheckPlotTriggers();
}

// ============================================================================
// 【MainPlotSystem::StartPlot】开始剧情
// ============================================================================
void MainPlotSystem::StartPlot(const std::string& plot_id) {
    auto it = plot_index_.find(plot_id);
    if (it == plot_index_.end()) return;

    const auto* plot = it->second;
    if (!plot) return;

    current_plot_id_ = plot_id;
    current_chapter_id_ = plot->chapter_id;
    current_node_id_ = plot->start_node_id;
    state_ = PlotState::Playing;

    if (callbacks_.on_chapter_start) {
        callbacks_.on_chapter_start(plot->chapter_id);
    }
    if (callbacks_.on_plot_start) {
        callbacks_.on_plot_start(plot_id);
    }

    EnterNode_(plot->start_node_id);
}

// ============================================================================
// 【MainPlotSystem::StartChapter】开始章节
// ============================================================================
void MainPlotSystem::StartChapter(const std::string& chapter_id) {
    auto it = chapter_index_.find(chapter_id);
    if (it == chapter_index_.end()) return;

    const auto* chapter = it->second;
    if (!chapter) return;

    current_chapter_id_ = chapter_id;

    if (!chapter->prologue_plot_id.empty()) {
        StartPlot(chapter->prologue_plot_id);
    }
}

// ============================================================================
// 【MainPlotSystem::EndPlot】结束剧情
// ============================================================================
void MainPlotSystem::EndPlot() {
    state_ = PlotState::Idle;
    current_plot_id_.clear();
    current_node_id_.clear();
    dialogue_engine_.EndDialogue();
}

// ============================================================================
// 【MainPlotSystem::OnConfirm】确认键
// ============================================================================
bool MainPlotSystem::OnConfirm() {
    if (!IsPlaying()) return false;

    if (dialogue_engine_.State() == DialogueState::Typing) {
        dialogue_engine_.SkipTyping();
        return true;
    }

    if (dialogue_engine_.State() == DialogueState::WaitingChoice) {
        if (dialogue_engine_.CurrentChoices().empty()) {
            return dialogue_engine_.OnConfirm();
        }
        return false;
    }

    return false;
}

// ============================================================================
// 【MainPlotSystem::SelectChoice】选择选项
// ============================================================================
bool MainPlotSystem::SelectChoice(std::size_t choice_index) {
    if (!IsPlaying()) return false;
    if (state_ != PlotState::ChoicePending &&
        dialogue_engine_.State() != DialogueState::WaitingChoice) {
        return false;
    }

    const auto& choices = dialogue_engine_.CurrentChoices();
    if (choice_index >= choices.size()) return false;

    const auto& choice = choices[choice_index];

    const auto* plot = CurrentPlot();
    if (plot) {
        for (const auto& node : plot->nodes) {
            if (node.id == current_node_id_) {
                for (const auto& effect : node.choice_effects) {
                    if (effect.flag_to_set == choice.id ||
                        effect.next_plot_id == choice.next_node_id) {
                        ExecuteChoiceEffect_(effect);
                        break;
                    }
                }
                break;
            }
        }
    }

    if (dialogue_engine_.SelectChoice(choice_index)) {
        if (!choice.next_node_id.empty()) {
            EnterNode_(choice.next_node_id);
        } else {
            OnPlotComplete_();
        }
        return true;
    }

    return false;
}

// ============================================================================
// 【MainPlotSystem::BuildDialogueContext_】构建对话上下文
// ============================================================================
DialogueContext MainPlotSystem::BuildDialogueContext_() const {
    DialogueContext ctx;
    ctx.current_day = clock_ ? clock_->Day() : 1;
    ctx.current_season = clock_ ? GameClock::SeasonName(clock_->Season()) : "春";
    ctx.current_weather = cloud_system_ ? cloud_system_->CurrentStateText() : "晴";
    return ctx;
}

// ============================================================================
// 【MainPlotSystem::EnterNode_】进入节点
// ============================================================================
void MainPlotSystem::EnterNode_(const std::string& node_id) {
    if (node_id.empty()) {
        OnPlotComplete_();
        return;
    }

    const auto* plot = CurrentPlot();
    if (!plot) return;

    const PlotNode* target_node = nullptr;
    for (const auto& n : plot->nodes) {
        if (n.id == node_id) {
            target_node = &n;
            break;
        }
    }

    if (!target_node) return;

    current_node_id_ = node_id;

    if (!target_node->trigger_event.empty()) {
        Event ev;
        ev.type = target_node->trigger_event;
        ev.data["plot_id"] = current_plot_id_;
        ev.data["node_id"] = node_id;
        GlobalEventBus().Emit(ev);
    }

    if (!target_node->unlock_region.empty()) {
        if (callbacks_.on_unlock_region) {
            callbacks_.on_unlock_region(target_node->unlock_region);
        }
        flags_.insert("region_unlocked_" + target_node->unlock_region);
    }

    if (!target_node->unlock_npc.empty()) {
        if (callbacks_.on_unlock_npc) {
            callbacks_.on_unlock_npc(target_node->unlock_npc);
        }
        flags_.insert("npc_unlocked_" + target_node->unlock_npc);
    }

    if (!target_node->flag_to_set.empty()) {
        flags_.insert(target_node->flag_to_set);
        if (callbacks_.on_flag_set) {
            callbacks_.on_flag_set(target_node->flag_to_set);
        }
    }

    if (target_node->cloud_delta != 0) {
        if (callbacks_.on_cloud_delta) {
            callbacks_.on_cloud_delta(target_node->cloud_delta);
        }
    }

    if (!target_node->chapter_complete_flag.empty()) {
        OnChapterComplete_(target_node->chapter_complete_flag);
    }

    DialogueNode de_node;
    de_node.id = target_node->id;
    de_node.speaker = target_node->speaker;
    de_node.text = target_node->text;
    de_node.choices = target_node->choices;

    if (target_node->choices.empty()) {
        state_ = PlotState::Playing;
    } else {
        state_ = PlotState::ChoicePending;
    }

    dialogue_engine_.StartDialogue({de_node}, de_node.id, BuildDialogueContext_());
}

// ============================================================================
// 【MainPlotSystem::ExecuteNodeEffect_】执行节点效果
// ============================================================================
void MainPlotSystem::ExecuteNodeEffect_(const PlotNode& node) {
    if (!node.flag_to_set.empty()) {
        flags_.insert(node.flag_to_set);
    }

    if (node.cloud_delta != 0 && callbacks_.on_cloud_delta) {
        callbacks_.on_cloud_delta(node.cloud_delta);
    }

    if (!node.chapter_complete_flag.empty()) {
        OnChapterComplete_(node.chapter_complete_flag);
    }

    if (!node.trigger_event.empty()) {
        Event ev;
        ev.type = node.trigger_event;
        GlobalEventBus().Emit(ev);
    }
}

// ============================================================================
// 【MainPlotSystem::ExecuteChoiceEffect_】执行选项效果
// ============================================================================
void MainPlotSystem::ExecuteChoiceEffect_(const PlotChoiceEffect& effect) {
    if (!effect.flag_to_set.empty()) {
        flags_.insert(effect.flag_to_set);
        if (callbacks_.on_flag_set) {
            callbacks_.on_flag_set(effect.flag_to_set);
        }
    }

    if (effect.cloud_level_delta != 0 && callbacks_.on_cloud_delta) {
        callbacks_.on_cloud_delta(effect.cloud_level_delta);
    }

    if (effect.favor_delta != 0 && !effect.npc_id_for_favor.empty()) {
        if (callbacks_.on_favor_delta) {
            callbacks_.on_favor_delta(effect.npc_id_for_favor, effect.favor_delta);
        }
    }

    if (!effect.route_lock.empty()) {
        LockRoute_(PlotRouteFromString(effect.route_lock));
    }
}

// ============================================================================
// 【MainPlotSystem::OnChapterComplete_】章节完成
// ============================================================================
void MainPlotSystem::OnChapterComplete_(const std::string& chapter_id) {
    completed_chapters_.insert(chapter_id);
    flags_.insert("chapter_completed_" + chapter_id);

    if (callbacks_.on_chapter_complete) {
        callbacks_.on_chapter_complete(chapter_id);
    }

    auto it = chapter_index_.find(chapter_id);
    if (it != chapter_index_.end() && it->second) {
        const auto& chapter = *it->second;
        if (!chapter.epilogue_plot_id.empty()) {
            StartPlot(chapter.epilogue_plot_id);
        }
    }
}

// ============================================================================
// 【MainPlotSystem::LockRoute_】锁定路线
// ============================================================================
void MainPlotSystem::LockRoute_(PlotRoute route) {
    current_route_ = route;
    flags_.insert(std::string("route_locked_") + RouteName(route));

    if (callbacks_.on_route_lock) {
        callbacks_.on_route_lock(RouteName(route));
    }

    if (callbacks_.on_notice) {
        callbacks_.on_notice("路线已锁定：" + std::string(
            route == PlotRoute::Balance ? "平衡之道" :
            route == PlotRoute::Open ? "打开之路" :
            "关闭之路"));
    }
}

// ============================================================================
// 【MainPlotSystem::GetTriggerablePlots_】获取可触发剧情
// ============================================================================
std::vector<const PlotEntry*> MainPlotSystem::GetTriggerablePlots_() const {
    std::vector<const PlotEntry*> result;

    int day = clock_ ? clock_->Day() : 1;
    Season season = clock_ ? clock_->Season() : Season::Spring;
    int cloud_level = cloud_system_ ? cloud_system_->SpiritEnergy() : 0;
    std::string location = "";

    const auto npc_hearts = GetNpcHeartLevels();

    for (const auto& plot : all_plots_) {
        if (completed_plots_.contains(plot.id) && !plot.repeatable) continue;
        if (!IsChapterUnlocked(plot.chapter_id)) continue;

        bool can_trigger = true;
        for (const auto& cond : plot.trigger_conditions) {
            if (!cond.IsSatisfied(day, season, cloud_level, location,
                                  flags_, npc_hearts, current_route_)) {
                can_trigger = false;
                break;
            }
        }

        if (can_trigger) {
            result.push_back(&plot);
        }
    }

    std::sort(result.begin(), result.end(),
             [](const PlotEntry* a, const PlotEntry* b) {
                 return a->priority > b->priority;
             });

    return result;
}

// ============================================================================
// 【MainPlotSystem::CanTriggerPlot_】检查是否可以触发
// ============================================================================
bool MainPlotSystem::CanTriggerPlot_(const PlotEntry& plot) const {
    if (!IsChapterUnlocked(plot.chapter_id)) return false;
    if (completed_plots_.contains(plot.id) && !plot.repeatable) return false;

    int day = clock_ ? clock_->Day() : 1;
    Season season = clock_ ? clock_->Season() : Season::Spring;
    int cloud_level = cloud_system_ ? cloud_system_->SpiritEnergy() : 0;
    std::string location = "";

    const auto npc_hearts = GetNpcHeartLevels();

    for (const auto& cond : plot.trigger_conditions) {
        if (!cond.IsSatisfied(day, season, cloud_level, location,
                              flags_, npc_hearts, current_route_)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// 【MainPlotSystem::CanEnterPlot_】检查是否可以进入
// ============================================================================
bool MainPlotSystem::CanEnterPlot_(const PlotEntry& plot) const {
    if (plot.enter_conditions.empty()) return true;

    int day = clock_ ? clock_->Day() : 1;
    Season season = clock_ ? clock_->Season() : Season::Spring;
    int cloud_level = cloud_system_ ? cloud_system_->SpiritEnergy() : 0;
    std::string location = "";

    const auto npc_hearts = GetNpcHeartLevels();

    for (const auto& cond : plot.enter_conditions) {
        if (!cond.IsSatisfied(day, season, cloud_level, location,
                              flags_, npc_hearts, current_route_)) {
            return false;
        }
    }

    return true;
}

std::unordered_map<std::string, int> MainPlotSystem::GetNpcHeartLevels() const {
    std::unordered_map<std::string, int> npc_hearts;
    if (!npc_heart_getter_) {
        return npc_hearts;
    }
    for (const auto id : kNpcIds) {
        int h = npc_heart_getter_(std::string{id});
        if (h > 0) {
            npc_hearts[std::string{id}] = h;
        }
    }
    return npc_hearts;
}

// ============================================================================
// 【MainPlotSystem::OnPlotComplete_】剧情完成处理
// ============================================================================
void MainPlotSystem::OnPlotComplete_() {
    if (!current_plot_id_.empty()) {
        completed_plots_.insert(current_plot_id_);

        if (callbacks_.on_plot_complete) {
            callbacks_.on_plot_complete(current_plot_id_);
        }

        auto it = plot_index_.find(current_plot_id_);
        if (it != plot_index_.end() && it->second) {
            const auto& plot = *it->second;
            if (!plot.next_plot_id.empty()) {
                std::string next = plot.next_plot_id;
                current_plot_id_.clear();
                current_node_id_.clear();
                state_ = PlotState::Idle;
                StartPlot(next);
                return;
            }
        }
    }

    state_ = PlotState::Idle;
}

// ============================================================================
// 【MainPlotSystem::CurrentPlot】获取当前剧情
// ============================================================================
const PlotEntry* MainPlotSystem::CurrentPlot() const {
    auto it = plot_index_.find(current_plot_id_);
    if (it != plot_index_.end()) return it->second;
    return nullptr;
}

// ============================================================================
// 【MainPlotSystem::CurrentChapter】获取当前章节
// ============================================================================
const ChapterEntry* MainPlotSystem::CurrentChapter() const {
    auto it = chapter_index_.find(current_chapter_id_);
    if (it != chapter_index_.end()) return it->second;
    return nullptr;
}

// ============================================================================
// 【MainPlotSystem::CurrentNode】获取当前节点
// ============================================================================
const PlotNode* MainPlotSystem::CurrentNode() const {
    const auto* plot = CurrentPlot();
    if (!plot) return nullptr;
    for (const auto& node : plot->nodes) {
        if (node.id == current_node_id_) return &node;
    }
    return nullptr;
}

// ============================================================================
// 【MainPlotSystem::CurrentText】获取当前文本
// ============================================================================
const std::string& MainPlotSystem::CurrentText() const {
    return dialogue_engine_.TypingText();
}

// ============================================================================
// 【MainPlotSystem::CurrentChoices】获取当前选项
// ============================================================================
const std::vector<DialogueChoice>& MainPlotSystem::CurrentChoices() const {
    return dialogue_engine_.CurrentChoices();
}

// ============================================================================
// 【MainPlotSystem::TypingProgress】获取打字进度
// ============================================================================
float MainPlotSystem::TypingProgress() const {
    return dialogue_engine_.TypingProgress();
}

// ============================================================================
// 【MainPlotSystem::IsTyping】是否在打字中
// ============================================================================
bool MainPlotSystem::IsTyping() const {
    return dialogue_engine_.State() == DialogueState::Typing;
}

// ============================================================================
// 【MainPlotSystem::IsChapterUnlocked】章节是否解锁
// ============================================================================
bool MainPlotSystem::IsChapterUnlocked(const std::string& chapter_id) const {
    auto it = chapter_index_.find(chapter_id);
    if (it == chapter_index_.end()) return false;
    const auto& chapter = *it->second;

    if (chapter.chapter_number == 1) return true;

    int day = clock_ ? clock_->Day() : 1;
    Season season = clock_ ? clock_->Season() : Season::Spring;
    int cloud_level = cloud_system_ ? cloud_system_->SpiritEnergy() : 0;

    std::unordered_map<std::string, int> npc_hearts;
    if (npc_heart_getter_) {
        for (const auto id : kNpcIds) {
            int h = npc_heart_getter_(std::string{id});
            if (h > 0) {
                npc_hearts[std::string{id}] = h;
            }
        }
    }

    std::string dummy_loc = "";
    for (const auto& cond : chapter.unlock_conditions) {
        if (!cond.IsSatisfied(day, season, cloud_level, dummy_loc,
                              flags_, npc_hearts, current_route_)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// 【MainPlotSystem::GetChapter】获取章节
// ============================================================================
const ChapterEntry* MainPlotSystem::GetChapter(const std::string& chapter_id) const {
    auto it = chapter_index_.find(chapter_id);
    if (it != chapter_index_.end()) return it->second;
    return nullptr;
}

bool MainPlotSystem::IsPlotCompleted(const std::string& plot_id) const {
    return completed_plots_.contains(plot_id);
}

bool MainPlotSystem::HasFlag(const std::string& flag) const {
    return flags_.contains(flag);
}

// ============================================================================
// 【MainPlotSystem::GetPendingPlots】获取待触发的剧情
// ============================================================================
std::vector<const PlotEntry*> MainPlotSystem::GetPendingPlots() const {
    return GetTriggerablePlots_();
}

// ============================================================================
// 【MainPlotSystem::GetChapterNotice】获取章节提示
// ============================================================================
std::string MainPlotSystem::GetChapterNotice() const {
    if (current_chapter_id_.empty()) return "";
    const auto* ch = GetChapter(current_chapter_id_);
    if (!ch) return "";
    return ch->title + "：" + ch->subtitle;
}

// ============================================================================
// 【MainPlotSystem::GetUpcomingPlots】获取即将到来的剧情
// ============================================================================
std::vector<const PlotEntry*> MainPlotSystem::GetUpcomingPlots(int max_count) const {
    auto all = GetTriggerablePlots_();
    if (static_cast<int>(all.size()) > max_count) {
        all.resize(max_count);
    }
    return all;
}

// ============================================================================
// 【MainPlotSystem::GetUpcomingNotice】获取即将到来的剧情通知
// ============================================================================
std::string MainPlotSystem::GetUpcomingNotice() const {
    auto upcoming = GetUpcomingPlots(3);
    if (upcoming.empty()) return "";

    std::ostringstream oss;
    oss << "即将到来：";
    for (const auto* plot : upcoming) {
        const auto* chapter = GetChapter(plot->chapter_id);
        if (chapter) {
            oss << "【" << chapter->title << "】" << plot->title;
        }
    }
    return oss.str();
}

// ============================================================================
// 【MainPlotSystem::DebugForceStartPlot】Debug强制开始剧情
// ============================================================================
void MainPlotSystem::DebugForceStartPlot(const std::string& plot_id) {
    StartPlot(plot_id);
}

// ============================================================================
// 【MainPlotSystem::DebugForceStartChapter】Debug强制开始章节
// ============================================================================
void MainPlotSystem::DebugForceStartChapter(const std::string& chapter_id) {
    StartChapter(chapter_id);
}

// ============================================================================
// 【MainPlotSystem::DebugSetRoute】Debug设置路线
// ============================================================================
void MainPlotSystem::DebugSetRoute(PlotRoute route) {
    LockRoute_(route);
}

// ============================================================================
// 【MainPlotSystem::DebugSetFlag】Debug设置flag
// ============================================================================
void MainPlotSystem::DebugSetFlag(const std::string& flag) {
    flags_.insert(flag);
}

// ============================================================================
// 【MainPlotSystem::DebugResetAllProgress】Debug重置所有进度
// ============================================================================
void MainPlotSystem::DebugResetAllProgress() {
    flags_.clear();
    completed_plots_.clear();
    completed_chapters_.clear();
    current_route_ = PlotRoute::None;
    current_chapter_id_.clear();
    current_plot_id_.clear();
    current_node_id_.clear();
    state_ = PlotState::Idle;
    dialogue_engine_.EndDialogue();
}

// ============================================================================
// 【MainPlotSystem::SaveState】保存状态
// ============================================================================
void MainPlotSystem::SaveState(std::vector<std::string>& lines) const {
    lines.emplace_back("main_plot|route|" + std::string(RouteName(current_route_)));

    for (const auto& ch : completed_chapters_) {
        lines.emplace_back("main_plot|chapter_completed|" + ch);
    }

    for (const auto& plot : completed_plots_) {
        lines.emplace_back("main_plot|plot_completed|" + plot);
    }

    for (const auto& flag : flags_) {
        lines.emplace_back("main_plot|flag|" + flag);
    }
}

// ============================================================================
// 【MainPlotSystem::LoadState】加载状态
// ============================================================================
void MainPlotSystem::LoadState(const std::vector<std::string>& lines) {
    current_route_ = PlotRoute::None;
    completed_chapters_.clear();
    completed_plots_.clear();
    flags_.clear();

    for (const auto& line : lines) {
        if (line.rfind("main_plot|", 0) != 0) continue;

        std::vector<std::string> parts;
        std::string::size_type start = 10;
        while (start < line.size()) {
            auto end = line.find('|', start);
            if (end == std::string::npos) {
                parts.push_back(line.substr(start));
                break;
            }
            parts.push_back(line.substr(start, end - start));
            start = end + 1;
        }
        if (parts.size() < 2) continue;

        const auto& key = parts[0];
        const auto& value = parts[1];

        if (key == "route") {
            current_route_ = PlotRouteFromString(value);
        } else if (key == "chapter_completed") {
            completed_chapters_.insert(value);
        } else if (key == "plot_completed") {
            completed_plots_.insert(value);
        } else if (key == "flag") {
            flags_.insert(value);
        }
    }
}

}  // namespace CloudSeamanor::engine
