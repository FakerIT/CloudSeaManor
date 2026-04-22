#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

class IMod {
public:
    virtual ~IMod() = default;
    virtual std::string Id() const = 0;
    virtual void OnLoad() = 0;
    virtual void OnUpdate(float delta_seconds) = 0;
    virtual void OnRender() = 0;
};

struct ModManifest {
    std::string id;
    std::string name;
    std::string version;
    std::vector<std::string> data_dirs;
    std::vector<std::string> hooks;
};

class ModLoader {
public:
    // 兼容旧用法：只注册 hook 名称（用于运行时提示/调试）。
    void RegisterHook(const std::string& hook_name);
    [[nodiscard]] const std::vector<std::string>& Hooks() const;

    // BE-070：扫描 mods/ 并加载 mod.json。
    // - mods_root 目录下每个子目录视作一个 mod 候选
    // - 读取 mod.json，解析 id/name/version/data_dirs/hooks
    bool LoadAll(const std::filesystem::path& mods_root = "mods");
    [[nodiscard]] const std::vector<ModManifest>& LoadedMods() const { return loaded_mods_; }

    // 返回数据根目录叠加顺序：base_data_root -> mods data_dirs...
    // 供各类数据表读取时按顺序叠加/覆盖。
    [[nodiscard]] std::vector<std::filesystem::path> BuildDataRoots(
        const std::filesystem::path& base_data_root) const;

private:
    std::vector<std::string> hooks_;
    std::vector<ModManifest> loaded_mods_;
    std::unordered_map<std::string, std::filesystem::path> mod_root_by_id_;
};

} // namespace CloudSeamanor::engine
