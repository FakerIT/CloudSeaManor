#pragma once

// ============================================================================
// 【DialogueJsonParser】Minimal JSON dialogue parser (infrastructure)
// ============================================================================
// Responsibilities:
// - Parse simplified dialogue JSON into plain data structs (no engine dependency)
// - Provide stable parsing for "nodes" and "choices" blocks without third-party JSON
// ============================================================================

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

struct DialogueChoiceData {
    std::string id;
    std::string text;
    std::string next_node_id;
    std::optional<int> min_favor;
    std::optional<std::string> require_item;
    std::string fallback_next_node_id;
};

struct DialogueNodeData {
    std::string id;
    std::string speaker;
    std::string text;
    std::vector<DialogueChoiceData> choices;
};

/**
 * @brief Parse dialogue nodes from simplified JSON content.
 * @note Expected shape:
 *   { "nodes": [ { "id":"...", "speaker":"...", "text":"...", "choices":[ ... ] }, ... ] }
 *       "choices" items support:
 *         - id, text, next
 *         - optional: min_favor, has_item, fallback
 *         - optional: condition { has_item, min_favor }
 */
[[nodiscard]] std::vector<DialogueNodeData> ParseDialogueNodes(const std::string& json);

}  // namespace CloudSeamanor::infrastructure

