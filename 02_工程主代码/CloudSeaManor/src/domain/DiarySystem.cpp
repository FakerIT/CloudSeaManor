#include "CloudSeamanor/DiarySystem.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

namespace {

std::string Trim_(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

std::vector<std::string> SplitCsvLine_(const std::string& line) {
    std::vector<std::string> out;
    std::string current;
    bool in_quotes = false;
    for (char ch : line) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (ch == ',' && !in_quotes) {
            out.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    out.push_back(current);
    return out;
}

} // namespace

bool DiarySystem::LoadFromFile(const std::string& csv_path) {
    std::ifstream in(csv_path);
    if (!in.is_open()) {
        cache_ = CreateDefaultDefinitions();
        return false;
    }

    std::vector<DiaryDefinition> loaded;
    std::string line;
    bool is_header = true;
    while (std::getline(in, line)) {
        const std::string trimmed = Trim_(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        if (is_header) {
            is_header = false;
            continue;
        }

        const auto cells = SplitCsvLine_(line);
        if (cells.size() < 3) {
            continue;
        }

        DiaryDefinition definition;
        definition.id = Trim_(cells[0]);
        definition.title = Trim_(cells[1]);
        definition.summary = Trim_(cells[2]);
        if (!definition.id.empty()) {
            loaded.push_back(std::move(definition));
        }
    }

    if (loaded.empty()) {
        cache_ = CreateDefaultDefinitions();
        return false;
    }

    cache_ = std::move(loaded);
    return true;
}

std::vector<DiaryDefinition> DiarySystem::CreateDefaultDefinitions() const {
    return {
        {"harvest_first", "茶田初收", "第一批茶叶终于收成，山庄开始有了真正属于自己的节奏。"},
        {"purify_return", "净化回响", "灵界净化后的余波回流山庄，茶园在接下来的几天会更有生机。"},
        {"festival_memory", "节日留影", "节庆让平凡的一天被记住，山庄开始拥有自己的年历。"},
        {"favor_bond", "云上知音", "有人开始把你当作重要的人，云海里的关系不再只是过客。"},
    };
}

bool DiarySystem::UnlockOnce(const DiaryDefinition& def,
                             int current_day,
                             std::vector<CloudSeamanor::engine::DiaryEntryState>& entries) const {
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& e) {
        return e.entry_id == def.id;
    });
    if (it != entries.end()) {
        return false;
    }
    entries.push_back({def.id, std::max(1, current_day), false});
    return true;
}

const DiaryDefinition* DiarySystem::FindById(const std::string& id) const {
    if (cache_.empty()) {
        const_cast<DiarySystem*>(this)->LoadFromFile();
    }
    const auto it = std::find_if(cache_.begin(), cache_.end(), [&](const auto& e) {
        return e.id == id;
    });
    return it == cache_.end() ? nullptr : &(*it);
}

} // namespace CloudSeamanor::domain
