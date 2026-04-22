#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace {

// 互斥锁用于保护日志写入，避免多处同时写文件时互相覆盖。
std::mutex g_logger_mutex;

// 当前日志文件路径。
std::filesystem::path g_log_file_path;

// 日志最大允许大小，超过后会截断。
std::uintmax_t g_max_bytes = 1024 * 1024;

// 标记日志系统是否已经初始化。
bool g_initialized = false;

// 生成当前时间戳，用于写入每条日志的前缀。
// 这样做的好处是后续排查问题时能明确知道每条记录发生的时间。
std::string CurrentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

// 在真正写日志前检查文件大小是否超限。
// 这样做的目的是避免日志无限膨胀，占用磁盘或影响查看效率。
void TruncateIfNeededLocked() {
    if (g_log_file_path.empty() || !std::filesystem::exists(g_log_file_path)) {
        return;
    }

    const auto size = std::filesystem::file_size(g_log_file_path);
    if (size <= g_max_bytes) {
        return;
    }

    std::ofstream truncate_stream(g_log_file_path, std::ios::trunc);
    if (!truncate_stream.is_open()) {
        return;
    }

    // 被截断后补一条说明，避免后面查看日志时不知道为什么前面内容没了。
    truncate_stream << "[" << CurrentTimestamp() << "] [WARN] Log file exceeded size limit and was cleared.\n";
}

// 在锁已经持有的前提下追加一条日志。
// 分成 locked 版本的好处是：Initialize / Write 等外部函数可以复用这套逻辑而不重复加锁。
void AppendLineLocked(std::string_view level, std::string_view message) {
    TruncateIfNeededLocked();

    std::ofstream stream(g_log_file_path, std::ios::app);
    if (!stream.is_open()) {
        return;
    }

    stream << "[" << CurrentTimestamp() << "] [" << level << "] " << message << '\n';
}

} // namespace

namespace CloudSeamanor::infrastructure {

void Logger::Initialize(const std::filesystem::path& log_directory,
                        std::string_view file_name,
                        std::uintmax_t max_bytes) {
    std::scoped_lock lock(g_logger_mutex);

    // 启动时先确保日志目录存在。
    std::filesystem::create_directories(log_directory);
    g_log_file_path = log_directory / file_name;
    g_max_bytes = max_bytes;
    g_initialized = true;

    AppendLineLocked("INFO", "Logger initialized.");
}

void Logger::Info(std::string_view message) {
    Write("INFO", message);
}

void Logger::Warning(std::string_view message) {
    Write("WARN", message);
}

void Logger::Error(std::string_view message) {
    Write("ERROR", message);
}

void Logger::Shutdown() {
    // 关闭前仍然写一条记录，帮助确认程序是正常退出还是异常中止。
    Write("INFO", "Logger shutdown.");
}

void Logger::Write(std::string_view level, std::string_view message) {
    std::scoped_lock lock(g_logger_mutex);

    // 只有初始化完成后才允许写入。
    if (!g_initialized || g_log_file_path.empty()) {
        return;
    }

    AppendLineLocked(level, message);
}

} // namespace CloudSeamanor::infrastructure
