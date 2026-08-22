#include "udon/json.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

namespace udon {

namespace {

class Parser {
public:
    explicit Parser(std::string_view source)
        : source_(source) {}

    [[nodiscard]] JsonValue parse_document() {
        skip_whitespace();
        JsonValue result = parse_value(0);
        skip_whitespace();
        if (offset_ != source_.size()) {
            fail("unexpected trailing data");
        }
        return result;
    }

private:
    [[nodiscard]] JsonValue parse_value(std::size_t depth) {
        if (depth > 512U) {
            fail("JSON nesting exceeds 512 levels");
        }
        if (offset_ >= source_.size()) {
            fail("expected JSON value");
        }
        switch (source_[offset_]) {
        case 'n':
            consume_literal("null");
            return JsonValue(nullptr);
        case 't':
            consume_literal("true");
            return JsonValue(true);
        case 'f':
            consume_literal("false");
            return JsonValue(false);
        case '"':
            return JsonValue(parse_string());
        case '[':
            return JsonValue(parse_array(depth + 1U));
        case '{':
            return JsonValue(parse_object(depth + 1U));
        default:
            if (source_[offset_] == '-' || is_digit(source_[offset_])) {
                return parse_number();
            }
            fail("invalid JSON value");
        }
    }

    [[nodiscard]] JsonValue::Array parse_array(std::size_t depth) {
        expect('[');
        skip_whitespace();
        JsonValue::Array values;
        if (consume_if(']')) {
            return values;
        }
        while (true) {
            skip_whitespace();
            values.push_back(parse_value(depth));
            skip_whitespace();
            if (consume_if(']')) {
                return values;
            }
            expect(',');
        }
    }

    [[nodiscard]] JsonValue::Object parse_object(std::size_t depth) {
        expect('{');
        skip_whitespace();
        JsonValue::Object values;
        if (consume_if('}')) {
            return values;
        }
        while (true) {
            skip_whitespace();
            if (offset_ >= source_.size() || source_[offset_] != '"') {
                fail("object key must be a string");
            }
            std::string key = parse_string();
            skip_whitespace();
            expect(':');
            skip_whitespace();
            JsonValue value = parse_value(depth);
            if (!values.emplace(std::move(key), std::move(value)).second) {
                fail("duplicate object key");
            }
            skip_whitespace();
            if (consume_if('}')) {
                return values;
            }
            expect(',');
        }
    }

    [[nodiscard]] std::string parse_string() {
        expect('"');
        std::string result;
        while (offset_ < source_.size()) {
            const unsigned char character = static_cast<unsigned char>(source_[offset_++]);
            if (character == '"') {
                return result;
            }
            if (character < 0x20U) {
                fail("control character in string");
            }
            if (character != '\\') {
                result.push_back(static_cast<char>(character));
                continue;
            }
            if (offset_ >= source_.size()) {
                fail("unterminated escape sequence");
            }
            const char escape = source_[offset_++];
            switch (escape) {
            case '"':
                result.push_back('"');
                break;
            case '\\':
                result.push_back('\\');
                break;
            case '/':
                result.push_back('/');
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'u':
                append_codepoint(result, parse_hex_quad());
                break;
            default:
                fail("invalid string escape");
            }
        }
        fail("unterminated string");
    }

    [[nodiscard]] std::uint32_t parse_hex_quad() {
        if (offset_ + 4U > source_.size()) {
            fail("incomplete unicode escape");
        }
        std::uint32_t codepoint = 0;
        for (std::size_t digitIndex = 0; digitIndex < 4U; ++digitIndex) {
            const char digit = source_[offset_++];
            codepoint <<= 4U;
            if (digit >= '0' && digit <= '9') {
                codepoint |= static_cast<std::uint32_t>(digit - '0');
            } else if (digit >= 'a' && digit <= 'f') {
                codepoint |= static_cast<std::uint32_t>(digit - 'a' + 10);
            } else if (digit >= 'A' && digit <= 'F') {
                codepoint |= static_cast<std::uint32_t>(digit - 'A' + 10);
            } else {
                fail("invalid unicode escape");
            }
        }
        return codepoint;
    }

    void append_codepoint(std::string& output, std::uint32_t codepoint) {
        if (codepoint >= 0xd800U && codepoint <= 0xdbffU) {
            if (offset_ + 2U > source_.size() || source_[offset_] != '\\' || source_[offset_ + 1U] != 'u') {
                fail("high surrogate without low surrogate");
            }
            offset_ += 2U;
            const std::uint32_t lowSurrogate = parse_hex_quad();
            if (lowSurrogate < 0xdc00U || lowSurrogate > 0xdfffU) {
                fail("invalid low surrogate");
            }
            codepoint = 0x10000U + ((codepoint - 0xd800U) << 10U) + (lowSurrogate - 0xdc00U);
        } else if (codepoint >= 0xdc00U && codepoint <= 0xdfffU) {
            fail("low surrogate without high surrogate");
        }

        if (codepoint <= 0x7fU) {
            output.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7ffU) {
            output.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
        } else if (codepoint <= 0xffffU) {
            output.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
        } else if (codepoint <= 0x10ffffU) {
            output.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
        } else {
            fail("unicode codepoint out of range");
        }
    }

    [[nodiscard]] JsonValue parse_number() {
        const std::size_t numberStart = offset_;
        bool integralSyntax = true;
        consume_if('-');
        if (offset_ >= source_.size()) {
            fail("incomplete number");
        }
        if (source_[offset_] == '0') {
            ++offset_;
            if (offset_ < source_.size() && is_digit(source_[offset_])) {
                fail("leading zero in number");
            }
        } else {
            consume_digits();
        }
        if (consume_if('.')) {
            integralSyntax = false;
            const std::size_t fractionStart = offset_;
            consume_digits();
            if (fractionStart == offset_) {
                fail("missing fraction digits");
            }
        }
        if (offset_ < source_.size() && (source_[offset_] == 'e' || source_[offset_] == 'E')) {
            integralSyntax = false;
            ++offset_;
            if (offset_ < source_.size() && (source_[offset_] == '+' || source_[offset_] == '-')) {
                ++offset_;
            }
            const std::size_t exponentStart = offset_;
            consume_digits();
            if (exponentStart == offset_) {
                fail("missing exponent digits");
            }
        }
        const std::string numberText(source_.substr(numberStart, offset_ - numberStart));
        if (integralSyntax) {
            std::int64_t integer = 0;
            const auto [parseEnd, error] = std::from_chars(
                numberText.data(),
                numberText.data() + numberText.size(),
                integer);
            if (error == std::errc{} && parseEnd == numberText.data() + numberText.size()) {
                return JsonValue(integer);
            }
        }
        char* parseEnd = nullptr;
        const double parsed = std::strtod(numberText.c_str(), &parseEnd);
        if (parseEnd == nullptr || *parseEnd != '\0' || !std::isfinite(parsed)) {
            fail("invalid number");
        }
        return JsonValue(parsed);
    }

    void consume_digits() {
        const std::size_t start = offset_;
        while (offset_ < source_.size() && is_digit(source_[offset_])) {
            ++offset_;
        }
        if (offset_ == start) {
            fail("expected digit");
        }
    }

    void consume_literal(std::string_view literal) {
        if (source_.substr(offset_, literal.size()) != literal) {
            fail("invalid literal");
        }
        offset_ += literal.size();
    }

    void skip_whitespace() {
        while (offset_ < source_.size()) {
            const char character = source_[offset_];
            if (character != ' ' && character != '\n' && character != '\r' && character != '\t') {
                return;
            }
            ++offset_;
        }
    }

    bool consume_if(char expected) {
        if (offset_ < source_.size() && source_[offset_] == expected) {
            ++offset_;
            return true;
        }
        return false;
    }

    void expect(char expected) {
        if (!consume_if(expected)) {
            fail(std::string("expected '") + expected + "'");
        }
    }

    [[nodiscard]] static bool is_digit(char character) {
        return character >= '0' && character <= '9';
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw JsonError(message + " at byte " + std::to_string(offset_));
    }

    std::string_view source_;
    std::size_t offset_ = 0;
};

void append_escaped(std::string& output, const std::string& input) {
    output.push_back('"');
    constexpr char hexadecimal[] = "0123456789abcdef";
    for (const unsigned char character : input) {
        switch (character) {
        case '"':
            output += "\\\"";
            break;
        case '\\':
            output += "\\\\";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            if (character < 0x20U) {
                output += "\\u00";
                output.push_back(hexadecimal[(character >> 4U) & 0x0fU]);
                output.push_back(hexadecimal[character & 0x0fU]);
            } else {
                output.push_back(static_cast<char>(character));
            }
            break;
        }
    }
    output.push_back('"');
}

void append_json(std::string& output, const JsonValue& value) {
    if (value.is_null()) {
        output += "null";
        return;
    }
    if (value.is_bool()) {
        output += value.boolean() ? "true" : "false";
        return;
    }
    if (value.is_number()) {
        if (value.is_integer_number()) {
            output += std::to_string(value.integer());
            return;
        }
        const double number = value.number();
        const double integral = std::trunc(number);
        if (integral == number && integral >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
            integral < 9223372036854775808.0) {
            output += std::to_string(static_cast<std::int64_t>(integral));
            return;
        }
        std::ostringstream stream;
        stream << std::setprecision(std::numeric_limits<double>::max_digits10) << number;
        output += stream.str();
        return;
    }
    if (value.is_string()) {
        append_escaped(output, value.string());
        return;
    }
    if (value.is_array()) {
        output.push_back('[');
        bool first = true;
        for (const JsonValue& element : value.array()) {
            if (!first) {
                output.push_back(',');
            }
            first = false;
            append_json(output, element);
        }
        output.push_back(']');
        return;
    }
    output.push_back('{');
    bool first = true;
    for (const auto& [key, member] : value.object()) {
        if (!first) {
            output.push_back(',');
        }
        first = false;
        append_escaped(output, key);
        output.push_back(':');
        append_json(output, member);
    }
    output.push_back('}');
}

template <typename Expected>
const Expected& require_type(const JsonValue::Storage& storage, const char* message) {
    if (const Expected* result = std::get_if<Expected>(&storage)) {
        return *result;
    }
    throw JsonError(message);
}

} 

JsonValue::JsonValue(std::nullptr_t)
    : storage_(nullptr) {}

JsonValue::JsonValue(bool value)
    : storage_(value) {}

JsonValue::JsonValue(double value)
    : storage_(value) {}

JsonValue::JsonValue(std::int64_t value)
    : storage_(value) {}

JsonValue::JsonValue(std::string value)
    : storage_(std::move(value)) {}

JsonValue::JsonValue(const char* value)
    : storage_(std::string(value)) {}

JsonValue::JsonValue(Array value)
    : storage_(std::move(value)) {}

JsonValue::JsonValue(Object value)
    : storage_(std::move(value)) {}

JsonValue JsonValue::parse(const std::string& source) {
    return Parser(source).parse_document();
}

std::string JsonValue::dump() const {
    std::string result;
    result.reserve(256U);
    append_json(result, *this);
    return result;
}

bool JsonValue::is_null() const {
    return std::holds_alternative<std::nullptr_t>(storage_);
}

bool JsonValue::is_bool() const {
    return std::holds_alternative<bool>(storage_);
}

bool JsonValue::is_number() const {
    return std::holds_alternative<std::int64_t>(storage_) ||
        std::holds_alternative<double>(storage_);
}

bool JsonValue::is_integer_number() const {
    return std::holds_alternative<std::int64_t>(storage_);
}

bool JsonValue::is_string() const {
    return std::holds_alternative<std::string>(storage_);
}

bool JsonValue::is_array() const {
    return std::holds_alternative<Array>(storage_);
}

bool JsonValue::is_object() const {
    return std::holds_alternative<Object>(storage_);
}

bool JsonValue::boolean() const {
    return require_type<bool>(storage_, "JSON value is not a boolean");
}

double JsonValue::number() const {
    if (const std::int64_t* integer = std::get_if<std::int64_t>(&storage_)) {
        return static_cast<double>(*integer);
    }
    return require_type<double>(storage_, "JSON value is not a number");
}

std::int64_t JsonValue::integer() const {
    if (const std::int64_t* integer = std::get_if<std::int64_t>(&storage_)) {
        return *integer;
    }
    const double rawNumber = number();
    if (!std::isfinite(rawNumber) || std::trunc(rawNumber) != rawNumber ||
        rawNumber < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
        rawNumber >= 9223372036854775808.0) {
        throw JsonError("JSON number is not a signed 64-bit integer");
    }
    return static_cast<std::int64_t>(rawNumber);
}

const std::string& JsonValue::string() const {
    return require_type<std::string>(storage_, "JSON value is not a string");
}

const JsonValue::Array& JsonValue::array() const {
    return require_type<Array>(storage_, "JSON value is not an array");
}

JsonValue::Array& JsonValue::array() {
    return const_cast<Array&>(static_cast<const JsonValue&>(*this).array());
}

const JsonValue::Object& JsonValue::object() const {
    return require_type<Object>(storage_, "JSON value is not an object");
}

JsonValue::Object& JsonValue::object() {
    return const_cast<Object&>(static_cast<const JsonValue&>(*this).object());
}

const JsonValue& JsonValue::at(const std::string& key) const {
    const Object& values = object();
    const auto iterator = values.find(key);
    if (iterator == values.end()) {
        throw JsonError("missing required JSON key: " + key);
    }
    return iterator->second;
}

bool JsonValue::contains(const std::string& key) const {
    if (!is_object()) {
        return false;
    }
    return object().contains(key);
}

}
