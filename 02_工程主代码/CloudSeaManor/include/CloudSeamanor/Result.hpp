#pragma once

#include <string>
#include <utility>

namespace CloudSeamanor {

template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : ok_(true) { new(&value_) T(std::move(value)); }

    Result(const char* error) : ok_(false) { new(&error_) std::string(error); }

    Result(std::string error) : ok_(false) { new(&error_) std::string(std::move(error)); }

    ~Result() {
        if (ok_) {
            value_.~T();
        } else {
            error_.~basic_string();
        }
    }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&& other) noexcept : ok_(other.ok_) {
        if (ok_) {
            new(&value_) T(std::move(other.value_));
        } else {
            new(&error_) std::string(std::move(other.error_));
        }
    }

    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (ok_) value_.~T(); else error_.~basic_string();
            ok_ = other.ok_;
            if (ok_) {
                new(&value_) T(std::move(other.value_));
            } else {
                new(&error_) std::string(std::move(other.error_));
            }
        }
        return *this;
    }

    [[nodiscard]] bool Ok() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }

    [[nodiscard]] T& Value() & { return value_; }
    [[nodiscard]] const T& Value() const& { return value_; }
    [[nodiscard]] T&& Value() && { return std::move(value_); }

    [[nodiscard]] const std::string& Error() const noexcept { return error_; }

    [[nodiscard]] T ValueOr(T fallback) const {
        return ok_ ? value_ : std::move(fallback);
    }

private:
    union {
        T value_;
        std::string error_;
    };
    bool ok_;
};

template <>
class [[nodiscard]] Result<void> {
public:
    Result() : ok_(true) {}

    Result(const char* error) : error_(error), ok_(false) {}

    Result(std::string error) : error_(std::move(error)), ok_(false) {}

    [[nodiscard]] bool Ok() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }

    [[nodiscard]] const std::string& Error() const noexcept { return error_; }

private:
    std::string error_;
    bool ok_;
};

}  // namespace CloudSeamanor
