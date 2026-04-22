#include "catch2Compat.hpp"

#include "CloudSeamanor/DialogueEngine.hpp"

using CloudSeamanor::engine::DialogueChoice;
using CloudSeamanor::engine::DialogueContext;
using CloudSeamanor::engine::DialogueEngine;
using CloudSeamanor::engine::DialogueNode;
using CloudSeamanor::engine::DialogueState;

namespace {

std::vector<DialogueNode> BuildBranchDialogue() {
    DialogueNode start;
    start.id = "start";
    start.speaker = "阿茶";
    start.text = "你好，$[PLAYER_NAME]";
    start.choices = {
        {"c1", "高好感分支", "high", 5, std::nullopt, "low"},
        {"c2", "有物品分支", "item", std::nullopt, std::string("TeaLeaf"), "no_item"},
    };

    DialogueNode high;
    high.id = "high";
    high.text = "高好感";

    DialogueNode low;
    low.id = "low";
    low.text = "低好感";

    DialogueNode item;
    item.id = "item";
    item.text = "你带了物品";

    DialogueNode no_item;
    no_item.id = "no_item";
    no_item.text = "你没带物品";

    return {start, high, low, item, no_item};
}

}  // namespace

TEST_CASE("DialogueEngine::StartDialogue enters typing and updates node") {
    DialogueEngine engine;
    const auto nodes = BuildBranchDialogue();
    DialogueContext ctx;
    ctx.player_name = "测试者";

    bool node_changed = false;
    engine.SetCallbacks({
        .on_text_update = {},
        .on_node_change = [&](const DialogueNode& node) { node_changed = (node.id == "start"); },
        .on_choices_change = {},
        .on_complete = {},
        .on_favor_change = {}
    });

    engine.StartDialogue(nodes, "start", ctx);

    CHECK_TRUE(engine.IsActive());
    CHECK_EQ(engine.State(), DialogueState::Typing);
    CHECK_TRUE(node_changed);
}

TEST_CASE("DialogueEngine::OnConfirm skips typing and enters waiting choice") {
    DialogueEngine engine;
    const auto nodes = BuildBranchDialogue();
    DialogueContext ctx;
    ctx.player_name = "测试者";

    engine.StartDialogue(nodes, "start", ctx);
    const bool consumed = engine.OnConfirm();

    CHECK_TRUE(consumed);
    CHECK_EQ(engine.State(), DialogueState::WaitingChoice);
    CHECK_THAT(engine.CurrentText(), Contains("测试者"));
    CHECK_EQ(engine.CurrentChoices().size(), 2u);
}

TEST_CASE("DialogueEngine::SelectChoice routes by favor and fallback") {
    DialogueEngine engine;
    const auto nodes = BuildBranchDialogue();
    DialogueContext ctx;
    ctx.player_name = "测试者";
    ctx.player_favor = 2;  // below min_favor 5

    engine.StartDialogue(nodes, "start", ctx);
    engine.OnConfirm();
    const bool ok = engine.SelectChoice(0);

    CHECK_TRUE(ok);
    REQUIRE(engine.CurrentNode() != nullptr);
    CHECK_THAT(engine.CurrentNode()->id, Equals("low"));
}

TEST_CASE("DialogueEngine::SelectChoice routes by has_item condition") {
    DialogueEngine engine;
    const auto nodes = BuildBranchDialogue();
    DialogueContext ctx;
    ctx.player_name = "测试者";
    ctx.has_item = true;

    engine.StartDialogue(nodes, "start", ctx);
    engine.OnConfirm();
    const bool ok = engine.SelectChoice(1);

    CHECK_TRUE(ok);
    REQUIRE(engine.CurrentNode() != nullptr);
    CHECK_THAT(engine.CurrentNode()->id, Equals("item"));
}

TEST_CASE("DialogueEngine::SetTypingSpeed clamps and progresses by update") {
    DialogueEngine engine;
    DialogueNode start;
    start.id = "start";
    start.text = "abcd";

    DialogueContext ctx;
    engine.SetTypingSpeed(1);  // should clamp to 20ms
    engine.StartDialogue({start}, "start", ctx);
    engine.Update(0.02f);

    CHECK_TRUE(engine.TypingProgress() > 0.0f);
}

TEST_CASE("DialogueEngine::ReplaceTokens replaces known placeholders") {
    DialogueEngine engine;
    DialogueContext ctx;
    ctx.player_name = "旅人";
    ctx.farm_name = "云海山庄";
    ctx.current_day = 7;
    ctx.current_season = "夏";

    const std::string raw = "你好 $[PLAYER_NAME]，欢迎来到 $[FARM_NAME]，今天是$[SEASON]第$[DAY]天。";
    const std::string replaced = engine.ReplaceTokens(raw, ctx);

    CHECK_THAT(replaced, Contains("旅人"));
    CHECK_THAT(replaced, Contains("云海山庄"));
    CHECK_THAT(replaced, Contains("夏"));
    CHECK_THAT(replaced, Contains("7"));
}

TEST_CASE("DialogueEngine::StartDialogue invalid node ends dialogue") {
    DialogueEngine engine;
    DialogueContext ctx;
    bool completed = false;
    engine.SetCallbacks({
        .on_text_update = {},
        .on_node_change = {},
        .on_choices_change = {},
        .on_complete = [&]() { completed = true; },
        .on_favor_change = {}
    });

    DialogueNode start;
    start.id = "exists";
    start.text = "x";
    engine.StartDialogue({start}, "missing", ctx);

    CHECK_FALSE(engine.IsActive());
    CHECK_TRUE(completed);
}
