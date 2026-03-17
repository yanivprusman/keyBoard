#include "BragiK100.h"
#include "HidrawManager.h"
#include "DeviceManager.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/hidraw.h>

// BRAGI NKRO index → Linux keycode mapping
const int BragiK100::bragiToLinux[200] = {
    // 0-3: unused
    -1, -1, -1, -1,
    // 4-29: a-z
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
    KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
    KEY_Y, KEY_Z,
    // 30-39: 1-0
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
    // 40-56: enter through slash
    KEY_ENTER, KEY_ESC, KEY_BACKSPACE, KEY_TAB, KEY_SPACE,
    KEY_MINUS, KEY_EQUAL, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_BACKSLASH,
    KEY_BACKSLASH, // hash (ISO)
    KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_COMMA, KEY_DOT, KEY_SLASH,
    // 57: caps
    KEY_CAPSLOCK,
    // 58-69: F1-F12
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    // 70-82: prtscn through up
    KEY_SYSRQ, KEY_SCROLLLOCK, KEY_PAUSE,
    KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_DELETE, KEY_END, KEY_PAGEDOWN,
    KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP,
    // 83-88: numlock through numenter
    KEY_NUMLOCK, KEY_KPSLASH, KEY_KPASTERISK, KEY_KPMINUS, KEY_KPPLUS, KEY_KPENTER,
    // 89-99: num1-num0, numdot
    KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5,
    KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP0, KEY_KPDOT,
    // 100: bslash_iso
    KEY_102ND,
    // 101: rmenu/compose
    KEY_COMPOSE,
    // 102-104: mute, volup, voldn
    KEY_MUTE, KEY_VOLUMEUP, KEY_VOLUMEDOWN,
    // 105-112: modifiers
    KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_LEFTALT, KEY_LEFTMETA,
    KEY_RIGHTCTRL, KEY_RIGHTSHIFT, KEY_RIGHTALT, KEY_RIGHTMETA,
    // 113-114: light/lock (corsair special, skip)
    -1, -1,
    // 115: ro (Japanese)
    KEY_RO,
    // 116-121: unused
    -1, -1, -1, -1, -1, -1,
    // 122: fn
    KEY_FN,
    // 123-126: media
    KEY_STOPCD, KEY_PLAYPAUSE, KEY_NEXTSONG, KEY_PREVIOUSSONG,
    // 127-128: mr/profswitch (corsair special)
    -1, -1,
    // 129-130: unused
    -1, -1,
    // 131-136: G1-G6 → F13-F18
    KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,
    // 137+: control wheel, light bars (skip)
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1,
};

BragiK100::~BragiK100() {
    shutdown();
}

bool BragiK100::bragiSend(int fd, const uint8_t* pkt, size_t len) {
    uint8_t buf[1024] = {};
    memcpy(buf, pkt, len < 1024 ? len : 1024);
    return write(fd, buf, 1024) == 1024;
}

int BragiK100::bragiRecv(int fd, uint8_t* buf, int timeoutMs) {
    struct pollfd pfd = {fd, POLLIN, 0};
    int ret = poll(&pfd, 1, timeoutMs);
    if (ret <= 0) return ret;
    return read(fd, buf, 1024);
}

bool BragiK100::bragiSetMode(uint8_t mode) {
    uint8_t pkt[5] = {BRAGI_MAGIC, BRAGI_CMD_SET, BRAGI_PROP_MODE, 0x00, mode};
    if (!bragiSend(m_ctrlFd, pkt, sizeof(pkt))) return false;

    uint8_t resp[1024];
    for (int i = 0; i < 10; i++) {
        int n = bragiRecv(m_ctrlFd, resp, 200);
        if (n > 2 && resp[1] == BRAGI_CMD_SET)
            return resp[2] == 0x00;
    }
    return false;
}

int BragiK100::bragiGetMode() {
    uint8_t pkt[3] = {BRAGI_MAGIC, BRAGI_CMD_GET, BRAGI_PROP_MODE};
    if (!bragiSend(m_ctrlFd, pkt, sizeof(pkt))) return -1;

    uint8_t resp[1024];
    for (int i = 0; i < 10; i++) {
        int n = bragiRecv(m_ctrlFd, resp, 200);
        if (n > 3 && resp[1] == BRAGI_CMD_GET && resp[2] == 0x00)
            return resp[3];
    }
    return -1;
}

bool BragiK100::init() {
    HidrawManager hidraw;

    // Open BRAGI control interface (interface 1)
    m_ctrlFd = hidraw.findAndOpen(CORSAIR_VID, CORSAIR_K100_PID, 1);
    if (m_ctrlFd < 0) {
        fprintf(stderr, "[bragi] Cannot find K100 BRAGI control (interface 1)\n");
        return false;
    }

    // Open NKRO interface (interface 2)
    m_nkroFd = hidraw.findAndOpen(CORSAIR_VID, CORSAIR_K100_PID, 2);
    if (m_nkroFd < 0) {
        fprintf(stderr, "[bragi] Cannot find K100 NKRO input (interface 2)\n");
        close(m_ctrlFd);
        m_ctrlFd = -1;
        return false;
    }

    // Switch to software mode
    fprintf(stderr, "[bragi] Switching to software mode...\n");
    if (!bragiSetMode(BRAGI_MODE_SW)) {
        fprintf(stderr, "[bragi] Failed to set software mode\n");
        close(m_nkroFd); m_nkroFd = -1;
        close(m_ctrlFd); m_ctrlFd = -1;
        return false;
    }

    int mode = bragiGetMode();
    if (mode != BRAGI_MODE_SW) {
        fprintf(stderr, "[bragi] Mode verify failed (got %d, expected %d)\n", mode, BRAGI_MODE_SW);
        close(m_nkroFd); m_nkroFd = -1;
        close(m_ctrlFd); m_ctrlFd = -1;
        return false;
    }
    fprintf(stderr, "[bragi] Software mode active\n");

    // Find and grab evdev to suppress phantom events from Interface 0
    DeviceManager devMgr;
    std::string evdevPath = devMgr.findKeyboardByVidPid(CORSAIR_VID, CORSAIR_K100_PID);
    if (!evdevPath.empty()) {
        m_evdevFd = devMgr.grabDevice(evdevPath);
        if (m_evdevFd >= 0)
            fprintf(stderr, "[bragi] Grabbed evdev %s (Interface 0 suppressed)\n", evdevPath.c_str());
        else
            fprintf(stderr, "[bragi] Warning: could not grab evdev\n");
    }

    // Create uinput virtual device
    if (!m_emitter.create("K100 BRAGI Keyboard", CORSAIR_VID, 0xFFFF)) {
        fprintf(stderr, "[bragi] Failed to create uinput device\n");
        shutdown();
        return false;
    }

    memset(m_keyState, 0, sizeof(m_keyState));
    m_initialized = true;
    m_grabbed = true;
    fprintf(stderr, "[bragi] Initialized — all keys from NKRO, G1-G6 → F13-F18\n");
    return true;
}

void BragiK100::shutdown() {
    if (m_initialized) {
        releaseAllKeys();
        m_initialized = false;
        m_grabbed = false;
    }

    m_emitter.destroy();

    if (m_evdevFd >= 0) {
        ioctl(m_evdevFd, EVIOCGRAB, 0);
        close(m_evdevFd);
        m_evdevFd = -1;
    }

    if (m_ctrlFd >= 0) {
        fprintf(stderr, "[bragi] Restoring hardware mode...\n");
        bragiSetMode(BRAGI_MODE_HW);
        close(m_ctrlFd);
        m_ctrlFd = -1;
    }

    if (m_nkroFd >= 0) {
        close(m_nkroFd);
        m_nkroFd = -1;
    }
}

void BragiK100::ungrab() {
    if (!m_grabbed) return;

    releaseAllKeys();
    m_emitter.destroy();

    if (m_evdevFd >= 0) {
        ioctl(m_evdevFd, EVIOCGRAB, 0);
        close(m_evdevFd);
        m_evdevFd = -1;
    }

    m_grabbed = false;
    fprintf(stderr, "[bragi] Ungrabbed (staying in SW mode)\n");
}

bool BragiK100::regrab() {
    if (!m_initialized) return false;
    if (m_grabbed) return true;

    // Re-grab evdev to suppress Interface 0 phantom events
    DeviceManager devMgr;
    std::string evdevPath = devMgr.findKeyboardByVidPid(CORSAIR_VID, CORSAIR_K100_PID);
    if (!evdevPath.empty()) {
        m_evdevFd = devMgr.grabDevice(evdevPath);
        if (m_evdevFd >= 0)
            fprintf(stderr, "[bragi] Re-grabbed evdev %s\n", evdevPath.c_str());
        else
            fprintf(stderr, "[bragi] Warning: could not re-grab evdev\n");
    }

    // Re-create uinput virtual device
    if (!m_emitter.create("K100 BRAGI Keyboard", CORSAIR_VID, 0xFFFF)) {
        fprintf(stderr, "[bragi] Failed to re-create uinput device\n");
        return false;
    }

    memset(m_keyState, 0, sizeof(m_keyState));
    m_grabbed = true;
    fprintf(stderr, "[bragi] Re-grabbed (SW mode preserved)\n");
    return true;
}

bool BragiK100::processNkroPacket() {
    uint8_t buf[64];
    int n = read(m_nkroFd, buf, sizeof(buf));
    if (n < 20 || buf[1] != BRAGI_NKRO_MARKER)
        return false;

    bool changed = false;
    for (int idx = 0; idx < 200 && idx < NKRO_BITS; idx++) {
        int linuxKey = bragiToLinux[idx];
        if (linuxKey < 0) continue;

        // Apply G-key overrides
        if (idx >= GKEY_BASE && idx < GKEY_END) {
            int gIdx = idx - GKEY_BASE;
            if (m_gkeyOverrides[gIdx] >= 0)
                linuxKey = m_gkeyOverrides[gIdx];
        }

        int byteOff = 2 + (idx / 8);
        int bitOff = idx % 8;
        bool pressed = (byteOff < n) && ((buf[byteOff] >> bitOff) & 1);

        if (pressed != m_keyState[idx]) {
            m_keyState[idx] = pressed;
            m_emitter.emitKey(linuxKey, pressed ? 1 : 0);
            changed = true;

            if (idx >= GKEY_BASE && idx < GKEY_END)
                fprintf(stderr, "[bragi] G%d %s (keycode %d)\n",
                        idx - GKEY_BASE + 1, pressed ? "pressed" : "released", linuxKey);
        }
    }

    if (changed)
        m_emitter.emitSyn();
    return changed;
}

void BragiK100::releaseAllKeys() {
    bool any = false;
    for (int idx = 0; idx < 200; idx++) {
        if (m_keyState[idx]) {
            int linuxKey = bragiToLinux[idx];
            if (idx >= GKEY_BASE && idx < GKEY_END) {
                int gIdx = idx - GKEY_BASE;
                if (m_gkeyOverrides[gIdx] >= 0)
                    linuxKey = m_gkeyOverrides[gIdx];
            }
            if (linuxKey >= 0) {
                m_emitter.emitKey(linuxKey, 0);
                any = true;
            }
            m_keyState[idx] = false;
        }
    }
    if (any)
        m_emitter.emitSyn();
}

void BragiK100::setGkeyMapping(int gkeyIndex, int linuxKeycode) {
    if (gkeyIndex >= 0 && gkeyIndex < 6)
        m_gkeyOverrides[gkeyIndex] = linuxKeycode;
}
