#pragma once

#include "CloudSeamanor/Result.hpp"

#include <string>
#include <unordered_map>

namespace CloudSeamanor::infrastructure {

// 游戏配置读取器。
// 支持浮点（float）和字符串（string）两种配置项。
class GameConfig {
public:
    // 从文本文件加载配置。
    // 支持 key=value（浮点）和 key="value"（字符串）两种格式。
    bool LoadFromFile(const std::string& path);
    [[nodiscard]] CloudSeamanor::Result<void> LoadFromFileResult(const std::string& path);

    // 获取浮点配置项，不存在则返回 fallback。
    [[nodiscard]] float GetFloat(const std::string& key, float fallback) const;

    // 获取字符串配置项，不存在则返回 fallback。
    [[nodiscard]] std::string GetString(const std::string& key,
                                          const std::string& fallback) const;
    [[nodiscard]] int DefaultFallbackCount() const { return default_fallback_count_; }

private:
    std::unordered_map<std::string, float> float_values_;
    std::unordered_map<std::string, std::string> string_values_;
    mutable int default_fallback_count_ = 0;
};

}  // namespace CloudSeamanor::infrastructure
