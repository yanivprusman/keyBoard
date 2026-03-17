#include "HidrawManager.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

std::vector<HidrawDeviceInfo> HidrawManager::enumerate() {
    std::vector<HidrawDeviceInfo> devices;

    DIR* dir = opendir("/sys/class/hidraw");
    if (!dir) return devices;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;

        HidrawDeviceInfo info;
        info.path = std::string("/dev/") + ent->d_name;

        // Read uevent for VID/PID
        char uepath[256];
        snprintf(uepath, sizeof(uepath), "/sys/class/hidraw/%s/device/uevent", ent->d_name);
        FILE* f = fopen(uepath, "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                // HID_ID=0003:00001B1C:00001BC5
                if (strncmp(line, "HID_ID=", 7) == 0) {
                    unsigned int bus, v, p;
                    if (sscanf(line + 7, "%x:%x:%x", &bus, &v, &p) == 3) {
                        info.vid = v;
                        info.pid = p;
                    }
                }
                if (strncmp(line, "HID_NAME=", 9) == 0) {
                    info.name = line + 9;
                    // Strip trailing newline
                    while (!info.name.empty() && (info.name.back() == '\n' || info.name.back() == '\r'))
                        info.name.pop_back();
                }
            }
            fclose(f);
        }

        // Read interface number
        char ifpath[256];
        snprintf(ifpath, sizeof(ifpath), "/sys/class/hidraw/%s/device/../bInterfaceNumber", ent->d_name);
        f = fopen(ifpath, "r");
        if (f) {
            fscanf(f, "%d", &info.interfaceNum);
            fclose(f);
        }

        devices.push_back(info);
    }
    closedir(dir);
    return devices;
}

int HidrawManager::findAndOpen(int vid, int pid, int interfaceNum) {
    DIR* dir = opendir("/sys/class/hidraw");
    if (!dir) return -1;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;

        // Check VID/PID
        char uepath[256];
        snprintf(uepath, sizeof(uepath), "/sys/class/hidraw/%s/device/uevent", ent->d_name);
        FILE* f = fopen(uepath, "r");
        if (!f) continue;

        bool match = false;
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "HID_ID=", 7) == 0) {
                unsigned int bus, v, p;
                if (sscanf(line + 7, "%x:%x:%x", &bus, &v, &p) == 3) {
                    if ((int)v == vid && (int)p == pid)
                        match = true;
                }
            }
        }
        fclose(f);
        if (!match) continue;

        // Check interface number
        char ifpath[256];
        snprintf(ifpath, sizeof(ifpath), "/sys/class/hidraw/%s/device/../bInterfaceNumber", ent->d_name);
        f = fopen(ifpath, "r");
        if (!f) continue;
        int iface = -1;
        fscanf(f, "%d", &iface);
        fclose(f);

        if (iface != interfaceNum) continue;

        // Found it — open
        char devpath[128];
        snprintf(devpath, sizeof(devpath), "/dev/%s", ent->d_name);
        closedir(dir);

        int fd = open(devpath, O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            fprintf(stderr, "[hidraw] Cannot open %s: %s\n", devpath, strerror(errno));
            return -1;
        }
        fprintf(stderr, "[hidraw] Opened %s (VID=%04x PID=%04x iface=%d)\n", devpath, vid, pid, interfaceNum);
        return fd;
    }

    closedir(dir);
    return -1;
}
