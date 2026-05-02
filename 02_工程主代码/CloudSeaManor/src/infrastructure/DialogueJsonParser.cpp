#include "CloudSeamanor/infrastructure/DialogueJsonParser.hpp"

// ============================================================================
// 架构豁免声明（P18-QUAL-006）
// ============================================================================
// 此文件包含 engine/DialogueEngine.hpp 是经过设计的特例：
// - ConvertDialogueNodes 函数位于 infrastructure 层，但其目的是供 engine 层共用
// - 这是 CQ-103 消除重复代码的解决方案
// - forward declaration 已在头文件中处理，此处 include 仅用于 .cpp 实现
// ============================================================================
#include "CloudSeamanor/engine/DialogueEngine.hpp"

#include <cctype>
#include <exception>
#include <stdexcept>
#include <string_view>

namespace CloudSeamanor::infrastructure {

namespace {

std::string::size_type FindKey_(const std::string& s, const std::string& key) {
    return s.find("\"" + key + "\"");
}

std::pair<std::string, std::string::size_type> ExtractString_(const std::string& s, std::string::size_type pos) {
    const auto start = s.find('"', pos);
    if (start == std::string::npos) return {"", std::string::npos};
    const auto end = s.find('"', start + 1);
    if (end == std::string::npos) return {"", std::string::npos};
    return {s.substr(start + 1, end - start - 1), end + 1};
}

std::optional<int> ExtractIntField_(const std::string& s, const std::string& field) {
    const auto field_pos = FindKey_(s, field);
    if (field_pos == std::string::npos) return std::nullopt;
    const auto colon = s.find(':', field_pos);
    if (colon == std::string::npos) return std::nullopt;
    auto p = colon + 1;
    while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p])) != 0) ++p;
    const auto number_start = s.find_first_of("0123456789", p);
    if (number_start == std::string::npos) return std::nullopt;
    const auto number_end = s.find_first_not_of("0123456789", number_start);
    try {
        return std::stoi(s.substr(number_start, number_end - number_start));
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::string> ExtractStringField_(const std::string& s, const std::string& field) {
    const auto field_pos = FindKey_(s, field);
    if (field_pos == std::string::npos) return std::nullopt;
    auto [v, _e] = ExtractString_(s, field_pos);
    if (v.empty()) return std::nullopt;
    return v;
}

}  // namespace

std::vector<DialogueNodeData> ParseDialogueNodes(const std::string& json) {
    std::vector<DialogueNodeData> nodes;

    const auto nodes_pos = FindKey_(json, "nodes");
    if (nodes_pos == std::string::npos) return nodes;
    const auto arr_start = json.find('[', nodes_pos);
    if (arr_start == std::string::npos) return nodes;

    std::string::size_type pos = arr_start + 1;
    while (pos < json.size()) {
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) ++pos;
        if (pos >= json.size() || json[pos] != '{') break;

        int brace_count = 1;
        std::string::size_type obj_end = pos + 1;
        while (obj_end < json.size() && brace_count > 0) {
            if (json[obj_end] == '{') ++brace_count;
            else if (json[obj_end] == '}') --brace_count;
            ++obj_end;
        }

        const std::string obj = json.substr(pos, obj_end - pos);
        DialogueNodeData node;

        {
            auto [id_val, _e0] = ExtractString_(obj, FindKey_(obj, "id"));
            node.id = id_val;
        }
        {
            auto [speaker_val, _e1] = ExtractString_(obj, FindKey_(obj, "speaker"));
            node.speaker = speaker_val;
        }
        {
            auto [text_val, _e2] = ExtractString_(obj, FindKey_(obj, "text"));
            node.text = text_val;
        }

        // choices array
        const auto choices_pos = FindKey_(obj, "choices");
        if (choices_pos != std::string::npos) {
            const auto choices_arr_start = obj.find('[', choices_pos);
            const auto choices_arr_end = obj.find(']', choices_arr_start);
            if (choices_arr_start != std::string::npos && choices_arr_end != std::string::npos) {
                const std::string choices_str = obj.substr(choices_arr_start + 1,
                                                           choices_arr_end - choices_arr_start - 1);

                std::string::size_type choice_pos = 0;
                while (choice_pos < choices_str.size()) {
                    while (choice_pos < choices_str.size()
                           && std::isspace(static_cast<unsigned char>(choices_str[choice_pos])) != 0) {
                        ++choice_pos;
                    }
                    if (choice_pos >= choices_str.size() || choices_str[choice_pos] != '{') break;

                    int choice_braces = 1;
                    std::string::size_type choice_end = choice_pos + 1;
                    while (choice_end < choices_str.size() && choice_braces > 0) {
                        if (choices_str[choice_end] == '{') ++choice_braces;
                        else if (choices_str[choice_end] == '}') --choice_braces;
                        ++choice_end;
                    }

                    const std::string choice_obj = choices_str.substr(choice_pos, choice_end - choice_pos);
                    DialogueChoiceData ch;

                    if (auto v = ExtractStringField_(choice_obj, "id")) ch.id = *v;
                    if (auto v = ExtractStringField_(choice_obj, "text")) ch.text = *v;
                    if (auto v = ExtractStringField_(choice_obj, "next")) ch.next_node_id = *v;

                    ch.min_favor = ExtractIntField_(choice_obj, "min_favor");
                    if (auto v = ExtractStringField_(choice_obj, "has_item")) ch.require_item = *v;
                    if (auto v = ExtractStringField_(choice_obj, "fallback")) ch.fallback_next_node_id = *v;

                    // condition object: { "condition": { ... } }
                    const auto condition_pos = FindKey_(choice_obj, "condition");
                    if (condition_pos != std::string::npos) {
                        const auto cond_open = choice_obj.find('{', condition_pos);
                        const auto cond_close = choice_obj.find('}', cond_open);
                        if (cond_open != std::string::npos && cond_close != std::string::npos) {
                            const std::string cond_obj = choice_obj.substr(cond_open, cond_close - cond_open + 1);
                            if (!ch.require_item.has_value()) {
                                if (auto v = ExtractStringField_(cond_obj, "has_item")) ch.require_item = *v;
                            }
                            if (!ch.min_favor.has_value()) {
                                ch.min_favor = ExtractIntField_(cond_obj, "min_favor");
                            }
                        }
                    }

                    if (!ch.id.empty()) node.choices.push_back(std::move(ch));
                    choice_pos = choice_end;
                }
            }
        }

        if (!node.id.empty()) nodes.push_back(std::move(node));
        pos = obj_end;
    }

    return nodes;
}

}  // namespace CloudSeamanor::infrastructure

// ============================================================================
// 【ConvertDialogueNodes】infrastructure 级别实现，供 engine 层共用
// ============================================================================
std::vector<CloudSeamanor::engine::DialogueNode> CloudSeamanor::infrastructure::ConvertDialogueNodes(
    const std::vector<CloudSeamanor::infrastructure::DialogueNodeData>& data_nodes) {
    std::vector<CloudSeamanor::engine::DialogueNode> out;
    out.reserve(data_nodes.size());
    for (const auto& dn : data_nodes) {
        CloudSeamanor::engine::DialogueNode n;
        n.id = dn.id;
        n.speaker = dn.speaker;
        n.text = dn.text;
        n.choices.reserve(dn.choices.size());
        for (const auto& dc : dn.choices) {
            CloudSeamanor::engine::DialogueChoice c;
            c.id = dc.id;
            c.text = dc.text;
            c.next_node_id = dc.next_node_id;
            c.min_favor = dc.min_favor;
            c.require_item = dc.require_item;
            c.fallback_next_node_id = dc.fallback_next_node_id;
            n.choices.push_back(std::move(c));
        }
        if (!n.id.empty()) {
            out.push_back(std::move(n));
        }
    }
    return out;
}

