#pragma once

#include "CloudSeamanor/infrastructure/JsonValue.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

// ============================================================================
// 【ReferenceSaveDataSystem】参考存档数据解析与归一化
// ============================================================================
// 目标：
// - 读取第三方参考数据（ES3 风格 __type/value 包装）
// - 归一化为结构化 JSON（便于映射到本项目 domain/engine 模型）
// - 输出数据质量报告（字段缺失、类型异常、槽位不一致）
//
// 当前支持：
// - *.es3（JSON 文本）
// - 聚合键模式：商店日购_槽X_<商品>_(窗口起始|次数)
// ============================================================================
class ReferenceSaveDataSystem {
public:
    // 加载参考存档文件，成功后可调用 BuildNormalizedSnapshot()/BuildValidationReport()
    bool LoadFromFile(const std::string& file_path);

    // 原始 JSON 根节点（解析失败时为 Null）
    [[nodiscard]] const JsonValue& RawRoot() const noexcept { return raw_root_; }

    // 解包 __type/value 后的标准化快照
    [[nodiscard]] JsonValue BuildNormalizedSnapshot() const;
    bool ExportNormalizedSnapshot(const std::filesystem::path& output_path) const;

    // 构建数据质量报告（字符串数组，空数组表示未发现问题）
    [[nodiscard]] std::vector<std::string> BuildValidationReport() const;

private:
    [[nodiscard]] static JsonValue UnwrapEs3TypedNode_(const JsonValue& node);
    [[nodiscard]] static bool ParseDailyShopKey_(
        const std::string& raw_key,
        int* out_slot,
        std::string* out_item_name,
        std::string* out_field_name);

    JsonValue raw_root_;
    std::string source_path_;
};

}  // namespace CloudSeamanor::infrastructure

