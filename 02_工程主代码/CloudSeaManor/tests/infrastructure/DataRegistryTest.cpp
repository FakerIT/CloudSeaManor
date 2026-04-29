#include "CloudSeamanor/infrastructure/DataRegistry.hpp"

#include "TestFramework.hpp"

#include <filesystem>
#include <fstream>
#include <optional>

namespace {

struct TestRow {
    std::string id;
    int value = 0;
};

TEST_CASE(DataRegistry_LoadsHeaderBasedCsvTable) {
    const std::string path = "data_registry_test.csv";
    {
        std::ofstream out(path);
        out << "id,value\n";
        out << "alpha,10\n";
        out << "beta,25\n";
    }

    CloudSeamanor::infrastructure::DataRegistry registry;
    CloudSeamanor::infrastructure::DataTable<TestRow> table(
        [](const TestRow& row) { return row.id; });
    registry.RegisterCsvTable<TestRow>(
        "test_rows",
        path,
        &table,
        [](const CloudSeamanor::infrastructure::CsvRow& row,
           std::vector<CloudSeamanor::infrastructure::DataIssue>&)
            -> std::optional<TestRow> {
            return TestRow{
                .id = row.Get("id"),
                .value = row.GetInt("value"),
            };
        });

    ASSERT_TRUE(registry.LoadAll());
    ASSERT_EQ(table.Size(), 2u);
    ASSERT_TRUE(table.FindById("alpha") != nullptr);
    ASSERT_EQ(table.FindById("beta")->value, 25);
    std::filesystem::remove(path);
    return true;
}

TEST_CASE(DataRegistry_LoadsCommentOnlyHeaderlessCsv) {
    const std::string path = "data_registry_headerless.csv";
    {
        std::ofstream out(path);
        out << "# no header in this file\n";
        out << "spring_awakening,山庄苏醒祭,spring,8\n";
        out << "summer_lantern,云海灯会,summer,12\n";
    }

    CloudSeamanor::infrastructure::CsvDocument document;
    CloudSeamanor::infrastructure::DataRegistry registry;
    ASSERT_TRUE(registry.LoadCsvDocument(path, document, nullptr));
    ASSERT_EQ(document.rows.size(), 2u);
    ASSERT_EQ(document.rows[0].Get("col0"), "spring_awakening");
    ASSERT_EQ(document.rows[1].GetInt("col3"), 12);
    std::filesystem::remove(path);
    return true;
}

} // namespace
