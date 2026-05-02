#include "../catch2Compat.hpp"

#include "CloudSeamanor/ModApi.hpp"

using CloudSeamanor::engine::ModLoader;

TEST_CASE("ModLoader stores registered hooks") {
    ModLoader loader;
    loader.RegisterHook("OnDayChanged");
    loader.RegisterHook("OnInteract");

    REQUIRE(loader.Hooks().size() == 2);
    CHECK(loader.Hooks()[0] == "OnDayChanged");
    CHECK(loader.Hooks()[1] == "OnInteract");
}
