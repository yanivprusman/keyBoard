#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

struct DeviceConfig {
    std::string path;
    std::string name;
    std::string vid;
    std::string pid;
    bool grab = false;
    std::string type = "generic"; // "generic" or "corsair-k100"
};

class ConfigManager {
public:
    bool load(const std::string& path);
    bool save();
    bool save(const std::string& path);

    const std::string& configPath() const { return m_path; }
    std::vector<DeviceConfig>& devices() { return m_devices; }
    const std::vector<DeviceConfig>& devices() const { return m_devices; }
    std::map<std::string, std::string>& gkeyMap() { return m_gkeyMap; }
    std::map<std::string, std::string>& numpadCustomMode() { return m_numpadCustomMode; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    std::string m_path;
    std::vector<DeviceConfig> m_devices;
    std::map<std::string, std::string> m_gkeyMap;
    std::map<std::string, std::string> m_numpadCustomMode;
};
