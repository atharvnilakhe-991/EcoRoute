#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
private:
    JsonType type_{JsonType::Null};
    bool boolVal_{false};
    double numVal_{0.0};
    std::string strVal_;
    std::vector<JsonValue> arrVal_;
    std::map<std::string, JsonValue> objVal_;

public:
    JsonValue() : type_(JsonType::Null) {}
    JsonValue(std::nullptr_t) : type_(JsonType::Null) {}
    JsonValue(bool b) : type_(JsonType::Bool), boolVal_(b) {}
    JsonValue(int n) : type_(JsonType::Number), numVal_(n) {}
    JsonValue(double d) : type_(JsonType::Number), numVal_(d) {}
    JsonValue(const char* s) : type_(JsonType::String), strVal_(s ? s : "") {}
    JsonValue(const std::string& s) : type_(JsonType::String), strVal_(s) {}
    JsonValue(std::vector<JsonValue> arr) : type_(JsonType::Array), arrVal_(std::move(arr)) {}
    JsonValue(std::map<std::string, JsonValue> obj) : type_(JsonType::Object), objVal_(std::move(obj)) {}

    JsonType getType() const { return type_; }
    bool isNull() const { return type_ == JsonType::Null; }
    bool isBool() const { return type_ == JsonType::Bool; }
    bool isNumber() const { return type_ == JsonType::Number; }
    bool isString() const { return type_ == JsonType::String; }
    bool isArray() const { return type_ == JsonType::Array; }
    bool isObject() const { return type_ == JsonType::Object; }

    bool asBool(bool def = false) const { return isBool() ? boolVal_ : def; }
    double asDouble(double def = 0.0) const { return isNumber() ? numVal_ : def; }
    int asInt(int def = 0) const { return isNumber() ? static_cast<int>(numVal_) : def; }
    const std::string& asString(const std::string& def = "") const { return isString() ? strVal_ : def; }
    const std::vector<JsonValue>& asArray() const { return arrVal_; }
    const std::map<std::string, JsonValue>& asObject() const { return objVal_; }

    bool hasKey(const std::string& key) const {
        if (!isObject()) return false;
        return objVal_.find(key) != objVal_.end();
    }

    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t idx) const;

    std::string serialize(int indent = 0) const;

    static JsonValue parse(const std::string& text, std::string& errorMsg);
    static JsonValue parseFile(const std::string& filePath, std::string& errorMsg);
};

#endif // JSON_PARSER_H
