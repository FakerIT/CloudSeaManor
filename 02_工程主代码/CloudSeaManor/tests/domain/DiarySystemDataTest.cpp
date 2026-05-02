#include "CloudSeamanor/domain/DiarySystem.hpp"

#include "TestFramework.hpp"

#include <filesystem>
#include <fstream>

namespace {

TEST_CASE(DiarySystem_LoadsDefinitionsFromCsv) {
    const std::string path = "diary_entries_test.csv";
    {
        std::ofstream out(path);
        out << "id,title,summary\n";
        out << "entry_a,第一条,说明 A\n";
        out << "entry_b,第二条,说明 B\n";
    }

    CloudSeamanor::domain::DiarySystem diary_system;
    ASSERT_TRUE(diary_system.LoadFromFile(path));
    const auto* entry = diary_system.FindById("entry_b");
    ASSERT_TRUE(entry != nullptr);
    ASSERT_EQ(entry->title, "第二条");
    ASSERT_EQ(entry->summary, "说明 B");
    std::filesystem::remove(path);
    return true;
}

} // namespace
