#include "ConfigManager.h"
#include <fstream>
#include <cstdio>

using json = nlohmann::json;

bool ConfigManager::load(const std::string& path) {
    m_path = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[config] Cannot open %s\n", path.c_str());
        return false;
    }

    try {
        json j = json::parse(f);
        fromJson(j);
        fprintf(stderr, "[config] Loaded %zu devices from %s\n", m_devices.size(), path.c_str());
        return true;
    } catch (const std::exception& e) {
        fprintf(stderr, "[config] Parse error: %s\n", e.what());
        return false;
    }
}

bool ConfigManager::save() {
    return save(m_path);
}

bool ConfigManager::save(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[config] Cannot write %s\n", path.c_str());
        return false;
    }
    f << toJson().dump(2) << std::endl;
    return true;
}

nlohmann::json ConfigManager::toJson() const {
    json j;
    json devArr = json::array();
    for (const auto& d : m_devices) {
        devArr.push_back({
            {"path", d.path},
            {"name", d.name},
            {"vid", d.vid},
            {"pid", d.pid},
            {"grab", d.grab},
            {"type", d.type}
        });
    }
    j["devices"] = devArr;
    j["gkeyMap"] = m_gkeyMap;
    j["numpadCustomMode"] = m_numpadCustomMode;
    return j;
}

void ConfigManager::fromJson(const nlohmann::json& j) {
    m_devices.clear();
    if (j.contains("devices") && j["devices"].is_array()) {
        for (const auto& d : j["devices"]) {
            DeviceConfig dc;
            dc.path = d.value("path", "");
            dc.name = d.value("name", "");
            dc.vid = d.value("vid", "");
            dc.pid = d.value("pid", "");
            dc.grab = d.value("grab", false);
            dc.type = d.value("type", "generic");
            m_devices.push_back(dc);
        }
    }

    m_gkeyMap.clear();
    if (j.contains("gkeyMap") && j["gkeyMap"].is_object()) {
        for (auto& [k, v] : j["gkeyMap"].items()) {
            m_gkeyMap[k] = v.get<std::string>();
        }
    }

    m_numpadCustomMode.clear();
    if (j.contains("numpadCustomMode") && j["numpadCustomMode"].is_object()) {
        for (auto& [k, v] : j["numpadCustomMode"].items()) {
            m_numpadCustomMode[k] = v.get<std::string>();
        }
    }
}
