#include "../TestFramework.hpp"
#include "CloudSeamanor/AudioManager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using CloudSeamanor::engine::audio::AudioManager;

namespace {

std::filesystem::path MakeTempAudioConfigPath() {
    const auto base = std::filesystem::temp_directory_path();
    const auto unique = "cloudseamanor_audio_config_test_" + std::to_string(std::rand()) + ".json";
    return base / unique;
}

}  // namespace

TEST_CASE(AudioManager_load_config_fallback_on_invalid_values) {
    const auto temp_path = MakeTempAudioConfigPath();
    {
        std::ofstream out(temp_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "{\n";
        out << "  \"music_volume\": \"bad\",\n";
        out << "  \"bgm_volume\": 0.5,\n";
        out << "  \"sfx_volume\": 99\n";
        out << "}\n";
    }

    AudioManager manager;
    ASSERT_TRUE(manager.LoadConfig(temp_path.string()));
    ASSERT_FLOAT_EQ(manager.MusicVolume(), 0.7f, 0.0001f);
    ASSERT_FLOAT_EQ(manager.BGMVolume(), 0.5f, 0.0001f);
    ASSERT_FLOAT_EQ(manager.SFXVolume(), 1.0f, 0.0001f);

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
    return true;
}

