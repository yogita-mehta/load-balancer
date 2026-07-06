#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

// Minimal, dependency-free JSON reader supporting exactly what our flat
// config file needs: string/number/bool scalars and arrays of numbers.
// This avoids pulling in an external JSON library so the project builds
// with nothing but a standard C++17 toolchain.
class JsonConfig {
public:
    static JsonConfig loadFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in.good()) {
            throw std::runtime_error("Could not open config file: " + path);
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        JsonConfig cfg;
        cfg.parse(buffer.str());
        return cfg;
    }

    double getNumber(const std::string& key, double fallback) const {
        auto it = values_.find(key);
        return it != values_.end() ? std::stod(it->second) : fallback;
    }

    int getInt(const std::string& key, int fallback) const {
        return static_cast<int>(getNumber(key, fallback));
    }

    std::string getString(const std::string& key, const std::string& fallback) const {
        auto it = values_.find(key);
        return it != values_.end() ? it->second : fallback;
    }

    bool getBool(const std::string& key, bool fallback) const {
        auto it = values_.find(key);
        if (it == values_.end()) return fallback;
        return it->second == "true";
    }

    std::vector<int> getIntArray(const std::string& key) const {
        std::vector<int> result;
        auto it = arrays_.find(key);
        if (it == arrays_.end()) return result;
        for (const auto& tok : it->second) result.push_back(std::stoi(tok));
        return result;
    }

private:
    std::unordered_map<std::string, std::string> values_;
    std::unordered_map<std::string, std::vector<std::string>> arrays_;

    // Extremely small recursive-descent-ish scanner: sufficient for a flat
    // (non-nested-object) JSON document with string/number/bool/array values.
    void parse(const std::string& text) {
        size_t i = 0;
        skipToObjectStart(text, i);
        while (i < text.size()) {
            skipWhitespaceAndCommas(text, i);
            if (i >= text.size() || text[i] == '}') break;
            std::string key = parseKey(text, i);
            skipWhitespace(text, i);
            if (text[i] != ':') throw std::runtime_error("Expected ':' in config near key " + key);
            ++i;
            skipWhitespace(text, i);
            if (text[i] == '[') {
                arrays_[key] = parseArray(text, i);
            } else if (text[i] == '"') {
                values_[key] = parseString(text, i);
            } else {
                values_[key] = parseLiteral(text, i);
            }
        }
    }

    static void skipToObjectStart(const std::string& text, size_t& i) {
        while (i < text.size() && text[i] != '{') ++i;
        if (i < text.size()) ++i; // consume '{'
    }

    static void skipWhitespace(const std::string& text, size_t& i) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    }

    static void skipWhitespaceAndCommas(const std::string& text, size_t& i) {
        while (i < text.size() &&
               (std::isspace(static_cast<unsigned char>(text[i])) || text[i] == ',')) ++i;
    }

    static std::string parseKey(const std::string& text, size_t& i) {
        skipWhitespace(text, i);
        return parseString(text, i);
    }

    static std::string parseString(const std::string& text, size_t& i) {
        if (text[i] != '"') throw std::runtime_error("Expected string in config");
        ++i;
        std::string result;
        while (i < text.size() && text[i] != '"') {
            result += text[i];
            ++i;
        }
        ++i; // consume closing quote
        return result;
    }

    static std::string parseLiteral(const std::string& text, size_t& i) {
        std::string result;
        while (i < text.size() && text[i] != ',' && text[i] != '}' && text[i] != '\n' &&
               text[i] != ']') {
            if (!std::isspace(static_cast<unsigned char>(text[i]))) result += text[i];
            ++i;
        }
        return result;
    }

    static std::vector<std::string> parseArray(const std::string& text, size_t& i) {
        std::vector<std::string> result;
        ++i; // consume '['
        while (i < text.size() && text[i] != ']') {
            skipWhitespaceAndCommas(text, i);
            if (text[i] == ']') break;
            std::string tok;
            while (i < text.size() && text[i] != ',' && text[i] != ']') {
                if (!std::isspace(static_cast<unsigned char>(text[i]))) tok += text[i];
                ++i;
            }
            if (!tok.empty()) result.push_back(tok);
        }
        if (i < text.size()) ++i; // consume ']'
        return result;
    }
};
