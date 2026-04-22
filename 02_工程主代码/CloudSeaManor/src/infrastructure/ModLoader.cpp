#include "CloudSeamanor/ModApi.hpp"

#include "CloudSeamanor/JsonValue.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <fstream>

namespace CloudSeamanor::engine {

namespace {

std::string ReadAllText(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return {};
    std::string content;
    in.seekg(0, std::ios::end);
    const std::streampos size = in.tellg();
    if (size <= 0) return {};
    content.resize(static_cast<std::size_t>(size));
    in.seekg(0, std::ios::beg);
    in.read(content.data(), size);
    return content;
}

}  // namespace

void ModLoader::RegisterHook(const std::string& hook_name) {
    hooks_.push_back(hook_name);
}

const std::vector<std::string>& ModLoader::Hooks() const {
    return hooks_;
}

bool ModLoader::LoadAll(const std::filesystem::path& mods_root) {
    loaded_mods_.clear();
    mod_root_by_id_.clear();

    std::error_code ec;
    if (!std::filesystem::exists(mods_root, ec)) {
        CloudSeamanor::infrastructure::Logger::Info(
            "ModLoader: mods root not found, skip. path=" + mods_root.string());
        return true;
    }
    if (!std::filesystem::is_directory(mods_root, ec)) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "ModLoader: mods root is not a directory. path=" + mods_root.string());
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(mods_root, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        const auto mod_dir = entry.path();
        const auto manifest_path = mod_dir / "mod.json";
        if (!std::filesystem::exists(manifest_path, ec)) {
            continue;
        }

        const std::string json_text = ReadAllText(manifest_path);
        if (json_text.empty()) {
            CloudSeamanor::infrastructure::Logger::Warning(
                "ModLoader: mod.json empty, skip. path=" + manifest_path.string());
            continue;
        }

        const auto root = CloudSeamanor::infrastructure::JsonValue::Parse(json_text);
        if (!root.IsObject()) {
            CloudSeamanor::infrastructure::Logger::Warning(
                "ModLoader: mod.json root is not object, skip. path=" + manifest_path.string());
            continue;
        }

        ModManifest m;
        if (const auto* v = root.Get("id"); v && v->IsString()) m.id = v->AsString();
        if (const auto* v = root.Get("name"); v && v->IsString()) m.name = v->AsString();
        if (const auto* v = root.Get("version"); v && v->IsString()) m.version = v->AsString();

        if (m.id.empty()) {
            CloudSeamanor::infrastructure::Logger::Warning(
                "ModLoader: mod.json missing id, skip. path=" + manifest_path.string());
            continue;
        }

        if (const auto* dirs = root.Get("data_dirs"); dirs && dirs->IsArray()) {
            for (const auto& it : dirs->AsArray()) {
                if (it.IsString() && !it.AsString().empty()) {
                    m.data_dirs.push_back(it.AsString());
                }
            }
        }
        if (const auto* hooks = root.Get("hooks"); hooks && hooks->IsArray()) {
            for (const auto& it : hooks->AsArray()) {
                if (it.IsString() && !it.AsString().empty()) {
                    m.hooks.push_back(it.AsString());
                }
            }
        }

        // 记录 hook 到全局 hooks 列表（用于运行时提示/调试）
        for (const auto& h : m.hooks) {
            RegisterHook(h);
        }

        mod_root_by_id_[m.id] = mod_dir;
        loaded_mods_.push_back(std::move(m));
        CloudSeamanor::infrastructure::Logger::Info(
            "ModLoader: loaded mod id=" + loaded_mods_.back().id
            + " name=" + loaded_mods_.back().name
            + " version=" + loaded_mods_.back().version);
    }

    return true;
}

std::vector<std::filesystem::path> ModLoader::BuildDataRoots(
    const std::filesystem::path& base_data_root) const {
    std::vector<std::filesystem::path> roots;
    roots.push_back(base_data_root);

    for (const auto& m : loaded_mods_) {
        const auto it = mod_root_by_id_.find(m.id);
        if (it == mod_root_by_id_.end()) continue;
        const auto& mod_root = it->second;
        for (const auto& rel : m.data_dirs) {
            if (rel.empty()) continue;
            roots.push_back(mod_root / rel);
        }
    }

    return roots;
}

}  // namespace CloudSeamanor::engine

