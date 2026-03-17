#pragma once
#include <string>
#include <vector>
#include <map>
#include <linux/input.h>

struct InputDeviceInfo {
    std::string path;       // /dev/input/eventN
    std::string name;
    std::string vid;        // hex string e.g. "1b1c"
    std::string pid;        // hex string e.g. "1bc5"
    std::string phys;
    bool hasKeys = false;
    bool hasRelAxes = false; // mouse
    bool isKeyboard = false;
    bool isMouse = false;
};

struct GrabbedDevice {
    int evdevFd = -1;
    int uinputFd = -1;
    std::string path;
    std::string name;
    std::string type; // "generic" or "corsair-k100"
    bool numLockOn = true; // track NumLock state for dual mode
};

class DeviceManager {
public:
    // Enumerate all input devices from /dev/input/event*
    std::vector<InputDeviceInfo> enumerate();

    // Grab an evdev device, returns fd or -1
    int grabDevice(const std::string& path);

    // Ungrab a device
    void ungrabDevice(int fd);

    // Check if a device matches VID:PID
    static bool matchesVidPid(const std::string& path, const std::string& vid, const std::string& pid);

    // Find evdev device by VID:PID that has KEY_A (keyboard)
    std::string findKeyboardByVidPid(int vid, int pid);
};
