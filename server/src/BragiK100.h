#pragma once
#include "UInputEmitter.h"
#include <cstdint>
#include <functional>

class BragiK100 {
public:
    // BRAGI protocol constants
    static constexpr uint8_t BRAGI_MAGIC = 0x08;
    static constexpr uint8_t BRAGI_CMD_SET = 0x01;
    static constexpr uint8_t BRAGI_CMD_GET = 0x02;
    static constexpr uint8_t BRAGI_PROP_MODE = 0x03;
    static constexpr uint8_t BRAGI_MODE_HW = 0x01;
    static constexpr uint8_t BRAGI_MODE_SW = 0x02;
    static constexpr uint8_t BRAGI_NKRO_MARKER = 0x02;
    static constexpr uint8_t BRAGI_CMD_OPEN_HANDLE = 0x0D;
    static constexpr uint8_t BRAGI_CMD_CLOSE_HANDLE = 0x05;
    static constexpr uint8_t BRAGI_CMD_WRITE_DATA = 0x06;
    static constexpr uint8_t BRAGI_CMD_CONTINUE_WRITE = 0x07;
    static constexpr uint16_t BRAGI_RES_ALT_LIGHTING = 0x0022;
    static constexpr uint16_t BRAGI_RES_LIGHTING_EXTRA = 0x002E;
    static constexpr int NUM_LEDS = 193;

    static constexpr int CORSAIR_VID = 0x1b1c;
    static constexpr int CORSAIR_K100_PID = 0x1bc5;

    static constexpr int NKRO_BYTES = 62;
    static constexpr int NKRO_BITS = NKRO_BYTES * 8;
    static constexpr int GKEY_BASE = 131;
    static constexpr int GKEY_END = 137;

    ~BragiK100();

    // Initialize: open hidraw interfaces, switch to SW mode, grab evdev, create uinput
    bool init();

    // Shutdown: release grabs, restore HW mode, destroy uinput (full teardown)
    void shutdown();

    // Soft ungrab: release keys, destroy uinput, ungrab evdev — stays in SW mode
    void ungrab();

    // Soft regrab: re-grab evdev, re-create uinput — assumes already in SW mode
    bool regrab();

    // Process a packet from the NKRO hidraw fd
    // Returns true if any keys changed
    bool processNkroPacket();

    // Release all held keys (for clean shutdown)
    void releaseAllKeys();

    int ctrlFd() const { return m_ctrlFd; }
    int nkroFd() const { return m_nkroFd; }
    int evdevFd() const { return m_evdevFd; }
    bool isInitialized() const { return m_initialized; }
    bool isGrabbed() const { return m_grabbed; }

    UInputEmitter& emitter() { return m_emitter; }

    // Custom G-key mapping (bragi index → linux keycode override)
    void setGkeyMapping(int gkeyIndex, int linuxKeycode);

    // LED control
    bool setAllColor(uint8_t r, uint8_t g, uint8_t b);
    bool setKeyColor(int led, uint8_t r, uint8_t g, uint8_t b);
    bool flushColors();

    // BRAGI index to linux keycode map
    static const int bragiToLinux[200];

private:
    bool bragiSend(int fd, const uint8_t* pkt, size_t len);
    int bragiRecv(int fd, uint8_t* buf, int timeoutMs);
    bool bragiSetMode(uint8_t mode);
    int bragiGetMode();
    bool bragiOpenHandle(uint8_t handle, uint16_t resource);
    void bragiCloseHandle(uint8_t handle);
    bool bragiWriteToHandle(uint8_t handle, const uint8_t* data, size_t len);

    int m_ctrlFd = -1;  // hidraw interface 1 (BRAGI control)
    int m_nkroFd = -1;  // hidraw interface 2 (NKRO bitmap)
    int m_evdevFd = -1;  // evdev device (grabbed to suppress phantom events)
    bool m_initialized = false;  // BRAGI connection established (in SW mode)
    bool m_grabbed = false;      // actively processing events (uinput + evdev grabbed)

    UInputEmitter m_emitter;
    bool m_keyState[200] = {};
    int m_gkeyOverrides[6] = {-1, -1, -1, -1, -1, -1}; // per-gkey overrides (-1 = use default)
    // Interleaved RGB with 2-byte offset; buffer stays at 579 bytes (192 usable LEDs)
    static constexpr int LED_BUF_OFFSET = 2;
    static constexpr int LED_BUF_LEDS = NUM_LEDS - 1;  // 192 (LED 192 excluded)
    uint8_t m_ledBuffer[NUM_LEDS * 3] = {};
};
