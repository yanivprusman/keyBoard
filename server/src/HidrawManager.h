#pragma once
#include <string>
#include <vector>

struct HidrawDeviceInfo {
    std::string path;       // /dev/hidrawN
    std::string name;
    int vid = 0;
    int pid = 0;
    int interfaceNum = -1;
};

class HidrawManager {
public:
    // Enumerate all hidraw devices from /sys/class/hidraw
    std::vector<HidrawDeviceInfo> enumerate();

    // Find hidraw device by VID:PID and interface number
    // Returns fd (O_RDWR | O_NONBLOCK) or -1
    int findAndOpen(int vid, int pid, int interfaceNum);
};
