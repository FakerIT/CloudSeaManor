#include "CloudSeamanor/engine/AudioManager.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/infrastructure/ResourceManager.hpp"

#include <algorithm>
#include <fstream>
#include <string>

namespace CloudSeamanor::engine::audio {
namespace {

bool TryParseVolumeValue(const std::string& raw_value,
                         const std::string& key_name,
                         const std::string& config_path,
                         float* out_value) {
    if (out_value == nullptr) {
        return false;
    }

    std::size_t parsed_chars = 0;
    try {
        const float parsed = std::stof(raw_value, &parsed_chars);
        if (parsed_chars == 0) {
            CloudSeamanor::infrastructure::Logger::Warning(
                "AudioManager: 配置值为空，字段=" + key_name + "，文件=" + config_path);
            return false;
        }
        *out_value = std::clamp(parsed, 0.0f, 1.0f);
        return true;
    } catch (const std::invalid_argument&) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "AudioManager: 配置解析失败（非法值），字段=" + key_name +
            "，原始值=" + raw_value + "，文件=" + config_path);
        return false;
    } catch (const std::out_of_range&) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "AudioManager: 配置解析失败（超范围），字段=" + key_name +
            "，原始值=" + raw_value + "，文件=" + config_path);
        return false;
    }
}

}  // namespace

AudioManager::AudioManager() = default;
AudioManager::~AudioManager() { Shutdown(); }
void AudioManager::Initialize() {
    if (initialized_) return;
    bgm_ = std::make_unique<sf::Music>();
    initialized_ = true;
}
bool AudioManager::LoadConfig(const std::string& config_path) {
    const auto result = LoadConfigResult(config_path);
    if (!result.Ok()) {
        CloudSeamanor::infrastructure::Logger::Warning("AudioManager: " + result.Error());
        return false;
    }
    return true;
}

CloudSeamanor::Result<void> AudioManager::LoadConfigResult(const std::string& config_path) {
    std::ifstream in(config_path);
    if (!in.is_open()) {
        return "无法打开音频配置文件: " + config_path;
    }
    std::string line;
    int parsed_count = 0;
    int failed_count = 0;
    int fallback_count = 0;
    bool seen_music = false;
    bool seen_bgm = false;
    bool seen_sfx = false;

    while (std::getline(in, line)) {
        const auto p = line.find(':');
        if (p == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, p);
        key.erase(std::remove_if(key.begin(), key.end(), [](unsigned char c) {
            return c == '"' || c == ',' || c == ' ' || c == '\t' || c == '{' || c == '}';
        }), key.end());

        std::string value = line.substr(p + 1);
        value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
            return c == '"' || c == ',' || c == ' ' || c == '\t' || c == '{' || c == '}';
        }), value.end());

        if (key == "music_volume") {
            seen_music = true;
            if (TryParseVolumeValue(value, key, config_path, &music_volume_)) {
                ++parsed_count;
            } else {
                ++failed_count;
                ++fallback_count;
            }
        } else if (key == "bgm_volume") {
            seen_bgm = true;
            if (TryParseVolumeValue(value, key, config_path, &bgm_volume_)) {
                ++parsed_count;
            } else {
                ++failed_count;
                ++fallback_count;
            }
        } else if (key == "sfx_volume") {
            seen_sfx = true;
            if (TryParseVolumeValue(value, key, config_path, &sfx_volume_)) {
                ++parsed_count;
            } else {
                ++failed_count;
                ++fallback_count;
            }
        }
    }

    if (!seen_music) ++fallback_count;
    if (!seen_bgm) ++fallback_count;
    if (!seen_sfx) ++fallback_count;

    CloudSeamanor::infrastructure::Logger::Info(
        "AudioManager: 配置加载完成 path=" + config_path +
        "，解析成功=" + std::to_string(parsed_count) +
        "，解析失败=" + std::to_string(failed_count) +
        "，默认回退=" + std::to_string(fallback_count));
    return {};
}
void AudioManager::Shutdown() {
    if (bgm_) {
        bgm_->stop();
        bgm_.reset();
    }
    active_sounds_.clear();
    sfx_buffers_.clear();
    resource_manager_ = nullptr;
    initialized_ = false;
}
void AudioManager::SetResourceManager(infrastructure::ResourceManager* rm) {
    resource_manager_ = rm;
}
void AudioManager::PlayBGM(const std::string& id, bool loop, float fade_in) {
    if (!initialized_) return;
    StopBGM();
    if (id.empty()) return;
    if (!bgm_->openFromFile(id)) {
        CloudSeamanor::infrastructure::Logger::Warning("AudioManager: BGM 加载失败 id/path=" + id);
        return;
    }
    bgm_->setLooping(loop);
    bgm_->setVolume(0.0f);
    bgm_->play();
    current_bgm_id_ = id;
    if (fade_in > 0.0f) {
        bgm_fade_timer_ = 0.0f;
        bgm_fade_duration_ = fade_in;
        bgm_target_volume_ = bgm_volume_ * music_volume_ * 100.0f;
        bgm_fading_in_ = true;
        bgm_fading_out_ = false;
    } else {
        bgm_->setVolume(bgm_volume_ * music_volume_ * 100.0f);
        bgm_fading_in_ = false;
    }
}
void AudioManager::StopBGM(float fade_out) {
    if (!bgm_) return;
    if (fade_out > 0.0f) {
        bgm_fade_timer_ = 0.0f;
        bgm_fade_duration_ = fade_out;
        bgm_fading_out_ = true;
        bgm_fading_in_ = false;
    } else {
        bgm_->stop();
        current_bgm_id_.clear();
    }
}
void AudioManager::PauseBGM() { if(bgm_) bgm_->pause(); }
void AudioManager::ResumeBGM() { if(bgm_) bgm_->play(); }
void AudioManager::SetBGMVolume(float v) { bgm_volume_=std::clamp(v,0.0f,1.0f); if(bgm_&&!bgm_fading_in_&&!bgm_fading_out_) bgm_->setVolume(bgm_volume_*music_volume_*100.0f); }
void AudioManager::SetMusicVolume(float v) { music_volume_ = std::clamp(v, 0.0f, 1.0f); }
bool AudioManager::IsBGMPlaying() const { return bgm_ && bgm_->getStatus() == sf::SoundSource::Status::Playing; }
bool AudioManager::PlaySFX(const std::string& id, float vol) {
    if(!initialized_) return false;

    // 优先从 ResourceManager 获取音效缓冲（统一资源路径）
    if (resource_manager_ != nullptr) {
        try {
            sf::SoundBuffer& buf = resource_manager_->GetSoundBuffer(id);
            active_sounds_.erase(std::remove_if(active_sounds_.begin(), active_sounds_.end(), [](const std::unique_ptr<sf::Sound>& s) {
                return s->getStatus() != sf::SoundSource::Status::Playing;
            }), active_sounds_.end());
            if(active_sounds_.size()>=MAX_ACTIVE_SOUNDS) {
                CloudSeamanor::infrastructure::Logger::Warning(
                    "AudioManager: 活跃 SFX 达到上限，已忽略 id=" + id);
                return false;
            }
            auto s = std::make_unique<sf::Sound>(buf);
            s->setVolume(std::clamp(vol, 0.0f, 1.0f) * sfx_volume_ * 100.0f);
            s->play();
            active_sounds_.push_back(std::move(s));
            return true;
        } catch (...) {
            // ResourceManager 中没有，继续尝试本地缓存
        }
    }

    // 回退：本地缓存
    auto it=sfx_buffers_.find(id); if(it==sfx_buffers_.end()) {
        CloudSeamanor::infrastructure::Logger::Warning("AudioManager: SFX 未找到 id=" + id);
        return false;
    }
    active_sounds_.erase(std::remove_if(active_sounds_.begin(), active_sounds_.end(), [](const std::unique_ptr<sf::Sound>& s) {
        return s->getStatus() != sf::SoundSource::Status::Playing;
    }), active_sounds_.end());
    if(active_sounds_.size()>=MAX_ACTIVE_SOUNDS) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "AudioManager: 活跃 SFX 达到上限，已忽略 id=" + id);
        return false;
    }
    auto s = std::make_unique<sf::Sound>(*it->second);
    s->setVolume(std::clamp(vol, 0.0f, 1.0f) * sfx_volume_ * 100.0f);
    s->play();
    active_sounds_.push_back(std::move(s));
    return true;
}
void AudioManager::SetSFXVolume(float v) { sfx_volume_=std::clamp(v,0.0f,1.0f); }
void AudioManager::Update(float dt) {
    if(!bgm_) return;
    if(bgm_fading_in_){ bgm_fade_timer_+=dt; float t=std::clamp(bgm_fade_timer_/bgm_fade_duration_,0.0f,1.0f); bgm_->setVolume(t*bgm_target_volume_); if(t>=1.0f) bgm_fading_in_=false; }
    else if(bgm_fading_out_){ bgm_fade_timer_+=dt; float t=1.0f-std::clamp(bgm_fade_timer_/bgm_fade_duration_,0.0f,1.0f); bgm_->setVolume(t*bgm_volume_*music_volume_*100.0f); if(t<=0.0f){bgm_->stop();current_bgm_id_.clear();bgm_fading_out_=false;} }
}
bool AudioManager::PreloadSFX(const std::string& id, const std::string& path){
    // 优先使用 ResourceManager 统一加载
    if (resource_manager_ != nullptr) {
        if (resource_manager_->LoadSoundBuffer(id, path)) {
            CloudSeamanor::infrastructure::Logger::Info(
                "AudioManager: SFX 通过 ResourceManager 预加载成功 id=" + id);
            return true;
        }
        // ResourceManager 加载失败，继续尝试本地加载
        CloudSeamanor::infrastructure::Logger::Warning(
            "AudioManager: ResourceManager 加载失败，尝试本地加载 id=" + id);
    }

    // 本地加载作为回退
    auto buf=std::make_unique<sf::SoundBuffer>();
    if(!buf->loadFromFile(path)) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "AudioManager: SFX 预加载失败 id=" + id + ", path=" + path);
        return false;
    }
    sfx_buffers_[id]=std::move(buf);
    CloudSeamanor::infrastructure::Logger::Info(
        "AudioManager: SFX 本地预加载成功 id=" + id + ", path=" + path);
    return true;
}
void AudioManager::UnloadSFX(const std::string& id){ sfx_buffers_.erase(id); }
void AudioManager::UnloadUnusedSFX(){
    for (auto it = sfx_buffers_.begin(); it != sfx_buffers_.end();) {
        const sf::SoundBuffer* buffer_ptr = it->second.get();
        bool in_use = false;
        for (const auto& sound : active_sounds_) {
            if (sound && sound->getStatus() == sf::SoundSource::Status::Playing && &sound->getBuffer() == buffer_ptr) {
                in_use = true;
                break;
            }
        }
        it = in_use ? std::next(it) : sfx_buffers_.erase(it);
    }
}
}  // namespace CloudSeamanor::engine::audio