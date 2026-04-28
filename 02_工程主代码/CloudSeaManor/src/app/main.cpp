#include "CloudSeamanor/GameApp.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <filesystem>
#include <exception>
#include <system_error>
#include <windows.h>

// ============================================================================
// 【SetProjectRootAsCwd】自动切换工作目录到项目根目录（exe 上两级）
//
// 无论从哪个路径启动游戏（IDE / 直接双击 / 快捷方式），
// 所有相对路径（assets/、config/、logs/）都会基于项目根目录解析。
// exe 位于 $(ProjectDir)bin\Debug\ 或 bin\Release\，
// 所以需要向上两级。
// ============================================================================
int main() {
    namespace fs = std::filesystem;

    const auto TrySetCwdToProjectRoot = []() -> void {
        std::wstring module_path;
        module_path.resize(static_cast<std::size_t>(MAX_PATH));
        const DWORD len = ::GetModuleFileNameW(nullptr, module_path.data(), static_cast<DWORD>(module_path.size()));
        if (len == 0) {
            return;
        }
        module_path.resize(static_cast<std::size_t>(len));

        std::error_code ec;
        fs::path exe_dir = fs::path(module_path).parent_path();

        // 从 exe 所在目录向上查找项目根（以 assets/ 或 config/ 作为锚点）。
        // 这样既兼容 build/vs2022-x64/Debug，也兼容未来的 bin/Debug 输出结构。
        fs::path probe = exe_dir;
        for (int i = 0; i < 8; ++i) {
            const fs::path assets_dir = probe / "assets";
            const fs::path config_dir = probe / "config";
            if (fs::exists(assets_dir, ec) || fs::exists(config_dir, ec)) {
                fs::current_path(probe, ec);
                return;
            }
            if (!probe.has_parent_path()) {
                break;
            }
            probe = probe.parent_path();
        }

        // 兜底：至少保证 cwd 在 exe 目录（相对路径更可预测）
        fs::current_path(exe_dir, ec);
    };

    TrySetCwdToProjectRoot();
    CloudSeamanor::infrastructure::Logger::Initialize("logs");

    try {
        CloudSeamanor::engine::GameApp app;
        return app.Run();
    } catch (const std::exception& ex) {
        CloudSeamanor::infrastructure::Logger::Error(
            std::string("Fatal exception: ") + ex.what());
        return 3;
    }
}
