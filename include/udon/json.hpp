#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace udon {

class JsonError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, Array, Object>;

    JsonValue() = default;
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(double value);
    JsonValue(std::int64_t value);
    JsonValue(std::string value);
    JsonValue(const char* value);
    JsonValue(Array value);
    JsonValue(Object value);

    [[nodiscard]] static JsonValue parse(const std::string& source);
    [[nodiscard]] std::string dump() const;

    [[nodiscard]] bool is_null() const;
    [[nodiscard]] bool is_bool() const;
    [[nodiscard]] bool is_number() const;
    [[nodiscard]] bool is_integer_number() const;
    [[nodiscard]] bool is_string() const;
    [[nodiscard]] bool is_array() const;
    [[nodiscard]] bool is_object() const;

    [[nodiscard]] bool boolean() const;
    [[nodiscard]] double number() const;
    [[nodiscard]] std::int64_t integer() const;
    [[nodiscard]] const std::string& string() const;
    [[nodiscard]] const Array& array() const;
    [[nodiscard]] Array& array();
    [[nodiscard]] const Object& object() const;
    [[nodiscard]] Object& object();
    [[nodiscard]] const JsonValue& at(const std::string& key) const;
    [[nodiscard]] bool contains(const std::string& key) const;

private:
    Storage storage_ = nullptr;
};

}
