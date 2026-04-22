#include "CloudSeamanor/TextRenderUtils.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace CloudSeamanor::rendering {

// ============================================================================
// 【toSfString】UTF-8 -> sf::String (char32_t)
// ============================================================================
sf::String toSfString(const std::string& utf8_string) {
    std::u32string result;
    result.reserve(utf8_string.size());

    for (std::size_t i = 0; i < utf8_string.size();) {
        const unsigned char c = static_cast<unsigned char>(utf8_string[i]);

        std::uint32_t cp = 0;
        int codepoint_len = 0;

        if (c < 0x80) {
            // ASCII
            cp = c;
            codepoint_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte: 110xxxxx 10xxxxxx
            if (i + 1 >= utf8_string.size()) break;
            cp = ((c & 0x1F) << 6)
               | (static_cast<unsigned char>(utf8_string[i + 1]) & 0x3F);
            codepoint_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
            if (i + 2 >= utf8_string.size()) break;
            cp = ((c & 0x0F) << 12)
               | ((static_cast<unsigned char>(utf8_string[i + 1]) & 0x3F) << 6)
               | (static_cast<unsigned char>(utf8_string[i + 2]) & 0x3F);
            codepoint_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            if (i + 3 >= utf8_string.size()) break;
            cp = ((c & 0x07) << 18)
               | ((static_cast<unsigned char>(utf8_string[i + 1]) & 0x3F) << 12)
               | ((static_cast<unsigned char>(utf8_string[i + 2]) & 0x3F) << 6)
               | (static_cast<unsigned char>(utf8_string[i + 3]) & 0x3F);
            codepoint_len = 4;
        } else {
            // 非法字节，跳过
            ++i;
            continue;
        }

        result.push_back(static_cast<char32_t>(cp));
        i += codepoint_len;
    }

    return sf::String(result);
}

// ============================================================================
// 【toUtf8】sf::String (char32_t) -> UTF-8 std::string
// ============================================================================
std::string toUtf8(const sf::String& sf_string) {
    std::string result;
    for (const char32_t cp : sf_string) {
        if (cp < 0x80) {
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return result;
}

// ============================================================================
// 【TextFactory 内部字体管理】
// ============================================================================
namespace detail {
struct FontStorage {
    std::unique_ptr<sf::Font> font;
    std::string loaded_path; // 记录哪个路径加载成功
    bool loaded = false;

    bool tryLoad(const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            font = std::make_unique<sf::Font>();
            if (font->openFromFile(path)) {
                loaded_path = path;
                loaded = true;
                return true;
            }
        }
        font.reset();
        return false;
    }
};

inline FontStorage& GlobalFontStorage() {
    static FontStorage instance;
    return instance;
}
} // namespace detail

const std::string& TextFactory::LoadFont(const std::vector<std::string>& paths) {
    detail::GlobalFontStorage().tryLoad(paths);
    static std::string empty;
    if (detail::GlobalFontStorage().loaded) {
        return detail::GlobalFontStorage().loaded_path;
    }
    return empty;
}

bool TextFactory::isFontLoaded() {
    return detail::GlobalFontStorage().loaded;
}

const sf::Font* TextFactory::GetFont() {
    if (!detail::GlobalFontStorage().loaded) {
        return nullptr;
    }
    return detail::GlobalFontStorage().font.get();
}

const sf::Font* TextFactory::fontInstance() {
    return GetFont();
}

std::unique_ptr<sf::Text> TextFactory::makeText(unsigned int character_size,
                                                const std::string& utf8_string) {
    const sf::Font* f = fontInstance();
    if (!f) return nullptr;

    auto text = std::make_unique<sf::Text>(*f);
    text->setCharacterSize(character_size);
    if (!utf8_string.empty()) {
        text->setString(toSfString(utf8_string));
    }
    return text;
}

} // namespace CloudSeamanor::rendering
