#include "CloudSeamanor/infrastructure/JsonValue.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace CloudSeamanor::infrastructure {

namespace {

struct ParseContext {
    const std::string& text;
    std::size_t pos = 0;

    void SkipWhitespace() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    }

    [[nodiscard]] char Peek() const {
        return pos < text.size() ? text[pos] : '\0';
    }

    char Advance() {
        return pos < text.size() ? text[pos++] : '\0';
    }

    [[nodiscard]] bool AtEnd() const { return pos >= text.size(); }

    [[nodiscard]] std::string ParseStringContent() {
        std::string result;
        while (!AtEnd()) {
            char c = Advance();
            if (c == '"') break;
            if (c == '\\' && !AtEnd()) {
                char esc = Advance();
                switch (esc) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    result += "\\u";
                    for (int i = 0; i < 4 && !AtEnd(); ++i) {
                        result += Advance();
                    }
                    break;
                }
                default: result += esc; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    [[nodiscard]] JsonValue ParseValue() {
        SkipWhitespace();
        if (AtEnd()) return JsonValue();

        char c = Peek();
        if (c == '"') return ParseString();
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == 't' || c == 'f') return ParseBool();
        if (c == 'n') return ParseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber();
        return JsonValue();
    }

    [[nodiscard]] JsonValue ParseString() {
        Advance();
        return JsonValue(ParseStringContent());
    }

    [[nodiscard]] JsonValue ParseNumber() {
        std::string num_str;
        bool is_float = false;
        if (Peek() == '-') {
            num_str += Advance();
        }
        while (!AtEnd() && (std::isdigit(static_cast<unsigned char>(Peek())) || Peek() == '.' || Peek() == 'e' || Peek() == 'E' || Peek() == '+' || Peek() == '-')) {
            if (Peek() == '.' || Peek() == 'e' || Peek() == 'E') is_float = true;
            num_str += Advance();
        }
        if (is_float) {
            return JsonValue(std::stod(num_str));
        }
        return JsonValue(static_cast<std::int64_t>(std::stoll(num_str)));
    }

    [[nodiscard]] JsonValue ParseBool() {
        if (text.compare(pos, 4, "true") == 0) {
            pos += 4;
            return JsonValue(true);
        }
        if (text.compare(pos, 5, "false") == 0) {
            pos += 5;
            return JsonValue(false);
        }
        return JsonValue();
    }

    [[nodiscard]] JsonValue ParseNull() {
        if (text.compare(pos, 4, "null") == 0) {
            pos += 4;
        }
        return JsonValue();
    }

    [[nodiscard]] JsonValue ParseArray() {
        Advance();
        auto arr = JsonValue::Array();
        SkipWhitespace();
        if (Peek() == ']') { Advance(); return arr; }
        while (!AtEnd()) {
            arr.PushBack(ParseValue());
            SkipWhitespace();
            if (Peek() == ',') { Advance(); continue; }
            if (Peek() == ']') { Advance(); break; }
            break;
        }
        return arr;
    }

    [[nodiscard]] JsonValue ParseObject() {
        Advance();
        auto obj = JsonValue::Object();
        SkipWhitespace();
        if (Peek() == '}') { Advance(); return obj; }
        while (!AtEnd()) {
            SkipWhitespace();
            if (Peek() != '"') break;
            Advance();
            std::string key = ParseStringContent();
            SkipWhitespace();
            if (Peek() == ':') Advance();
            obj.Insert(key, ParseValue());
            SkipWhitespace();
            if (Peek() == ',') { Advance(); continue; }
            if (Peek() == '}') { Advance(); break; }
            break;
        }
        return obj;
    }
};

}  // namespace

JsonValue JsonValue::Parse(const std::string& json_text) {
    ParseContext ctx{json_text, 0};
    return ctx.ParseValue();
}

}  // namespace CloudSeamanor::infrastructure
