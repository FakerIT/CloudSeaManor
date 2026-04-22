#pragma once

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/String.hpp>

#include <string>

namespace CloudSeamanor::rendering {

// ============================================================================
// 【toSfString】将 UTF-8 std::string 安全转换为 sf::String
//
// SFML 3 的 sf::Text::setString() 接受 sf::String（本质是 std::u32string）。
// 若直接传入本地编码的 std::string，SFML 会将其当作 Latin-1 或 ANSI 编码
// 处理，导致汉字显示为方块。本函数先将 UTF-8 字节流解码为 UTF-32 码点，
// 再构造 sf::String，确保汉字正确渲染。
//
// 与 std::wstring 的区别：
//   - wchar_t 在 Windows 上为 16-bit（UTF-16），在 Linux/macOS 上为 32-bit
//     但 char16_t/char32_t 在所有平台上都是固定宽度，更安全。
//   - SFML 内部使用 char32_t 存储文本，本函数直接构造 char32_t 序列，
//     避免跨平台 wchar_t 宽度不一的隐患。
// ============================================================================
sf::String toSfString(const std::string& utf8_string);

// ============================================================================
// 【toUtf8】将 sf::String 转换回 UTF-8 std::string
// ============================================================================
std::string toUtf8(const sf::String& sf_string);

// ============================================================================
// 【TextFactory】线程安全的字体单例 + 文本快捷构造
//
// 使用 C++11 static-local 特性实现 Meyers' Singleton，
// 保证初始化线程安全（仅在首次调用时执行，编译器保证原子性）。
// 对外只暴露静态方法，不持有任何成员状态。
// ============================================================================
class TextFactory {
public:
    TextFactory(const TextFactory&) = delete;
    TextFactory& operator=(const TextFactory&) = delete;

    // 加载字体文件路径列表，按顺序尝试，返回第一个成功的路径（用于日志）
    static const std::string& LoadFont(const std::vector<std::string>& paths);

    // 检查字体是否已成功加载
    static bool isFontLoaded();

    // 获取已加载字体的引用（未加载时返回 nullptr）
    static const sf::Font* GetFont();

    // 快捷构造一个 sf::Text 对象（内部自动使用已加载字体）
    // 失败时返回 nullptr
    static std::unique_ptr<sf::Text> makeText(unsigned int character_size,
                                              const std::string& utf8_string = "");

private:
    TextFactory() = default;

    static const sf::Font* fontInstance();
};

} // namespace CloudSeamanor::rendering
