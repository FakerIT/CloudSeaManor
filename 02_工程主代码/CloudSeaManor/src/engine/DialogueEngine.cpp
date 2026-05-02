#include "CloudSeamanor/engine/DialogueEngine.hpp"

#include "CloudSeamanor/infrastructure/DialogueJsonParser.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace CloudSeamanor::engine {

// ============================================================================
// 【LoadDialogueFromJson】从 JSON 加载对话表
// ============================================================================
std::vector<DialogueNode> LoadDialogueFromJson(const std::string& json_content) {
    return ConvertDialogueNodes(CloudSeamanor::infrastructure::ParseDialogueNodes(json_content));
}

// ============================================================================
// 【DialogueEngine::SetCallbacks】设置回调
// ============================================================================
void DialogueEngine::SetCallbacks(DialogueCallbacks callbacks) {
    callbacks_ = std::move(callbacks);
}

// ============================================================================
// 【DialogueEngine::StartDialogue】开始对话
// ============================================================================
void DialogueEngine::StartDialogue(const std::vector<DialogueNode>& nodes,
                                  const std::string& start_node_id,
                                  const DialogueContext& ctx) {
    nodes_ = nodes;
    context_ = ctx;
    state_ = DialogueState::Typing;
    typing_skipped_ = false;

    EnterNode_(start_node_id);

    if (callbacks_.on_node_change && current_node_id_ != start_node_id) {
        if (auto* node = CurrentNode()) callbacks_.on_node_change(*node);
    }
}

// ============================================================================
// 【DialogueEngine::StartDialogueById】通过 ID 开始对话
// ============================================================================
void DialogueEngine::StartDialogueById(const std::string& dialogue_table_id,
                                      const std::string& start_node_id,
                                      const DialogueContext& ctx) {
    if (data_root_.empty()) {
        infrastructure::Logger::Warning("DialogueEngine::StartDialogueById: data_root_ 未设置，无法加载对话文件。");
        return;
    }

    std::string full_path = data_root_ + "/" + dialogue_table_id;
    std::ifstream file(full_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("DialogueEngine::StartDialogueById: 对话文件不存在：" + full_path);
        return;
    }

    std::string json_content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

    auto nodes = LoadDialogueFromJson(json_content);
    if (nodes.empty()) {
        infrastructure::Logger::Warning("DialogueEngine::StartDialogueById: 对话文件解析失败或无节点：" + full_path);
        return;
    }

    StartDialogue(nodes, start_node_id, ctx);
}

// ============================================================================
// 【DialogueEngine::EndDialogue】停止对话
// ============================================================================
void DialogueEngine::EndDialogue() {
    state_ = DialogueState::Idle;
    current_text_.clear();
    full_text_.clear();
    current_choices_.clear();
    nodes_.clear();
    typing_char_index_ = 0;
    typing_timer_ = 0.0f;
}

// ============================================================================
// 【DialogueEngine::Update】每帧更新
// ============================================================================
void DialogueEngine::Update(float delta_seconds) {
    if (state_ != DialogueState::Typing) return;
    if (typing_skipped_) return;

    typing_timer_ += delta_seconds * 1000.0f;  // 转换为毫秒

    while (typing_timer_ >= ms_per_char_ && typing_char_index_ < full_text_.size()) {
        typing_timer_ -= static_cast<float>(ms_per_char_);
        ++typing_char_index_;
        current_text_ = full_text_.substr(0, typing_char_index_);

        if (callbacks_.on_text_update) {
            callbacks_.on_text_update(current_text_);
        }
    }

    // 打字完成
    if (typing_char_index_ >= full_text_.size()) {
        state_ = DialogueState::WaitingChoice;
        current_text_ = full_text_;

        if (callbacks_.on_text_update) {
            callbacks_.on_text_update(current_text_);
        }
        if (callbacks_.on_choices_change && !current_choices_.empty()) {
            callbacks_.on_choices_change(current_choices_);
        }
    }
}

// ============================================================================
// 【DialogueEngine::OnConfirm】确认键
// ============================================================================
bool DialogueEngine::OnConfirm() {
    if (state_ == DialogueState::Idle) return false;

    if (state_ == DialogueState::Typing) {
        // 打字中：跳过打字机效果
        SkipTyping();
        return true;
    }

    if (state_ == DialogueState::WaitingChoice) {
        // 有选项时不能直接确认
        if (!current_choices_.empty()) return false;

        // 无选项，自动进入下一节点
        if (!current_node_id_.empty()) {
            auto next = RouteChoice_(DialogueChoice{}, context_);
            if (!next.empty()) {
                EnterNode_(next);
            } else {
                EndDialogue();
                if (callbacks_.on_complete) callbacks_.on_complete();
            }
        }
        return true;
    }

    return false;
}

// ============================================================================
// 【DialogueEngine::SelectChoice】选择选项
// ============================================================================
bool DialogueEngine::SelectChoice(std::size_t choice_index) {
    if (state_ != DialogueState::WaitingChoice) return false;
    if (choice_index >= current_choices_.size()) return false;

    const auto& choice = current_choices_[choice_index];
    const std::string& next_id = RouteChoice_(choice, context_);

    if (next_id.empty() || next_id == "END") {
        EndDialogue();
        if (callbacks_.on_complete) callbacks_.on_complete();
    } else {
        EnterNode_(next_id);
    }
    return true;
}

// ============================================================================
// 【DialogueEngine::SkipTyping】跳过打字机
// ============================================================================
void DialogueEngine::SkipTyping() {
    if (state_ != DialogueState::Typing) return;
    typing_skipped_ = true;
    current_text_ = full_text_;
    state_ = DialogueState::WaitingChoice;

    if (callbacks_.on_text_update) {
        callbacks_.on_text_update(current_text_);
    }
    if (callbacks_.on_choices_change && !current_choices_.empty()) {
        callbacks_.on_choices_change(current_choices_);
    }
}

// ============================================================================
// 【DialogueEngine::IsActive】对话是否进行中
// ============================================================================
bool DialogueEngine::IsActive() const {
    return state_ != DialogueState::Idle && state_ != DialogueState::Completed;
}

// ============================================================================
// 【DialogueEngine::SetTypingSpeed】设置打字机速度
// ============================================================================
void DialogueEngine::SetTypingSpeed(int ms_per_char) {
    ms_per_char_ = std::clamp(ms_per_char, 20, 100);
}

// ============================================================================
// 【DialogueEngine::CurrentNode】获取当前节点
// ============================================================================
const DialogueNode* DialogueEngine::CurrentNode() const {
    for (const auto& n : nodes_) {
        if (n.id == current_node_id_) return &n;
    }
    return nullptr;
}

// ============================================================================
// 【DialogueEngine::TypingProgress】打字进度
// ============================================================================
float DialogueEngine::TypingProgress() const {
    if (full_text_.empty()) return 1.0f;
    return static_cast<float>(typing_char_index_) / static_cast<float>(full_text_.size());
}

// ============================================================================
// 【DialogueEngine::RouteChoice_】路由选择
// ============================================================================
std::string DialogueEngine::RouteChoice_(const DialogueChoice& choice,
                                         const DialogueContext& ctx) const {
    const bool favor_ok = !choice.min_favor.has_value() || ctx.player_favor >= *choice.min_favor;
    const bool item_ok = !choice.require_item.has_value() || ctx.has_item;
    if (!favor_ok || !item_ok) {
        if (!choice.fallback_next_node_id.empty()) {
            return choice.fallback_next_node_id;
        }
    }
    return choice.next_node_id;
}

// ============================================================================
// 【DialogueEngine::EnterNode_】进入新节点
// ============================================================================
void DialogueEngine::EnterNode_(const std::string& node_id) {
    current_node_id_ = node_id;
    current_choices_.clear();
    typing_timer_ = 0.0f;
    typing_char_index_ = 0;
    typing_skipped_ = false;

    const DialogueNode* node = nullptr;
    for (const auto& n : nodes_) {
        if (n.id == node_id) {
            node = &n;
            break;
        }
    }

    if (!node) {
        CloudSeamanor::infrastructure::Logger::Warning("DialogueEngine: 节点 '" + node_id + "' 不存在，对话结束。");
        EndDialogue();
        if (callbacks_.on_complete) callbacks_.on_complete();
        return;
    }

    // 替换特殊标记
    full_text_ = ReplaceTokens(node->text, context_);
    current_text_.clear();

    // 复制选项
    current_choices_ = node->choices;

    // 立即进入打字状态
    state_ = DialogueState::Typing;

    if (callbacks_.on_node_change) {
        callbacks_.on_node_change(*node);
    }
}

// ============================================================================
// 【DialogueEngine::ReplaceTokens】替换特殊标记
// ============================================================================
std::string DialogueEngine::ReplaceTokens(const std::string& raw,
                                          const DialogueContext& ctx) const {
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
    replace_all(result, "$[NPC_NAME]", ctx.npc_name);
    replace_all(result, "$[WEATHER]", ctx.current_weather);
    replace_all(result, "$[SEASON]", ctx.current_season);
    replace_all(result, "$[DAY]", std::to_string(ctx.current_day));
    replace_all(result, "$[ITEM_NAME]", ctx.item_name);

    return result;
}

}  // namespace CloudSeamanor::engine
