#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::infrastructure {

enum class JsonValueType {
    Null,
    Bool,
    Int,
    Float,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonValue() : type_(JsonValueType::Null) {}
    explicit JsonValue(bool v) : type_(JsonValueType::Bool), bool_val_(v) {}
    explicit JsonValue(std::int64_t v) : type_(JsonValueType::Int), int_val_(v) {}
    explicit JsonValue(double v) : type_(JsonValueType::Float), float_val_(v) {}
    explicit JsonValue(const std::string& v) : type_(JsonValueType::String), str_val_(v) {}
    explicit JsonValue(const char* v) : type_(JsonValueType::String), str_val_(v) {}

    static JsonValue Array() {
        JsonValue v;
        v.type_ = JsonValueType::Array;
        v.arr_val_ = std::vector<JsonValue>();
        return v;
    }

    static JsonValue Object() {
        JsonValue v;
        v.type_ = JsonValueType::Object;
        v.obj_val_ = std::unordered_map<std::string, JsonValue>();
        return v;
    }

    [[nodiscard]] JsonValueType Type() const noexcept { return type_; }
    [[nodiscard]] bool IsNull() const noexcept { return type_ == JsonValueType::Null; }
    [[nodiscard]] bool IsBool() const noexcept { return type_ == JsonValueType::Bool; }
    [[nodiscard]] bool IsInt() const noexcept { return type_ == JsonValueType::Int; }
    [[nodiscard]] bool IsFloat() const noexcept { return type_ == JsonValueType::Float; }
    [[nodiscard]] bool IsString() const noexcept { return type_ == JsonValueType::String; }
    [[nodiscard]] bool IsArray() const noexcept { return type_ == JsonValueType::Array; }
    [[nodiscard]] bool IsObject() const noexcept { return type_ == JsonValueType::Object; }

    [[nodiscard]] bool AsBool() const { return bool_val_; }
    [[nodiscard]] std::int64_t AsInt() const { return int_val_; }
    [[nodiscard]] double AsFloat() const { return type_ == JsonValueType::Int ? static_cast<double>(int_val_) : float_val_; }
    [[nodiscard]] const std::string& AsString() const { return str_val_; }
    [[nodiscard]] const std::vector<JsonValue>& AsArray() const { return arr_val_; }
    [[nodiscard]] const std::unordered_map<std::string, JsonValue>& AsObject() const { return obj_val_; }

    [[nodiscard]] double AsFloatOrDefault(double def) const {
        if (type_ == JsonValueType::Float) return float_val_;
        if (type_ == JsonValueType::Int) return static_cast<double>(int_val_);
        return def;
    }

    [[nodiscard]] std::int64_t AsIntOrDefault(std::int64_t def) const {
        if (type_ == JsonValueType::Int) return int_val_;
        if (type_ == JsonValueType::Float) return static_cast<std::int64_t>(float_val_);
        return def;
    }

    [[nodiscard]] const JsonValue* Get(const std::string& key) const {
        if (type_ != JsonValueType::Object) return nullptr;
        auto it = obj_val_.find(key);
        return it != obj_val_.end() ? &it->second : nullptr;
    }

    [[nodiscard]] const JsonValue& operator[](const std::string& key) const {
        static const JsonValue null_val;
        if (type_ != JsonValueType::Object) return null_val;
        auto it = obj_val_.find(key);
        return it != obj_val_.end() ? it->second : null_val;
    }

    [[nodiscard]] const JsonValue& operator[](std::size_t index) const {
        static const JsonValue null_val;
        if (type_ != JsonValueType::Array || index >= arr_val_.size()) return null_val;
        return arr_val_[index];
    }

    void PushBack(JsonValue v) {
        if (type_ == JsonValueType::Array) {
            arr_val_.push_back(std::move(v));
        }
    }

    void Insert(const std::string& key, JsonValue v) {
        if (type_ == JsonValueType::Object) {
            obj_val_[key] = std::move(v);
        }
    }

    [[nodiscard]] std::size_t Size() const {
        if (type_ == JsonValueType::Array) return arr_val_.size();
        if (type_ == JsonValueType::Object) return obj_val_.size();
        return 0;
    }

    [[nodiscard]] static JsonValue Parse(const std::string& json_text);

private:
    JsonValueType type_;
    bool bool_val_ = false;
    std::int64_t int_val_ = 0;
    double float_val_ = 0.0;
    std::string str_val_;
    std::vector<JsonValue> arr_val_;
    std::unordered_map<std::string, JsonValue> obj_val_;
};

}  // namespace CloudSeamanor::infrastructure
