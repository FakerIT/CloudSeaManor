#pragma once
// 【AudioManager】BGM/SFX 音频管理器
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace CloudSeamanor::engine::audio {

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    void Initialize();
    void Shutdown();
    bool LoadConfig(const std::string& config_path);
    void PlayBGM(const std::string& id, bool loop=true, float fade_in_seconds=1.0f);
    void StopBGM(float fade_out_seconds=1.0f);
    void PauseBGM();
    void ResumeBGM();
    void SetBGMVolume(float volume);
    void SetMusicVolume(float volume);
    [[nodiscard]] float BGMVolume() const { return bgm_volume_; }
    [[nodiscard]] float MusicVolume() const { return music_volume_; }
    [[nodiscard]] bool IsBGMPlaying() const;
    bool PlaySFX(const std::string& id, float volume=1.0f);
    void SetSFXVolume(float volume);
    [[nodiscard]] float SFXVolume() const { return sfx_volume_; }
    void Update(float delta_seconds);
    bool PreloadSFX(const std::string& id, const std::string& path);
    void UnloadSFX(const std::string& id);
    void UnloadUnusedSFX();
    [[nodiscard]] std::size_t SFXCacheCount() const { return sfx_buffers_.size(); }
    [[nodiscard]] std::string CurrentBGMId() const { return current_bgm_id_; }
private:
    std::unique_ptr<sf::Music> bgm_;
    std::string current_bgm_id_;
    float bgm_volume_ = 0.7f;
    float music_volume_ = 0.7f;
    float bgm_fade_timer_ = 0.0f;
    float bgm_fade_duration_ = 0.0f;
    float bgm_target_volume_ = 0.7f;
    bool bgm_fading_in_ = false;
    bool bgm_fading_out_ = false;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sfx_buffers_;
    std::vector<std::unique_ptr<sf::Sound>> active_sounds_;
    float sfx_volume_ = 1.0f;
    static constexpr std::size_t MAX_ACTIVE_SOUNDS = 16;
    bool initialized_ = false;
};

}  // namespace CloudSeamanor::engine::audio
