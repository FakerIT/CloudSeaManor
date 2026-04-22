#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace CloudSeamanor::infrastructure {

// 轻量日志系统。
// 作用是把运行过程中的关键信息写到日志文件里，方便排查问题。
// 当前采用静态类设计，是因为原型阶段通常只需要全局唯一的一份日志输出入口。
class Logger {
public:
    // 初始化日志系统，指定目录、文件名和最大体积。
    // 这样做的好处是日志策略集中管理，后续替换实现也不影响业务代码调用方式。
    static void Initialize(const std::filesystem::path& log_directory,
                           std::string_view file_name = "CloudSeamanor.log",
                           std::uintmax_t max_bytes = 1024 * 1024);

    // 三种常用日志级别。
    // 业务代码只需要表达“这是信息、警告还是错误”，不用关心写文件细节。
    static void Info(std::string_view message);
    static void Warning(std::string_view message);
    static void Error(std::string_view message);

    // 关闭日志系统。
    // 当前主要用于在程序退出前补一条结束记录。
    static void Shutdown();

private:
    // 统一底层写入入口，避免重复代码。
    static void Write(std::string_view level, std::string_view message);
};

} // namespace CloudSeamanor::infrastructure
