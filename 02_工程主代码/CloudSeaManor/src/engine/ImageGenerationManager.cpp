#include "engine/ImageGenerationManager.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include <SFML/Graphics/Image.hpp>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace CloudSeamanor::engine {

ImageGenerationManager& ImageGenerationManager::Instance() {
    static ImageGenerationManager instance;
    return instance;
}

void ImageGenerationManager::Initialize(const std::string& api_key) {
    if (initialized_.load()) {
        Logger::Warning("ImageGenerationManager already initialized");
        return;
    }

    infrastructure::NvidiaImageGenerator::Initialize(api_key);
    
    shutdown_requested_.store(false);
    worker_thread_ = std::thread(&ImageGenerationManager::WorkerThread_, this);
    
    initialized_.store(true);
    Logger::Info("ImageGenerationManager initialized successfully");
}

void ImageGenerationManager::Shutdown() {
    if (!initialized_.load()) {
        return;
    }

    shutdown_requested_.store(true);
    queue_cv_.notify_all();

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    infrastructure::NvidiaImageGenerator::Shutdown();
    
    ClearCache();
    initialized_.store(false);
    Logger::Info("ImageGenerationManager shutdown complete");
}

std::string ImageGenerationManager::SubmitRequest(const ImageGenerationRequest& request) {
    if (!initialized_.load()) {
        Logger::Error("ImageGenerationManager not initialized");
        return "";
    }

    std::string id = request.id;
    if (id.empty()) {
        std::ostringstream oss;
        oss << "img_" << std::setfill('0') << std::setw(8) 
            << request_counter_.fetch_add(1) << "_" 
            << std::chrono::system_clock::now().time_since_epoch().count();
        id = oss.str();
    }

    ImageGenerationRequest internal_request = request;
    internal_request.id = id;

    GeneratedImage image;
    image.id = id;
    image.prompt = request.prompt;
    image.category = request.category;
    image.status = ImageGenerationStatus::Generating;

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        image_cache_[id] = image;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(internal_request);
    }
    queue_cv_.notify_one();

    Logger::Info("Image generation request submitted: " + id);
    return id;
}

std::string ImageGenerationManager::SubmitCharacterPortraitRequest(
    const std::string& character_name,
    const std::string& template_id,
    std::function<void(const GeneratedImage&)> callback
) {
    const auto& templates = prompt_templates::PromptTemplateLibrary::GetCharacterPortraitTemplates();
    
    std::string prompt;
    for (const auto& tmpl : templates) {
        if (tmpl.category == template_id) {
            prompt = tmpl.template_prompt;
            break;
        }
    }

    if (prompt.empty()) {
        prompt = "beautiful anime character portrait, " + character_name + 
                ", elegant chinese xianxia style, detailed features, high quality illustration";
    }

    ImageGenerationRequest request;
    request.id = "portrait_" + character_name;
    request.prompt = prompt;
    request.negative_prompt = "low quality, blurry, bad anatomy, deformed, ugly, bad hands, extra fingers";
    request.category = "character_portrait";
    request.width = 1024;
    request.height = 1024;
    request.aspect_ratio = "1:1";
    request.callback = callback;

    return SubmitRequest(request);
}

std::string ImageGenerationManager::SubmitGameSceneRequest(
    const std::string& scene_name,
    const std::string& template_id,
    std::function<void(const GeneratedImage&)> callback
) {
    const auto& templates = prompt_templates::PromptTemplateLibrary::GetGameSceneTemplates();
    
    std::string prompt;
    for (const auto& tmpl : templates) {
        if (tmpl.category == template_id) {
            prompt = tmpl.template_prompt;
            break;
        }
    }

    if (prompt.empty()) {
        prompt = "beautiful Chinese xianxia fantasy landscape, " + scene_name + 
                ", detailed environment, high quality game art";
    }

    ImageGenerationRequest request;
    request.id = "scene_" + scene_name;
    request.prompt = prompt;
    request.negative_prompt = "low quality, blurry, watermark, text, logo, signature, deformed buildings";
    request.category = "game_scene";
    request.width = 1920;
    request.height = 1080;
    request.aspect_ratio = "16:9";
    request.callback = callback;

    return SubmitRequest(request);
}

std::optional<GeneratedImage> ImageGenerationManager::GetGeneratedImage(const std::string& id) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = image_cache_.find(id);
    if (it != image_cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::shared_ptr<sf::Texture> ImageGenerationManager::GetTexture(const std::string& id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = image_cache_.find(id);
    if (it != image_cache_.end() && it->second.status == ImageGenerationStatus::Success) {
        if (!it->second.texture) {
            it->second.texture = std::make_shared<sf::Texture>();
            sf::Image image;
            if (image.loadFromMemory(it->second.image_data.data(), it->second.image_data.size())) {
                it->second.texture->loadFromImage(image);
            }
        }
        return it->second.texture;
    }
    return nullptr;
}

ImageGenerationStatus ImageGenerationManager::GetStatus(const std::string& id) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = image_cache_.find(id);
    if (it != image_cache_.end()) {
        return it->second.status;
    }
    return ImageGenerationStatus::Idle;
}

std::vector<std::string> ImageGenerationManager::GetPendingRequestIds() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, image] : image_cache_) {
        if (image.status == ImageGenerationStatus::Generating) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::string> ImageGenerationManager::GetCompletedImageIds() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, image] : image_cache_) {
        if (image.status == ImageGenerationStatus::Success) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::string> ImageGenerationManager::GetImagesByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, image] : image_cache_) {
        if (image.category == category && image.status == ImageGenerationStatus::Success) {
            ids.push_back(id);
        }
    }
    return ids;
}

void ImageGenerationManager::CancelRequest(const std::string& id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = image_cache_.find(id);
    if (it != image_cache_.end() && it->second.status == ImageGenerationStatus::Generating) {
        it->second.status = ImageGenerationStatus::Failed;
        it->second.error_message = "Cancelled by user";
        Logger::Info("Image generation cancelled: " + id);
    }
}

void ImageGenerationManager::ClearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    image_cache_.clear();
    Logger::Info("Image cache cleared");
}

void ImageGenerationManager::ClearCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (auto it = image_cache_.begin(); it != image_cache_.end(); ) {
        if (it->second.category == category) {
            it = image_cache_.erase(it);
        } else {
            ++it;
        }
    }
    Logger::Info("Cleared images in category: " + category);
}

std::size_t ImageGenerationManager::GetCacheSize() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return image_cache_.size();
}

void ImageGenerationManager::SaveImageToFile(const std::string& id, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = image_cache_.find(id);
    if (it != image_cache_.end() && it->second.status == ImageGenerationStatus::Success) {
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        std::ofstream out_file(file_path, std::ios::binary);
        if (out_file.is_open()) {
            out_file.write(reinterpret_cast<const char*>(it->second.image_data.data()),
                          static_cast<std::streamsize>(it->second.image_data.size()));
            out_file.close();
            Logger::Info("Image saved to file: " + file_path);
        } else {
            Logger::Error("Failed to open file for writing: " + file_path);
        }
    }
}

bool ImageGenerationManager::LoadImageFromFile(const std::string& id, const std::string& file_path) {
    std::ifstream in_file(file_path, std::ios::binary | std::ios::ate);
    if (!in_file.is_open()) {
        Logger::Error("Failed to open file for reading: " + file_path);
        return false;
    }

    auto size = in_file.tellg();
    in_file.seekg(0, std::ios::beg);

    GeneratedImage image;
    image.id = id;
    image.image_data.resize(static_cast<std::size_t>(size));
    in_file.read(reinterpret_cast<char*>(image.image_data.data()), size);
    in_file.close();

    image.status = ImageGenerationStatus::Success;
    image.generated_at = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        image_cache_[id] = image;
    }

    Logger::Info("Image loaded from file: " + file_path);
    return true;
}

void ImageGenerationManager::ProcessAsyncResults() {
    std::queue<GeneratedImage> to_process;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        to_process = std::move(completed_queue_);
        completed_queue_ = std::queue<GeneratedImage>();
    }

    while (!to_process.empty()) {
        ProcessCompletedRequest_(to_process.front());
        to_process.pop();
    }
}

std::size_t ImageGenerationManager::GetPendingCount() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::size_t count = 0;
    for (const auto& [id, image] : image_cache_) {
        if (image.status == ImageGenerationStatus::Generating) {
            ++count;
        }
    }
    return count;
}

std::size_t ImageGenerationManager::GetCompletedCount() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::size_t count = 0;
    for (const auto& [id, image] : image_cache_) {
        if (image.status == ImageGenerationStatus::Success) {
            ++count;
        }
    }
    return count;
}

void ImageGenerationManager::WorkerThread_() {
    while (!shutdown_requested_.load()) {
        ImageGenerationRequest request;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !request_queue_.empty() || shutdown_requested_.load();
            });

            if (shutdown_requested_.load()) {
                break;
            }

            if (request_queue_.empty()) {
                continue;
            }

            request = request_queue_.front();
            request_queue_.pop();
        }

        infrastructure::NvidiaImageRequest api_request;
        api_request.prompt = request.prompt;
        api_request.negative_prompt = request.negative_prompt;
        api_request.width = request.width;
        api_request.height = request.height;
        api_request.steps = request.steps;
        api_request.cfg_scale = request.cfg_scale;
        api_request.seed = request.seed;
        api_request.aspect_ratio = request.aspect_ratio;

        auto response = infrastructure::NvidiaImageGenerator::GenerateImageSync(api_request);

        GeneratedImage result;
        result.id = request.id;
        result.prompt = request.prompt;
        result.category = request.category;
        result.generated_at = std::chrono::system_clock::now();

        if (response && response->success) {
            result.image_data = std::move(response->image_data);
            result.status = ImageGenerationStatus::Success;
            Logger::Info("Image generation completed: " + request.id);
        } else {
            result.status = ImageGenerationStatus::Failed;
            result.error_message = response ? response->error_message : "Unknown error";
            Logger::Error("Image generation failed: " + request.id + " - " + result.error_message);
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            completed_queue_.push(result);
        }

        if (request.callback) {
            request.callback(result);
        }
    }
}

void ImageGenerationManager::ProcessCompletedRequest_(const GeneratedImage& image) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    image_cache_[image.id] = image;
    UpdateCache_();
}

void ImageGenerationManager::UpdateCache_() {
    if (image_cache_.size() > max_cache_size_) {
        std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> to_remove;
        
        for (const auto& [id, image] : image_cache_) {
            if (image.status != ImageGenerationStatus::Generating) {
                to_remove.emplace_back(id, image.generated_at);
            }
        }

        std::sort(to_remove.begin(), to_remove.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        std::size_t remove_count = image_cache_.size() - max_cache_size_;
        for (std::size_t i = 0; i < remove_count && i < to_remove.size(); ++i) {
            image_cache_.erase(to_remove[i].first);
        }

        Logger::Info("Cache cleanup: removed " + std::to_string(std::min(remove_count, to_remove.size())) + " images");
    }
}

} // namespace CloudSeamanor::engine