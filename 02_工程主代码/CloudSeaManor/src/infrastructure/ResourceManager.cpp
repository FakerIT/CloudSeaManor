#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

// ============================================================================
// 【CreateDefaultTexture】创建 1×1 品红色占位纹理
// ============================================================================
std::unique_ptr<sf::Texture> CreateDefaultTexture() {
    auto tex = std::make_unique<sf::Texture>();
    // SFML 3.0: Image 构造函数接受 Vector2u size 和 Color
    sf::Image img(sf::Vector2u(1, 1), sf::Color(255, 0, 255));  // 品红色
    const bool loaded = tex->loadFromImage(img);
    (void)loaded;
    tex->setSmooth(false);
    return tex;
}

// ============================================================================
// 【CreateDefaultFont】创建默认字体占位符
// ============================================================================
std::unique_ptr<sf::Font> CreateDefaultFont() {
    return std::make_unique<sf::Font>();
}

// ============================================================================
// 【BuildStr】简单的字符串拼接辅助（避免引入 fmt 依赖）
// ============================================================================
inline std::string BuildStr() { return {}; }
template <typename T0>
std::string BuildStr(const T0& v0) {
    std::ostringstream oss;
    oss << v0;
    return oss.str();
}
template <typename T0, typename T1>
std::string BuildStr(const T0& v0, const T1& v1) {
    std::ostringstream oss;
    oss << v0 << v1;
    return oss.str();
}
template <typename T0, typename T1, typename T2>
std::string BuildStr(const T0& v0, const T1& v1, const T2& v2) {
    std::ostringstream oss;
    oss << v0 << v1 << v2;
    return oss.str();
}
template <typename T0, typename T1, typename T2, typename T3>
std::string BuildStr(const T0& v0, const T1& v1, const T2& v2, const T3& v3) {
    std::ostringstream oss;
    oss << v0 << v1 << v2 << v3;
    return oss.str();
}
template <typename T0, typename T1, typename T2, typename T3, typename T4>
std::string BuildStr(const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4) {
    std::ostringstream oss;
    oss << v0 << v1 << v2 << v3 << v4;
    return oss.str();
}
template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
std::string BuildStr(const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5) {
    std::ostringstream oss;
    oss << v0 << v1 << v2 << v3 << v4 << v5;
    return oss.str();
}

bool IsAutoFontToken(const std::string& value) {
    return value.empty() || value == "default" || value == "auto";
}

std::vector<std::string> BuildConfiguredFontPathCandidates(const std::string& configured_value) {
    std::vector<std::string> candidates;
    if (IsAutoFontToken(configured_value)) {
        return candidates;
    }
    candidates.push_back(configured_value);
    candidates.push_back(BuildStr("./", configured_value));
    candidates.push_back(BuildStr("assets/fonts/", configured_value));
    candidates.push_back(BuildStr("./assets/fonts/", configured_value));
#ifdef _WIN32
    const char* windir = std::getenv("WINDIR");
    const std::string font_dir = windir ? BuildStr(windir, "/Fonts/") : "C:/Windows/Fonts/";
    candidates.push_back(BuildStr(font_dir, configured_value));
    candidates.push_back(BuildStr("C:/Windows/Fonts/", configured_value));
#elif defined(__APPLE__)
    candidates.push_back(BuildStr("/System/Library/Fonts/", configured_value));
    candidates.push_back(BuildStr("/Library/Fonts/", configured_value));
    if (const char* home = std::getenv("HOME")) {
        candidates.push_back(BuildStr(home, "/Library/Fonts/", configured_value));
    }
#else
    candidates.push_back(BuildStr("/usr/share/fonts/", configured_value));
    candidates.push_back(BuildStr("/usr/share/fonts/truetype/", configured_value));
    candidates.push_back(BuildStr("/usr/share/fonts/opentype/", configured_value));
    if (const char* home = std::getenv("HOME")) {
        candidates.push_back(BuildStr(home, "/.local/share/fonts/", configured_value));
        candidates.push_back(BuildStr(home, "/.fonts/", configured_value));
    }
#endif
    return candidates;
}

}  // namespace

// ============================================================================
// 【LoadTexture】加载纹理
// ============================================================================
bool ResourceManager::LoadTexture(const std::string& id, const std::string& path) {
    if (textures_.contains(id)) {
        return true;
    }

    auto tex = std::make_unique<sf::Texture>();
    if (!tex->loadFromFile(path)) {
        Logger::Warning(BuildStr("ResourceManager: 无法加载纹理 '", id, "'，路径='", path, "'"));
        return false;
    }
    tex->setSmooth(false);

    textures_.emplace(id, std::move(tex));
    ref_counts_[id] = 1;
    Logger::Info(BuildStr("ResourceManager: 加载纹理 '", id, "'（路径='", path, "')"));
    return true;
}

// ============================================================================
// 【GetTexture】获取纹理引用
// ============================================================================
sf::Texture& ResourceManager::GetTexture(const std::string& id) {
    auto it = textures_.find(id);
    if (it == textures_.end()) {
        Logger::Warning(BuildStr("ResourceManager: 纹理未找到 '", id, "'，使用默认占位符"));
        if (!default_texture_) {
            default_texture_ = CreateDefaultTexture();
        }
        return *default_texture_;
    }
    return *it->second;
}

// ============================================================================
// 【HasTexture】检查纹理是否存在
// ============================================================================
bool ResourceManager::HasTexture(const std::string& id) const {
    return textures_.contains(id);
}

// ============================================================================
// 【UnloadTexture】卸载纹理
// ============================================================================
void ResourceManager::UnloadTexture(const std::string& id) {
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        textures_.erase(it);
        ref_counts_.erase(id);
        Logger::Info(BuildStr("ResourceManager: 卸载纹理 '", id, "'"));
    }
}

// ============================================================================
// 【LoadFont】加载字体
// ============================================================================
bool ResourceManager::LoadFont(const std::string& id, const std::string& path) {
    if (fonts_.contains(id)) {
        return true;
    }

    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(path)) {
        Logger::Warning(BuildStr("ResourceManager: 无法加载字体 '", id, "'，路径='", path, "'"));
        return false;
    }

    fonts_.emplace(id, std::move(font));
    ref_counts_[id] = 1;
    Logger::Info(BuildStr("ResourceManager: 加载字体 '", id, "'（路径='", path, "')"));
    return true;
}

// ============================================================================
// 【GetFont】获取字体引用
// ============================================================================
sf::Font& ResourceManager::GetFont(const std::string& id) {
    auto it = fonts_.find(id);
    if (it == fonts_.end()) {
        Logger::Warning(BuildStr("ResourceManager: 字体未找到 '", id, "'，使用默认占位符"));
        if (!default_font_) {
            default_font_ = CreateDefaultFont();
        }
        return *default_font_;
    }
    return *it->second;
}

// ============================================================================
// 【HasFont】检查字体是否存在
// ============================================================================
bool ResourceManager::HasFont(const std::string& id) const {
    return fonts_.contains(id);
}

bool ResourceManager::LoadSoundBuffer(const std::string& id, const std::string& path) {
    if (sound_buffers_.contains(id)) {
        return true;
    }

    auto sound = std::make_unique<sf::SoundBuffer>();
    if (!sound->loadFromFile(path)) {
        Logger::Warning(BuildStr("ResourceManager: 无法加载音效 '", id, "'，路径='", path, "'"));
        return false;
    }

    sound_buffers_.emplace(id, std::move(sound));
    ref_counts_[id] = 1;
    Logger::Info(BuildStr("ResourceManager: 加载音效 '", id, "'（路径='", path, "')"));
    return true;
}

sf::SoundBuffer& ResourceManager::GetSoundBuffer(const std::string& id) {
    auto it = sound_buffers_.find(id);
    if (it == sound_buffers_.end()) {
        Logger::Warning(BuildStr("ResourceManager: 音效未找到 '", id, "'，使用默认占位符"));
        if (!default_sound_buffer_) {
            default_sound_buffer_ = std::make_unique<sf::SoundBuffer>();
        }
        return *default_sound_buffer_;
    }
    return *it->second;
}

bool ResourceManager::HasSoundBuffer(const std::string& id) const {
    return sound_buffers_.contains(id);
}

void ResourceManager::UnloadSoundBuffer(const std::string& id) {
    auto it = sound_buffers_.find(id);
    if (it != sound_buffers_.end()) {
        sound_buffers_.erase(it);
        ref_counts_.erase(id);
        Logger::Info(BuildStr("ResourceManager: 卸载音效 '", id, "'"));
    }
}

// ============================================================================
// 【UnloadFont】卸载字体
// ============================================================================
void ResourceManager::UnloadFont(const std::string& id) {
    auto it = fonts_.find(id);
    if (it != fonts_.end()) {
        fonts_.erase(it);
        ref_counts_.erase(id);
        Logger::Info(BuildStr("ResourceManager: 卸载字体 '", id, "'"));
    }
}

// ============================================================================
// 【LoadBestAvailableFont】搜索并加载最佳可用字体
// ============================================================================
bool ResourceManager::LoadBestAvailableFont(const std::string& id) {
    if (fonts_.contains(id)) {
        return true;
    }

    const std::vector<std::pair<std::string, std::string>> candidates = BuildFontCandidates_();

    for (const auto& [path, desc] : candidates) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            continue;
        }
        if (LoadFont(id, path)) {
            Logger::Info(BuildStr("ResourceManager: 字体 '", id, "' 加载成功（", desc, "）"));
            return true;
        }
    }

    Logger::Error(BuildStr("ResourceManager: 字体 '", id, "' 未能加载，所有候选路径均失败"));
    return false;
}

// ============================================================================
// 【GetMainFont】获取主字体
// ============================================================================
sf::Font& ResourceManager::GetMainFont() {
    if (!best_main_font_id_.empty() && HasFont(best_main_font_id_)) {
        return GetFont(best_main_font_id_);
    }

    constexpr const char* kMainFontId = "best_main_font";
    if (LoadBestAvailableFont(kMainFontId)) {
        best_main_font_id_ = kMainFontId;
        return GetFont(kMainFontId);
    }

    Logger::Warning("ResourceManager: 主字体加载失败，返回空引用");
    static sf::Font dummy_font;
    return dummy_font;
}

// ============================================================================
// 【PreloadBundle】批量预加载
// ============================================================================
void ResourceManager::PreloadBundle(const std::string& bundle_name) {
    (void)bundle_name;
    UiLayoutConfig ui_layout;
    if (ui_layout.LoadFromFile("configs/ui_layout.json")) {
        const auto& ui_data = ui_layout.Data();
        bool loaded_from_ui = false;

        const auto primary_candidates = BuildConfiguredFontPathCandidates(ui_data.primary_font);
        for (const auto& candidate : primary_candidates) {
            if (LoadFont("best_main_font", candidate)) {
                best_main_font_id_ = "best_main_font";
                Logger::Info(BuildStr("ResourceManager: 已从 ui_layout.primary 加载主字体（", candidate, "）"));
                loaded_from_ui = true;
                break;
            }
        }

        if (!loaded_from_ui) {
            const auto fallback_candidates = BuildConfiguredFontPathCandidates(ui_data.fallback_font);
            for (const auto& candidate : fallback_candidates) {
                if (LoadFont("best_main_font", candidate)) {
                    best_main_font_id_ = "best_main_font";
                    Logger::Info(BuildStr("ResourceManager: 已从 ui_layout.fallback 加载主字体（", candidate, "）"));
                    loaded_from_ui = true;
                    break;
                }
            }
        }

        if (!loaded_from_ui) {
            Logger::Warning("ResourceManager: ui_layout 配置字体加载失败，回退到自动字体搜索链");
        }
    } else {
        Logger::Warning("ResourceManager: 未找到 ui_layout 配置，回退到自动字体搜索链");
    }

    LoadBestAvailableFont("best_main_font");
}

// ============================================================================
// 【ReleaseUnused】释放未使用资源
// ============================================================================
void ResourceManager::ReleaseUnused() {
    std::vector<std::string> to_remove;
    for (const auto& [id, count] : ref_counts_) {
        if (count <= 0) {
            to_remove.push_back(id);
        }
    }
    for (const auto& id : to_remove) {
        UnloadTexture(id);
        UnloadFont(id);
        UnloadSoundBuffer(id);
    }
    Logger::Info(BuildStr("ResourceManager: ReleaseUnused 完成，释放了 ", to_remove.size(), " 个未使用资源"));
}

// ============================================================================
// 【ReleaseAll】释放所有资源
// ============================================================================
void ResourceManager::ReleaseAll() {
    textures_.clear();
    fonts_.clear();
    sound_buffers_.clear();
    ref_counts_.clear();
    best_main_font_id_.clear();
    Logger::Info("ResourceManager: 所有资源已释放");
}

// ============================================================================
// 【PrintStats】输出统计信息
// ============================================================================
void ResourceManager::PrintStats() const {
    const auto stats = GetStats();
    std::ostringstream oss;
    oss << "ResourceManager 统计: 纹理=" << stats.texture_count << " 个, 字体=" << stats.font_count << " 个, "
        << "音效=" << stats.sound_buffer_count << " 个, "
        << "纹理内存估算=" << (stats.total_texture_memory_bytes / 1024) << " KB, 字体内存估算="
        << (stats.total_font_memory_bytes / 1024) << " KB, 音效内存估算="
        << (stats.total_sound_memory_bytes / 1024) << " KB";
    Logger::Info(oss.str());
}

// ============================================================================
// 【GetStats】获取统计信息
// ============================================================================
ResourceStats ResourceManager::GetStats() const {
    ResourceStats stats;
    stats.texture_count = textures_.size();
    stats.font_count = fonts_.size();
    stats.sound_buffer_count = sound_buffers_.size();
    stats.total_texture_memory_bytes = stats.texture_count * 256 * 1024;
    stats.total_font_memory_bytes = stats.font_count * 2 * 1024 * 1024;
    stats.total_sound_memory_bytes = stats.sound_buffer_count * 512 * 1024;
    return stats;
}

// ============================================================================
// 【GetSystemFontSearchPaths_】系统字体搜索路径（保留备用）
// ============================================================================
std::vector<std::pair<std::string, std::string>> ResourceManager::GetSystemFontSearchPaths_() {
    std::vector<std::pair<std::string, std::string>> paths;
#ifdef _WIN32
    const char* windir = std::getenv("WINDIR");
    std::string font_dir = windir ? std::string(windir) + "/Fonts/" : "C:/Windows/Fonts/";
    paths.push_back({font_dir + "SourceHanSansCN-Normal.ttf", "思源黑体"});
    paths.push_back({font_dir + "simhei.ttf", "黑体"});
    paths.push_back({font_dir + "msyh.ttc", "雅黑"});
    paths.push_back({font_dir + "simsun.ttc", "宋体"});
    paths.push_back({font_dir + "arial.ttf", "Arial"});
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    paths.push_back({"/System/Library/Fonts/PingFang.ttc", "苹方"});
    paths.push_back({"/System/Library/Fonts/STHeiti Light.ttc", "华文黑体"});
    paths.push_back({"/Library/Fonts/Arial Unicode.ttf", "Arial Unicode"});
    paths.push_back({"/System/Library/Fonts/Helvetica.ttc", "Helvetica"});
    if (home) {
        paths.push_back({std::string(home) + "/Library/Fonts/Arial Unicode.ttf", "Arial Unicode（用户）"});
        paths.push_back({std::string(home) + "/Library/Fonts/PingFang.ttc", "苹方（用户）"});
    }
#else
    const char* home = std::getenv("HOME");
    paths.push_back({"/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc", "Noto Sans CJK"});
    paths.push_back({"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "Noto Sans CJK"});
    paths.push_back({"/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc", "文泉驿正黑"});
    paths.push_back({"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "DejaVu Sans"});
    if (home) {
        paths.push_back({std::string(home) + "/.local/share/fonts/NotoSansCJK-Regular.ttc", "Noto Sans CJK（用户）"});
        paths.push_back({std::string(home) + "/.fonts/NotoSansCJK-Regular.ttc", "Noto Sans CJK（用户）"});
    }
#endif
    return paths;
}

std::vector<std::pair<std::string, std::string>> ResourceManager::GetProjectFontSearchPaths_() {
    return {
        {"assets/fonts/simhei.ttf", "黑体（项目）"},
        {"assets/fonts/msyh.ttc", "雅黑（项目）"},
        {"assets/fonts/arial.ttf", "Arial（项目）"},
        {"./assets/fonts/simhei.ttf", "黑体（项目./）"},
        {"./assets/fonts/msyh.ttc", "雅黑（项目./）"},
        {"./assets/fonts/arial.ttf", "Arial（项目./）"},
    };
}

std::vector<std::pair<std::string, std::string>> ResourceManager::BuildFontCandidates_() {
    auto system_fonts = GetSystemFontSearchPaths_();
    auto project_fonts = GetProjectFontSearchPaths_();
    std::vector<std::pair<std::string, std::string>> candidates;
    candidates.reserve(system_fonts.size() + project_fonts.size());
    candidates.insert(candidates.end(), project_fonts.begin(), project_fonts.end());
    candidates.insert(candidates.end(), system_fonts.begin(), system_fonts.end());
    return candidates;
}

}  // namespace CloudSeamanor::infrastructure
