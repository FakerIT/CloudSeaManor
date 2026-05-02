#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace CloudSeamanor::infrastructure {

struct NvidiaImageRequest {
    std::string prompt;
    std::string negative_prompt;
    int width = 1024;
    int height = 1024;
    int steps = 50;
    float cfg_scale = 5.0f;
    int seed = 0;
    std::string aspect_ratio = "1:1";
};

struct NvidiaImageResponse {
    bool success = false;
    std::string error_message;
    std::vector<std::uint8_t> image_data;
    std::string image_format;
};

class NvidiaImageGenerator {
public:
    static void Initialize(const std::string& api_key, const std::string& model = "stable-diffusion-3-medium");
    static void Shutdown();
    static bool IsInitialized();

    static void GenerateImageAsync(
        const NvidiaImageRequest& request,
        std::function<void(NvidiaImageResponse)> callback
    );

    static std::optional<NvidiaImageResponse> GenerateImageSync(const NvidiaImageRequest& request);

    static std::string GetLastError();
    static void SetTimeout(int timeout_seconds);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // namespace CloudSeamanor::infrastructure