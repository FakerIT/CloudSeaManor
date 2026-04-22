#include "CloudSeamanor/SaveSlotManager.hpp"

#include <cctype>
#include <fstream>

namespace CloudSeamanor::infrastructure {

namespace {

std::string Trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

}  // namespace

SaveSlotManager::SaveSlotManager(std::filesystem::path root_dir)
    : root_dir_(std::move(root_dir)) {
}

bool SaveSlotManager::IsValidSlot(int slot_index) const {
    return slot_index >= 1 && slot_index <= 3;
}

std::filesystem::path SaveSlotManager::BuildSlotPath(int slot_index) const {
    return root_dir_ / ("save_slot_0" + std::to_string(slot_index) + ".txt");
}

std::filesystem::path SaveSlotManager::BuildMetaPath(int slot_index) const {
    return root_dir_ / ("save_slot_0" + std::to_string(slot_index) + ".meta");
}

void SaveSlotManager::EnsureDirectories() const {
    std::filesystem::create_directories(root_dir_);
}

bool SaveSlotManager::WriteMetadata(int slot_index, const SaveSlotMetadata& metadata) const {
    if (!IsValidSlot(slot_index)) {
        return false;
    }
    EnsureDirectories();
    std::ofstream out(BuildMetaPath(slot_index), std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "saved_at=" << metadata.saved_at_text << '\n';
    out << "day=" << metadata.day << '\n';
    out << "season=" << metadata.season_text << '\n';
    out << "thumbnail=" << metadata.thumbnail_path << '\n';
    return out.good();
}

SaveSlotMetadata SaveSlotManager::ReadMetadata(int slot_index) const {
    SaveSlotMetadata metadata;
    metadata.slot_index = slot_index;
    if (!IsValidSlot(slot_index)) {
        return metadata;
    }
    metadata.exists = std::filesystem::exists(BuildSlotPath(slot_index));

    std::ifstream in(BuildMetaPath(slot_index));
    if (!in.is_open()) {
        return metadata;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));
        if (key == "saved_at") {
            metadata.saved_at_text = value;
        } else if (key == "day") {
            try { metadata.day = std::stoi(value); } catch (...) {}
        } else if (key == "season") {
            metadata.season_text = value;
        } else if (key == "thumbnail") {
            metadata.thumbnail_path = value;
        }
    }
    return metadata;
}

std::vector<SaveSlotMetadata> SaveSlotManager::ReadAllMetadata() const {
    std::vector<SaveSlotMetadata> result;
    result.reserve(3);
    for (int slot = 1; slot <= 3; ++slot) {
        result.push_back(ReadMetadata(slot));
    }
    return result;
}

}  // namespace CloudSeamanor::infrastructure
