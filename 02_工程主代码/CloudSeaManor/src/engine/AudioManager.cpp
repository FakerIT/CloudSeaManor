#include "CloudSeamanor/AudioManager.hpp"

#include <algorithm>
#include <fstream>

namespace CloudSeamanor::engine::audio {
AudioManager::AudioManager() = default;
AudioManager::~AudioManager() { Shutdown(); }
void AudioManager::Initialize() {
    if (initialized_) return;
    bgm_ = std::make_unique<sf::Music>();
    initialized_ = true;
}
bool AudioManager::LoadConfig(const std::string& config_path) {
    std::ifstream in(config_path);
    if (!in.is_open()) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("music_volume") != std::string::npos) {
            const auto p = line.find(':');
            if (p != std::string::npos) music_volume_ = std::clamp(std::stof(line.substr(p + 1)), 0.0f, 1.0f);
        } else if (line.find("bgm_volume") != std::string::npos) {
            const auto p = line.find(':');
            if (p != std::string::npos) bgm_volume_ = std::clamp(std::stof(line.substr(p + 1)), 0.0f, 1.0f);
        } else if (line.find("sfx_volume") != std::string::npos) {
            const auto p = line.find(':');
            if (p != std::string::npos) sfx_volume_ = std::clamp(std::stof(line.substr(p + 1)), 0.0f, 1.0f);
        }
    }
    return true;
}
void AudioManager::Shutdown() {
    if (bgm_) {
        bgm_->stop();
        bgm_.reset();
    }
    active_sounds_.clear();
    sfx_buffers_.clear();
    initialized_ = false;
}
void AudioManager::PlayBGM(const std::string& id, bool loop, float fade_in) {
    if (!initialized_) return;
    StopBGM();
    if (id.empty()) return;
    if (!bgm_->openFromFile(id)) return;
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
    auto it=sfx_buffers_.find(id); if(it==sfx_buffers_.end()) return false;
    active_sounds_.erase(std::remove_if(active_sounds_.begin(), active_sounds_.end(), [](const std::unique_ptr<sf::Sound>& s) {
        return s->getStatus() != sf::SoundSource::Status::Playing;
    }), active_sounds_.end());
    if(active_sounds_.size()>=MAX_ACTIVE_SOUNDS) return false;
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
bool AudioManager::PreloadSFX(const std::string& id, const std::string& path){ auto buf=std::make_unique<sf::SoundBuffer>(); if(!buf->loadFromFile(path)) return false; sfx_buffers_[id]=std::move(buf); return true; }
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