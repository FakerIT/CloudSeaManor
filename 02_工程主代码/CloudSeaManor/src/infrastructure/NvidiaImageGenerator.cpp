#include "infrastructure/NvidiaImageGenerator.hpp"
#include <curl/curl.h>
#include <json/json.h>
#include <chrono>
#include <thread>
#include <cstring>

namespace CloudSeamanor::infrastructure {

namespace {
    std::string g_api_key;
    std::string g_model = "stable-diffusion-3-medium";
    std::string g_api_endpoint = "https://ai.api.nvidia.com/v1/genai/stabilityai/stable-diffusion-3-medium";
    bool g_initialized = false;
    int g_timeout_seconds = 120;
    thread_local std::string g_last_error;
}

void NvidiaImageGenerator::Initialize(const std::string& api_key, const std::string& model) {
    g_api_key = api_key;
    g_model = model;
    g_initialized = true;
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void NvidiaImageGenerator::Shutdown() {
    g_initialized = false;
    g_api_key.clear();
    curl_global_cleanup();
}

bool NvidiaImageGenerator::IsInitialized() {
    return g_initialized;
}

std::string NvidiaImageGenerator::GetLastError() {
    return g_last_error;
}

void NvidiaImageGenerator::SetTimeout(int timeout_seconds) {
    g_timeout_seconds = timeout_seconds;
}

size_t NvidiaImageGenerator::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* response = static_cast<NvidiaImageResponse*>(userp);

    if (response->image_data.size() < 1024 * 1024 * 10) {
        auto* byte_ptr = static_cast<std::uint8_t*>(contents);
        response->image_data.insert(response->image_data.end(), byte_ptr, byte_ptr + realsize);
    }

    return realsize;
}

size_t NvidiaImageGenerator::HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* header = static_cast<char*>(contents);
    auto* response = static_cast<NvidiaImageResponse*>(userp);

    std::string header_str(header, realsize);
    if (header_str.find("content-type:") == 0) {
        std::string content_type = header_str.substr(13);
        content_type.erase(0, content_type.find_first_not_of(" \t"));
        content_type.erase(content_type.find_last_not_of(" \t\r\n") + 1);
        if (content_type.find("image/") != std::string::npos) {
            if (content_type.find("png") != std::string::npos) {
                response->image_format = "png";
            } else if (content_type.find("jpeg") != std::string::npos || content_type.find("jpg") != std::string::npos) {
                response->image_format = "jpeg";
            } else if (content_type.find("webp") != std::string::npos) {
                response->image_format = "webp";
            }
        }
    }

    return realsize;
}

void NvidiaImageGenerator::GenerateImageAsync(
    const NvidiaImageRequest& request,
    std::function<void(NvidiaImageResponse)> callback
) {
    std::thread([request, callback]() {
        auto response = GenerateImageSync(request);
        callback(response.value_or(NvidiaImageResponse{}));
    }).detach();
}

std::optional<NvidiaImageResponse> NvidiaImageGenerator::GenerateImageSync(const NvidiaImageRequest& request) {
    if (!g_initialized) {
        g_last_error = "NvidiaImageGenerator not initialized. Call Initialize() first.";
        return std::nullopt;
    }

    NvidiaImageResponse response;

    CURL* curl = curl_easy_init();
    if (!curl) {
        g_last_error = "Failed to initialize CURL";
        return std::nullopt;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + g_api_key).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: image/png,image/jpeg,image/webp");

    Json::Value json_request;
    json_request["prompt"] = request.prompt;
    json_request["aspect_ratio"] = request.aspect_ratio;
    json_request["cfg_scale"] = request.cfg_scale;
    json_request["mode"] = "text-to-image";
    json_request["model"] = "sd3";
    json_request["seed"] = request.seed;
    json_request["steps"] = request.steps;

    if (!request.negative_prompt.empty()) {
        json_request["negative_prompt"] = request.negative_prompt;
    }

    Json::StreamWriterBuilder writer;
    std::string json_str = Json::writeString(writer, json_request);

    curl_easy_setopt(curl, CURLOPT_URL, g_api_endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        g_last_error = std::string("CURL error: ") + curl_easy_strerror(res);
        response.success = false;
        response.error_message = g_last_error;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code >= 200 && http_code < 300) {
            response.success = true;
            if (response.image_format.empty()) {
                response.image_format = "png";
            }
        } else {
            response.success = false;
            response.error_message = "HTTP error: " + std::to_string(http_code);
            g_last_error = response.error_message;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

} // namespace CloudSeamanor::infrastructure