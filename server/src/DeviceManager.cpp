#include "DeviceManager.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

std::vector<InputDeviceInfo> DeviceManager::enumerate() {
    std::vector<InputDeviceInfo> devices;

    for (int i = 0; i < 64; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        InputDeviceInfo info;
        info.path = path;

        // Get device name
        char name[256] = {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0)
            info.name = name;

        // Get device ID
        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) >= 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%04x", id.vendor);
            info.vid = buf;
            snprintf(buf, sizeof(buf), "%04x", id.product);
            info.pid = buf;
        }

        // Get phys
        char phys[256] = {};
        if (ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) >= 0)
            info.phys = phys;

        // Check capabilities (use uint8_t for simple byte-level bit ops)
        uint8_t evbits[(EV_MAX + 7) / 8] = {};
        ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);

        // Has EV_KEY?
        if (evbits[EV_KEY / 8] & (1 << (EV_KEY % 8))) {
            info.hasKeys = true;

            uint8_t keybits[(KEY_MAX + 7) / 8] = {};
            ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);

            // Has KEY_A? → keyboard
            if (keybits[KEY_A / 8] & (1 << (KEY_A % 8)))
                info.isKeyboard = true;

            // Has BTN_LEFT? → mouse
            if (keybits[BTN_LEFT / 8] & (1 << (BTN_LEFT % 8)))
                info.isMouse = true;
        }

        // Has EV_REL? → mouse axes
        if (evbits[EV_REL / 8] & (1 << (EV_REL % 8)))
            info.hasRelAxes = true;

        close(fd);
        devices.push_back(info);
    }

    return devices;
}

int DeviceManager::grabDevice(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "[device] Cannot open %s: %s\n", path.c_str(), strerror(errno));
        return -1;
    }

    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "[device] Cannot grab %s: %s\n", path.c_str(), strerror(errno));
        close(fd);
        return -1;
    }

    char name[256] = {};
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    fprintf(stderr, "[device] Grabbed %s (%s)\n", path.c_str(), name);
    return fd;
}

void DeviceManager::ungrabDevice(int fd) {
    if (fd >= 0) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
}

bool DeviceManager::matchesVidPid(const std::string& path, const std::string& vid, const std::string& pid) {
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;

    struct input_id id;
    bool match = false;
    if (ioctl(fd, EVIOCGID, &id) >= 0) {
        char vbuf[8], pbuf[8];
        snprintf(vbuf, sizeof(vbuf), "%04x", id.vendor);
        snprintf(pbuf, sizeof(pbuf), "%04x", id.product);
        match = (vid == vbuf && pid == pbuf);
    }
    close(fd);
    return match;
}

std::string DeviceManager::findKeyboardByVidPid(int vid, int pid) {
    for (int i = 0; i < 64; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) < 0) { close(fd); continue; }
        if (id.vendor != vid || id.product != pid) { close(fd); continue; }

        // Must have KEY_A (keyboard, not mouse/media)
        uint8_t keybits[(KEY_MAX + 7) / 8] = {};
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
        bool hasKeyA = keybits[KEY_A / 8] & (1 << (KEY_A % 8));

        close(fd);
        if (hasKeyA) return path;
    }
    return "";
}
