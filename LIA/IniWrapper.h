#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

class IniWrapper {
public:
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) return false;

        std::string line, section;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';') continue;
            if (line[0] == '[') {
                section = line.substr(1, line.find(']') - 1);
            }
            else {
                auto pos = line.find('=');
                if (pos == std::string::npos) continue;
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                data[section + "." + key] = value;
            }
        }
        return true;
    }

    // 型変換付き取得（int, double など）
    template <typename T>
    T get(std::string_view section, std::string_view key, T def) const {
        static_assert(std::is_arithmetic_v<T>, "Only arithmetic types are supported");

        auto it = data.find(std::string(section) + "." + std::string(key));
        if (it == data.end()) return def;

        std::istringstream iss(it->second);
        T value;
        return (iss >> value) ? value : def;
    }

    template <typename T>
    void set(std::string_view section, std::string_view key, const T& value) {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, bool>) {
            oss << (value ? "true" : "false");
        }
        else {
            oss << value;
        }
        data[std::string(section) + "." + std::string(key)] = oss.str();
    }

    bool save(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) return false;

        // ソート可能な vector に変換
        std::vector<std::pair<std::string, std::string>> sorted_data(data.begin(), data.end());

        // キー順にソート
        std::sort(sorted_data.begin(), sorted_data.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

        std::string last_section;
        for (const auto& [full_key, value] : sorted_data) {
            auto dot = full_key.find('.');
            std::string section = full_key.substr(0, dot);
            std::string key = full_key.substr(dot + 1);
            if (section != last_section) {
                if (!last_section.empty()) file << "\n";
                file << "[" << section << "]\n";
                last_section = section;
            }
            file << key << "=" << value << "\n";
        }
        return true;
    }
private:
    std::unordered_map<std::string, std::string> data;
};
