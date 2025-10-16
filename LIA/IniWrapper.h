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

    std::string get(const std::string& section, const std::string& key, const std::string& def = "") const {
        auto it = data.find(section + "." + key);
        return it != data.end() ? it->second : def;
    }

    void set(const std::string& section, const std::string& key, const std::string& value) {
        data[section + "." + key] = value;
    }

    bool toBool(const std::string& value) {
        return value == "true" || value == "1";
    }
    bool save(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) return false;

        std::string last_section;
        for (const auto& [full_key, value] : data) {
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


/*
Ini ini;
ini.load("config.ini");

int x = std::stoi(ini.get("Window", "PosX", "100"));
int y = std::stoi(ini.get("Window", "PosY", "100"));
int w = std::stoi(ini.get("Window", "Width", "800"));
int h = std::stoi(ini.get("Window", "Height", "600"));
std::string value = ini.get("Window", "Fullscreen", "false");
bool isFullscreen = (value == "true"); // •¶Žš—ñ”äŠr

ini.set("Window", "PosX", std::to_string(x));
ini.set("Window", "PosY", std::to_string(y));
ini.set("Window", "Width", std::to_string(w));
ini.set("Window", "Height", std::to_string(h));

bool isFullscreen = true;
ini.set("Window", "Fullscreen", isFullscreen ? "true" : "false");
ini.save("config.ini");


*/