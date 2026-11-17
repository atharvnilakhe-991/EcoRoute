#include "../include/JsonParser.h"
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <iomanip>

static const JsonValue g_nullJson;

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (!isObject()) return g_nullJson;
    auto it = objVal_.find(key);
    return (it != objVal_.end()) ? it->second : g_nullJson;
}

const JsonValue& JsonValue::operator[](size_t idx) const {
    if (!isArray() || idx >= arrVal_.size()) return g_nullJson;
    return arrVal_[idx];
}

std::string JsonValue::serialize(int indent) const {
    std::ostringstream ss;
    switch (type_) {
        case JsonType::Null:
            ss << "null";
            break;
        case JsonType::Bool:
            ss << (boolVal_ ? "true" : "false");
            break;
        case JsonType::Number:
            // Check if it's an integer
            if (numVal_ == static_cast<double>(static_cast<long long>(numVal_))) {
                ss << static_cast<long long>(numVal_);
            } else {
                ss << std::setprecision(10) << numVal_;
            }
            break;
        case JsonType::String: {
            ss << "\"";
            for (char c : strVal_) {
                switch (c) {
                    case '"': ss << "\\\""; break;
                    case '\\': ss << "\\\\"; break;
                    case '\b': ss << "\\b"; break;
                    case '\f': ss << "\\f"; break;
                    case '\n': ss << "\\n"; break;
                    case '\r': ss << "\\r"; break;
                    case '\t': ss << "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c)) << std::dec;
                        } else {
                            ss << c;
                        }
                        break;
                }
            }
            ss << "\"";
            break;
        }
        case JsonType::Array: {
            ss << "[";
            for (size_t i = 0; i < arrVal_.size(); ++i) {
                if (i > 0) ss << ",";
                ss << arrVal_[i].serialize(indent);
            }
            ss << "]";
            break;
        }
        case JsonType::Object: {
            ss << "{";
            bool first = true;
            for (const auto& pair : objVal_) {
                if (!first) ss << ",";
                first = false;
                ss << "\"" << pair.first << "\":" << pair.second.serialize(indent);
            }
            ss << "}";
            break;
        }
    }
    return ss.str();
}

namespace {

class Parser {
private:
    const std::string& src;
    size_t pos{0};
    int line{1};
    int col{1};

    void advance() {
        if (pos < src.size()) {
            if (src[pos] == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
        }
    }

    char peek() const {
        return (pos < src.size()) ? src[pos] : '\0';
    }

    void skipWhitespace() {
        while (pos < src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else {
                break;
            }
        }
    }

    std::string formatError(const std::string& msg) const {
        std::ostringstream ss;
        ss << "JSON parse error at line " << line << ", col " << col << ": " << msg;
        return ss.str();
    }

public:
    explicit Parser(const std::string& s) : src(s) {}

    bool parse(JsonValue& out, std::string& errorMsg) {
        skipWhitespace();
        if (pos >= src.size()) {
            errorMsg = formatError("Empty JSON input");
            return false;
        }
        if (!parseValue(out, errorMsg)) {
            return false;
        }
        skipWhitespace();
        if (pos < src.size()) {
            errorMsg = formatError("Unexpected trailing character after root JSON value");
            return false;
        }
        return true;
    }

private:
    bool parseValue(JsonValue& val, std::string& errorMsg) {
        skipWhitespace();
        char c = peek();
        if (c == '{') return parseObject(val, errorMsg);
        if (c == '[') return parseArray(val, errorMsg);
        if (c == '"') return parseString(val, errorMsg);
        if (c == 't' || c == 'f') return parseBool(val, errorMsg);
        if (c == 'n') return parseNull(val, errorMsg);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(val, errorMsg);

        errorMsg = formatError(std::string("Unexpected character '") + c + "'");
        return false;
    }

    bool parseObject(JsonValue& val, std::string& errorMsg) {
        advance(); // consume '{'
        skipWhitespace();
        std::map<std::string, JsonValue> obj;

        if (peek() == '}') {
            advance(); // empty object
            val = JsonValue(obj);
            return true;
        }

        while (true) {
            skipWhitespace();
            if (peek() != '"') {
                errorMsg = formatError("Expected string key in object");
                return false;
            }
            JsonValue keyVal;
            if (!parseString(keyVal, errorMsg)) return false;
            std::string key = keyVal.asString();

            skipWhitespace();
            if (peek() != ':') {
                errorMsg = formatError("Expected ':' after key in object");
                return false;
            }
            advance(); // consume ':'

            JsonValue memberVal;
            if (!parseValue(memberVal, errorMsg)) return false;
            obj[key] = std::move(memberVal);

            skipWhitespace();
            if (peek() == ',') {
                advance();
                skipWhitespace();
                if (peek() == '}') {
                    errorMsg = formatError("Trailing comma not allowed in JSON object");
                    return false;
                }
            } else if (peek() == '}') {
                advance();
                break;
            } else {
                errorMsg = formatError("Expected ',' or '}' in object");
                return false;
            }
        }

        val = JsonValue(std::move(obj));
        return true;
    }

    bool parseArray(JsonValue& val, std::string& errorMsg) {
        advance(); // consume '['
        skipWhitespace();
        std::vector<JsonValue> arr;

        if (peek() == ']') {
            advance(); // empty array
            val = JsonValue(arr);
            return true;
        }

        while (true) {
            JsonValue item;
            if (!parseValue(item, errorMsg)) return false;
            arr.push_back(std::move(item));

            skipWhitespace();
            if (peek() == ',') {
                advance();
                skipWhitespace();
                if (peek() == ']') {
                    errorMsg = formatError("Trailing comma not allowed in JSON array");
                    return false;
                }
            } else if (peek() == ']') {
                advance();
                break;
            } else {
                errorMsg = formatError("Expected ',' or ']' in array");
                return false;
            }
        }

        val = JsonValue(std::move(arr));
        return true;
    }

    bool parseString(JsonValue& val, std::string& errorMsg) {
        advance(); // consume '"'
        std::string s;
        while (pos < src.size()) {
            char c = src[pos];
            if (c == '"') {
                advance();
                val = JsonValue(s);
                return true;
            }
            if (c == '\\') {
                advance();
                if (pos >= src.size()) {
                    errorMsg = formatError("Unfinished escape sequence in string");
                    return false;
                }
                char esc = src[pos];
                switch (esc) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case '/': s += '/'; break;
                    case 'b': s += '\b'; break;
                    case 'f': s += '\f'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    case 'u': {
                        // 4 hex digits
                        advance();
                        int code = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (pos >= src.size() || !std::isxdigit(static_cast<unsigned char>(src[pos]))) {
                                errorMsg = formatError("Invalid unicode escape sequence");
                                return false;
                            }
                            char h = src[pos];
                            int digit = (h >= '0' && h <= '9') ? (h - '0') :
                                        (h >= 'a' && h <= 'f') ? (h - 'a' + 10) : (h - 'A' + 10);
                            code = (code << 4) | digit;
                            if (i < 3) advance();
                        }
                        if (code < 128) {
                            s += static_cast<char>(code);
                        } else {
                            // Simple UTF-8 encoding for BMP
                            if (code < 0x800) {
                                s += static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
                                s += static_cast<char>(0x80 | (code & 0x3F));
                            } else {
                                s += static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
                                s += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                s += static_cast<char>(0x80 | (code & 0x3F));
                            }
                        }
                        break;
                    }
                    default:
                        errorMsg = formatError(std::string("Invalid escape character '\\") + esc + "'");
                        return false;
                }
                advance();
            } else if (static_cast<unsigned char>(c) < 0x20) {
                errorMsg = formatError("Unescaped control character in string");
                return false;
            } else {
                s += c;
                advance();
            }
        }
        errorMsg = formatError("Unterminated string");
        return false;
    }

    bool parseBool(JsonValue& val, std::string& errorMsg) {
        if (src.compare(pos, 4, "true") == 0) {
            pos += 4;
            col += 4;
            val = JsonValue(true);
            return true;
        }
        if (src.compare(pos, 5, "false") == 0) {
            pos += 5;
            col += 5;
            val = JsonValue(false);
            return true;
        }
        errorMsg = formatError("Invalid boolean literal");
        return false;
    }

    bool parseNull(JsonValue& val, std::string& errorMsg) {
        if (src.compare(pos, 4, "null") == 0) {
            pos += 4;
            col += 4;
            val = JsonValue(nullptr);
            return true;
        }
        errorMsg = formatError("Invalid null literal");
        return false;
    }

    bool parseNumber(JsonValue& val, std::string& errorMsg) {
        size_t start = pos;
        if (peek() == '-') {
            advance();
        }
        if (peek() == '0') {
            advance();
        } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        } else {
            errorMsg = formatError("Malformed number format");
            return false;
        }

        if (peek() == '.') {
            advance();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                errorMsg = formatError("Expected digit after decimal point");
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        if (peek() == 'e' || peek() == 'E') {
            advance();
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                errorMsg = formatError("Expected digit in exponent");
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        std::string numStr = src.substr(start, pos - start);
        char* endPtr = nullptr;
        double d = std::strtod(numStr.c_str(), &endPtr);
        val = JsonValue(d);
        return true;
    }
};

} // namespace

JsonValue JsonValue::parse(const std::string& text, std::string& errorMsg) {
    Parser parser(text);
    JsonValue out;
    if (parser.parse(out, errorMsg)) {
        return out;
    }
    return JsonValue();
}

static bool tryOpenStream(const std::string& path, std::ifstream& file) {
    file.open(path, std::ios::in | std::ios::binary);
    return file.is_open();
}

JsonValue JsonValue::parseFile(const std::string& filePath, std::string& errorMsg) {
    std::ifstream file;
    if (!tryOpenStream(filePath, file)) {
        // If relative path, try parent directory fallback (for build/ or bin/ execution)
        bool isAbsolute = (filePath.size() >= 2 && filePath[1] == ':') ||
                          (filePath.size() >= 1 && (filePath[0] == '/' || filePath[0] == '\\'));
        bool found = false;
        if (!isAbsolute) {
            std::string candidate1 = "../" + filePath;
            if (tryOpenStream(candidate1, file)) {
                found = true;
            } else {
                std::string candidate2 = "D:/Robot/" + filePath;
                if (tryOpenStream(candidate2, file)) {
                    found = true;
                }
            }
        }
        if (!found) {
            errorMsg = "File not found or cannot be opened: " + filePath;
            return JsonValue();
        }
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse(ss.str(), errorMsg);
}
