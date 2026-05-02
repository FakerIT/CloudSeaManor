#pragma once

#include "CloudSeamanor/infrastructure/NvidiaImageGenerator.hpp"
#include "CloudSeamanor/prompt_templates/PromptTemplateLibrary.hpp"
#include <SFML/Graphics/Texture.hpp>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

namespace CloudSeamanor::engine {

enum class ImageGenerationStatus {
    Idle,
    Generating,
    Success,
    Failed
};

struct GeneratedImage {
    std::string id;
    std::string prompt;
    std::string category;
    std::vector<std::uint8_t> image_data;
    std::shared_ptr<sf::Texture> texture;
    ImageGenerationStatus status = ImageGenerationStatus::Idle;
    std::string error_message;
    std::chrono::system_clock::time_point generated_at;
};

struct ImageGenerationRequest {
    std::string id;
    std::string prompt;
    std::string negative_prompt;
    std::string category;
    int width = 1024;
    int height = 1024;
    int steps = 50;
    float cfg_scale = 7.0f;
    int seed = 0;
    std::string aspect_ratio = "1:1";
    std::function<void(const GeneratedImage&)> callback;
};

class ImageGenerationManager {
public:
    static ImageGenerationManager& Instance();

    void Initialize(const std::string& api_key);
    void Shutdown();

    bool IsInitialized() const { return initialized_.load(); }

    std::string SubmitRequest(const ImageGenerationRequest& request);
    std::string SubmitCharacterPortraitRequest(
        const std::string& character_name,
        const std::string& template_id,
        std::function<void(const GeneratedImage&)> callback = nullptr
    );
    std::string SubmitGameSceneRequest(
        const std::string& scene_name,
        const std::string& template_id,
        std::function<void(const GeneratedImage&)> callback = nullptr
    );

    std::optional<GeneratedImage> GetGeneratedImage(const std::string& id) const;
    std::shared_ptr<sf::Texture> GetTexture(const std::string& id);
    ImageGenerationStatus GetStatus(const std::string& id) const;

    std::vector<std::string> GetPendingRequestIds() const;
    std::vector<std::string> GetCompletedImageIds() const;
    std::vector<std::string> GetImagesByCategory(const std::string& category) const;

    void CancelRequest(const std::string& id);
    void ClearCache();
    void ClearCategory(const std::string& category);

    void SetMaxCacheSize(std::size_t max_images) { max_cache_size_ = max_images; }
    std::size_t GetCacheSize() const;

    void SaveImageToFile(const std::string& id, const std::string& file_path);
    bool LoadImageFromFile(const std::string& id, const std::string& file_path);

    void ProcessAsyncResults();
    bool HasPendingResults() const { return !completed_queue_.empty(); }

    std::size_t GetPendingCount() const;
    std::size_t GetCompletedCount() const;

private:
    ImageGenerationManager() = default;
    ~ImageGenerationManager() = default;

    ImageGenerationManager(const ImageGenerationManager&) = delete;
    ImageGenerationManager& operator=(const ImageGenerationManager&) = delete;

    void WorkerThread_();
    void ProcessCompletedRequest_(const GeneratedImage& image);
    void UpdateCache_();

    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};

    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, GeneratedImage> image_cache_;
    std::size_t max_cache_size_ = 100;

    mutable std::mutex queue_mutex_;
    std::queue<ImageGenerationRequest> request_queue_;
    std::queue<GeneratedImage> completed_queue_;
    std::condition_variable queue_cv_;

    std::thread worker_thread_;
    std::atomic<std::size_t> request_counter_{0};
};

} // namespace CloudSeamanor::engine