#include "CloudSeamanor/infrastructure/NvidiaImageGenerator.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace CloudSeamanor::examples {

struct CharacterPortraitRequest {
    std::string character_name;
    std::string hairstyle;
    std::string outfit;
    std::string expression;
    std::string pose;
    std::string art_style;
    std::string description_zh;
};

struct GameSceneRequest {
    std::string scene_name;
    std::string location;
    std::string time_of_day;
    std::string weather;
    std::string atmosphere;
    std::string description_zh;
};

class NvidiaImageIntegrationExamples {
public:
    static void Example_CharacterPortraitGeneration() {
        std::cout << "=== NVIDIA Image Generation Example: Character Portrait ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        NvidiaImageGenerator::Initialize(api_key);

        CharacterPortraitRequest char_req;
        char_req.character_name = "阿茶";
        char_req.hairstyle = "黑色长发，饰以银色发簪";
        char_req.outfit = "淡青色仙侠风格长裙，绣有云纹";
        char_req.expression = "温柔微笑，眼神温柔";
        char_req.pose = "手持茶杯，站在茶园中";
        char_req.art_style = "精美二次元游戏立绘风格，柔和光影";
        char_req.description_zh = "云海山庄角色立绘";

        std::string prompt = BuildCharacterPortraitPrompt(char_req);

        std::cout << "Generated Prompt: " << prompt << std::endl;

        NvidiaImageRequest request;
        request.prompt = prompt;
        request.negative_prompt = "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers";
        request.width = 1024;
        request.height = 1024;
        request.steps = 50;
        request.cfg_scale = 7.5f;
        request.seed = 12345;
        request.aspect_ratio = "1:1";

        std::cout << "Generating character portrait..." << std::endl;
        auto response = NvidiaImageGenerator::GenerateImageSync(request);

        if (response && response->success) {
            std::string output_path = "generated_assets/portraits/" + char_req.character_name + ".png";
            SaveImageToFile(response->image_data, output_path);
            std::cout << "Character portrait saved to: " << output_path << std::endl;
        } else {
            std::string error = response ? response->error_message : NvidiaImageGenerator::GetLastError();
            std::cerr << "Failed to generate portrait: " << error << std::endl;
        }

        NvidiaImageGenerator::Shutdown();
    }

    static std::string BuildCharacterPortraitPrompt(const CharacterPortraitRequest& req) {
        return req.art_style + ", " +
               "character portrait of " + req.character_name + ", " +
               req.hairstyle + ", " +
               req.outfit + ", " +
               req.expression + ", " +
               req.pose + ", " +
               "detailed face, beautiful eyes, high quality illustration";
    }

    static void Example_GameSceneGeneration() {
        std::cout << "=== NVIDIA Image Generation Example: Game Scene ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        NvidiaImageGenerator::Initialize(api_key);

        GameSceneRequest scene_req;
        scene_req.scene_name = "云海茶园";
        scene_req.location = "中国古风仙侠世界中的茶园";
        scene_req.time_of_day = "清晨时分，薄雾缭绕";
        scene_req.weather = "晴朗，有淡淡的晨雾";
        scene_req.atmosphere = "宁静祥和，灵气充沛";
        scene_req.description_zh = "云海山庄游戏场景";

        std::string prompt = BuildGameScenePrompt(scene_req);

        std::cout << "Generated Prompt: " << prompt << std::endl;

        NvidiaImageRequest request;
        request.prompt = prompt;
        request.negative_prompt = "low quality, blurry, watermark, text, logo, signature, deformed buildings";
        request.width = 1920;
        request.height = 1080;
        request.steps = 50;
        request.cfg_scale = 7.0f;
        request.seed = 54321;
        request.aspect_ratio = "16:9";

        std::cout << "Generating game scene..." << std::endl;
        auto response = NvidiaImageGenerator::GenerateImageSync(request);

        if (response && response->success) {
            std::string output_path = "generated_assets/scenes/" + scene_req.scene_name + ".png";
            SaveImageToFile(response->image_data, output_path);
            std::cout << "Game scene saved to: " << output_path << std::endl;
        } else {
            std::string error = response ? response->error_message : NvidiaImageGenerator::GetLastError();
            std::cerr << "Failed to generate scene: " << error << std::endl;
        }

        NvidiaImageGenerator::Shutdown();
    }

    static std::string BuildGameScenePrompt(const GameSceneRequest& req) {
        return "vertical scrolling game background scene, " +
               std::string("beautiful Chinese xianxia fantasy style landscape, ") +
               req.location + ", " +
               req.time_of_day + ", " +
               req.weather + ", " +
               req.atmosphere + ", " +
               "lush green tea plantation, traditional chinese architecture, " +
               "mountain peaks in background, clouds and mist, peaceful atmosphere, " +
               "high quality game art, detailed environment illustration, soft lighting";
    }

    static void Example_AsyncCharacterPortraitGeneration() {
        std::cout << "=== NVIDIA Image Generation Example: Async Character Portrait ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        NvidiaImageGenerator::Initialize(api_key);

        std::vector<std::string> characters = {"小满", "晚星", "素锦", "烟萝"};

        for (const auto& character : characters) {
            std::cout << "Generating portrait for: " << character << std::endl;

            NvidiaImageRequest request;
            request.prompt = "beautiful anime game character portrait, " + character + ", " +
                           "elegant chinese xianxia style, detailed hair with ornaments, " +
                           "colorful traditional hanfu, gentle expression, upper body shot, " +
                           "high quality illustration, soft lighting";
            request.width = 1024;
            request.height = 1024;
            request.steps = 50;
            request.cfg_scale = 7.5f;
            request.seed = 0;
            request.aspect_ratio = "1:1";

            NvidiaImageGenerator::GenerateImageAsync(request,
                [character](NvidiaImageResponse resp) {
                    if (resp.success) {
                        std::string output_path = "generated_assets/portraits/" + character + ".png";
                        SaveImageToFile(resp.image_data, output_path);
                        std::cout << "Portrait generated for " << character << std::endl;
                    } else {
                        std::cerr << "Failed to generate portrait for " << character
                                  << ": " << resp.error_message << std::endl;
                    }
                });
        }

        std::cout << "All portrait generation requests submitted!" << std::endl;
        std::cout << "Waiting for async responses..." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(30));

        NvidiaImageGenerator::Shutdown();
    }

    static void SaveImageToFile(const std::vector<std::uint8_t>& image_data,
                                const std::string& file_path) {
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        std::ofstream out_file(file_path, std::ios::binary);
        if (out_file.is_open()) {
            out_file.write(reinterpret_cast<const char*>(image_data.data()),
                          static_cast<std::streamsize>(image_data.size()));
            out_file.close();
            std::cout << "Image saved successfully: " << file_path
                      << " (Size: " << image_data.size() << " bytes)" << std::endl;
        } else {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
        }
    }

    static void Example_ImageGenerationWithSFML() {
        std::cout << "=== NVIDIA Image Generation Example: SFML Integration ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        NvidiaImageGenerator::Initialize(api_key);

        NvidiaImageRequest request;
        request.prompt = "beautiful anime character portrait, young girl with long black hair, " +
                       "wearing elegant chinese hanfu with floral patterns, gentle smile, " +
                       "holding a tea cup, warm lighting, high quality illustration";
        request.width = 512;
        request.height = 512;
        request.steps = 30;
        request.cfg_scale = 7.0f;
        request.seed = 42;
        request.aspect_ratio = "1:1";

        std::cout << "Generating image for SFML display..." << std::endl;
        auto response = NvidiaImageGenerator::GenerateImageSync(request);

        if (response && response->success) {
            std::string temp_path = "generated_assets/temp_character.png";
            SaveImageToFile(response->image_data, temp_path);

            sf::Texture texture;
            if (texture.loadFromFile(temp_path)) {
                sf::Sprite sprite(texture);

                sf::RenderWindow window(sf::VideoMode(512, 512), "Generated Character Portrait");
                while (window.isOpen()) {
                    sf::Event event;
                    while (window.pollEvent(event)) {
                        if (event.type == sf::Event::Closed) {
                            window.close();
                        }
                    }
                    window.clear();
                    window.draw(sprite);
                    window.display();
                }
            } else {
                std::cerr << "Failed to load texture from: " << temp_path << std::endl;
            }

            std::filesystem::remove(temp_path);
        } else {
            std::string error = response ? response->error_message : NvidiaImageGenerator::GetLastError();
            std::cerr << "Failed to generate image: " << error << std::endl;
        }

        NvidiaImageGenerator::Shutdown();
    }

    static void Example_GameSceneBatchGeneration() {
        std::cout << "=== NVIDIA Image Generation Example: Batch Scene Generation ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        NvidiaImageGenerator::Initialize(api_key);
        NvidiaImageGenerator::SetTimeout(180);

        std::vector<GameSceneRequest> scenes = {
            {"主屋", "云海山庄主建筑", "黄昏时分，金色阳光", "晴朗，微风", "温馨氛围，炊烟袅袅"},
            {"茶园", "云海茶园梯田", "清晨，薄雾未散", "晴朗", "宁静祥和，露珠闪烁"},
            {"温泉区", "仙气缭绕的温泉", "夜晚，星光点点", "薄雾", "神秘浪漫，灵气氤氲"},
            {"观云台", "山顶观景平台", "黎明破晓时分", "云海翻涌", "壮观辽阔，心旷神怡"},
            {"灵兽园", "仙兽栖息之地", "正午，阳光明媚", "晴朗", "生机勃勃，灵兽嬉戏"}
        };

        for (const auto& scene : scenes) {
            std::string prompt = BuildGameScenePrompt(scene);

            NvidiaImageRequest request;
            request.prompt = prompt;
            request.width = 1920;
            request.height = 1080;
            request.steps = 50;
            request.cfg_scale = 7.0f;
            request.seed = 0;
            request.aspect_ratio = "16:9";

            std::cout << "Generating scene: " << scene.scene_name << std::endl;
            auto response = NvidiaImageGenerator::GenerateImageSync(request);

            if (response && response->success) {
                std::string output_path = "generated_assets/scenes/" + scene.scene_name + ".png";
                SaveImageToFile(response->image_data, output_path);
                std::cout << "Scene generated: " << scene.scene_name << std::endl;
            } else {
                std::string error = response ? response->error_message : NvidiaImageGenerator::GetLastError();
                std::cerr << "Failed to generate scene " << scene.scene_name << ": " << error << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        NvidiaImageGenerator::Shutdown();
    }
};

} // namespace CloudSeamanor::examples

int main() {
    std::cout << "Starting NVIDIA Image Generation Examples..." << std::endl;

    CloudSeamanor::examples::NvidiaImageIntegrationExamples::Example_CharacterPortraitGeneration();

    CloudSeamanor::examples::NvidiaImageIntegrationExamples::Example_GameSceneGeneration();

    CloudSeamanor::examples::NvidiaImageIntegrationExamples::Example_GameSceneBatchGeneration();

    std::cout << "All examples completed!" << std::endl;
    return 0;
}