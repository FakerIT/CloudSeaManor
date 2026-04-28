#pragma once

// ============================================================================
// 【DialogueJsonParser】Minimal JSON dialogue parser (infrastructure)
// ============================================================================
// Responsibilities:
// - Parse simplified dialogue JSON into plain data structs (no engine dependency)
// - Provide stable parsing for "nodes" and "choices" blocks without third-party JSON
//
// CQ-103: ConvertDialogueNodes defined here to eliminate duplication between
// DialogueEngine.cpp and NpcDialogueManager.cpp (each previously had an identical copy).
// ============================================================================

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor {

// Forward-declare engine types to avoid including DialogueEngine.hpp here
// (prevents infrastructure ↔ engine circular dependency at header level).
namespace engine { class DialogueNode; }

namespace infrastructure {

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

/**
 * @brief Convert raw ParseDialogueNodes output to engine-level DialogueNode/Choice structs.
 * @note Used by both DialogueEngine and NpcDialogueManager — defined once here to eliminate CQ-103 duplication.
 */
[[nodiscard]] std::vector<CloudSeamanor::engine::DialogueNode> ConvertDialogueNodes(
    const std::vector<DialogueNodeData>& data_nodes);

}  // namespace CloudSeamanor::infrastructure
}  // namespace CloudSeamanor
