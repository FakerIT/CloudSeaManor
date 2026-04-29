#include "../TestFramework.hpp"
#include "CloudSeamanor/engine/UiLayoutSystem.hpp"

using CloudSeamanor::engine::UiLayoutSerializer;
using CloudSeamanor::engine::UiPageConfig;

TEST_CASE(UiLayoutSerializer_roundtrip_page_header_fields) {
    UiPageConfig cfg;
    cfg.page_id = "inventory";
    cfg.title = "背包";
    cfg.width = 640;
    cfg.height = 480;
    cfg.modal = true;
    cfg.pause_game = true;
    cfg.root_element_ids = {"slot_1", "slot_2"};

    const std::string json = UiLayoutSerializer::SerializePage(cfg);
    const auto parsed = UiLayoutSerializer::ParsePage(json);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->page_id, "inventory");
    ASSERT_EQ(parsed->title, "背包");
    ASSERT_EQ(parsed->width, 640);
    ASSERT_EQ(parsed->height, 480);
    ASSERT_TRUE(parsed->modal);
    ASSERT_TRUE(parsed->pause_game);
    ASSERT_EQ(parsed->root_element_ids.size(), static_cast<std::size_t>(2));
    return true;
}

