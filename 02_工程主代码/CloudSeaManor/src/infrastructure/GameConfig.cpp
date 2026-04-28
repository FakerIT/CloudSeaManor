#include "CloudSeamanor/GameConfig.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <exception>
#include <fstream>
#include <string>

namespace CloudSeamanor::infrastructure {

namespace {

void Trim(std::string& s) {
    const std::size_t first_non_space = s.find_first_not_of(" \t\r");
    if (first_non_space == std::string::npos) {
        s.clear();
        return;
    }

    const std::size_t last_non_space = s.find_last_not_of(" \t\r");
    s.erase(last_non_space + 1);
    s.erase(0, first_non_space);
}

}  // namespace

bool GameConfig::LoadFromFile(const std::string& path) {
    const auto result = LoadFromFileResult(path);
    if (!result.Ok()) {
        Logger::Error("GameConfig: " + result.Error());
        return false;
    }
    return true;
}

CloudSeamanor::Result<void> GameConfig::LoadFromFileResult(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return "无法打开配置文件: " + path;
    }

    float_values_.clear();
    string_values_.clear();
    default_fallback_count_ = 0;

    std::string line;
    int line_no = 0;
    int parsed_count = 0;
    int ignored_count = 0;
    while (std::getline(stream, line)) {
        ++line_no;
        Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // 字符串格式: key="value"
        const auto quote_pos = line.find("=\"");
        if (quote_pos != std::string::npos) {
            std::string key = line.substr(0, quote_pos);
            Trim(key);
            const auto close_quote = line.find_last_of('"');
            if (close_quote != std::string::npos && close_quote > quote_pos + 1) {
                string_values_[key] = line.substr(quote_pos + 2, close_quote - quote_pos - 2);
                ++parsed_count;
            } else {
                ++ignored_count;
                Logger::Warning(
                    "GameConfig: 字符串配置格式错误（缺少闭合引号），行 " + std::to_string(line_no) + ": " + line);
            }
            continue;
        }

        // 浮点格式: key=value
        const auto equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            ++ignored_count;
            Logger::Warning(
                "GameConfig: 无法解析的配置行（缺少 '='），行 " + std::to_string(line_no) + ": " + line);
            continue;
        }
        std::string key = line.substr(0, equal_pos);
        std::string value_text = line.substr(equal_pos + 1);
        Trim(key);
        Trim(value_text);

        try {
            float_values_[key] = std::stof(value_text);
            ++parsed_count;
        } catch (const std::invalid_argument& ex) {
            ++ignored_count;
            Logger::Warning(
                "GameConfig: 浮点配置解析失败（invalid_argument），行 " + std::to_string(line_no)
                + ": key=" + key + ", value=" + value_text + ", error=" + ex.what());
        } catch (const std::out_of_range& ex) {
            ++ignored_count;
            Logger::Warning(
                "GameConfig: 浮点配置解析失败（out_of_range），行 " + std::to_string(line_no)
                + ": key=" + key + ", value=" + value_text + ", error=" + ex.what());
        } catch (const std::exception& ex) {
            ++ignored_count;
            Logger::Warning(
                "GameConfig: 浮点配置解析失败（exception），行 " + std::to_string(line_no)
                + ": key=" + key + ", value=" + value_text + ", error=" + ex.what());
        }
    }

    Logger::Info(
        "GameConfig: 加载完成 path=" + path +
        "，有效项=" + std::to_string(parsed_count) +
        "，忽略项=" + std::to_string(ignored_count));
    return {};
}

float GameConfig::GetFloat(const std::string& key, float fallback) const {
    const auto it = float_values_.find(key);
    if (it == float_values_.end()) {
        ++default_fallback_count_;
        Logger::Warning(
            "[Config] " + key + " 无效 (value=<missing>), 使用默认值 " + std::to_string(fallback)
            + " [defaults_used=" + std::to_string(default_fallback_count_) + "]");
        return fallback;
    }
    return it->second;
}

std::string GameConfig::GetString(const std::string& key,
                                    const std::string& fallback) const {
    const auto it = string_values_.find(key);
    if (it == string_values_.end()) {
        ++default_fallback_count_;
        Logger::Warning(
            "[Config] " + key + " 无效 (value=<missing>), 使用默认值 \"" + fallback + "\""
            + " [defaults_used=" + std::to_string(default_fallback_count_) + "]");
        return fallback;
    }
    return it->second;
}

}  // namespace CloudSeamanor::infrastructure
