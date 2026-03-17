#include "ConfigManager.h"
#include "DeviceManager.h"
#include "HidrawManager.h"
#include "UInputEmitter.h"
#include "BragiK100.h"
#include "ApiServer.h"

#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/input.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static constexpr const char* DEFAULT_SOCKET = "/run/keyboard-server.sock";
static constexpr const char* DEFAULT_CONFIG = "config/keyboard.json";
static constexpr int MAX_EVENTS = 32;

static volatile bool g_running = true;
static void sighandler(int) { g_running = false; }

// ── NumLock custom mode keymap ──────────────────────────────────

struct NumpadRemap {
    int from;  // e.g. KEY_KP1
    int to;    // e.g. KEY_1
};

static const NumpadRemap NUMPAD_REMAPS[] = {
    {KEY_KP0, KEY_0},
    {KEY_KP1, KEY_1},
    {KEY_KP2, KEY_2},
    {KEY_KP3, KEY_3},
    {KEY_KP4, KEY_4},
    {KEY_KP5, KEY_5},
    {KEY_KP6, KEY_6},
    {KEY_KP7, KEY_7},
    {KEY_KP8, KEY_8},
    {KEY_KP9, KEY_9},
    {KEY_KPDOT, KEY_DOT},
    {KEY_KPENTER, KEY_ENTER},
    {KEY_KPPLUS, KEY_EQUAL},     // + → =
    {KEY_KPMINUS, KEY_MINUS},
    {KEY_KPASTERISK, KEY_8},     // * → 8 (Shift+8 in most layouts)
    {KEY_KPSLASH, KEY_SLASH},
};
static constexpr int NUM_REMAPS = sizeof(NUMPAD_REMAPS) / sizeof(NUMPAD_REMAPS[0]);

static int remapNumpad(int code) {
    for (int i = 0; i < NUM_REMAPS; i++) {
        if (NUMPAD_REMAPS[i].from == code)
            return NUMPAD_REMAPS[i].to;
    }
    return code; // no remap
}

// ── Global state ────────────────────────────────────────────────

static ConfigManager g_config;
static DeviceManager g_devMgr;
static ApiServer g_api;
static BragiK100 g_bragi;
static int g_epfd = -1;

struct GenericDevice {
    int evdevFd = -1;
    UInputEmitter emitter;
    std::string path;
    std::string name;
    bool numLockOn = true;
};

static std::vector<GenericDevice> g_genericDevices;

// ── Helpers ─────────────────────────────────────────────────────

static int keyNameToCode(const std::string& name) {
    if (name == "KEY_F13") return KEY_F13;
    if (name == "KEY_F14") return KEY_F14;
    if (name == "KEY_F15") return KEY_F15;
    if (name == "KEY_F16") return KEY_F16;
    if (name == "KEY_F17") return KEY_F17;
    if (name == "KEY_F18") return KEY_F18;
    if (name == "KEY_F19") return KEY_F19;
    if (name == "KEY_F20") return KEY_F20;
    if (name == "KEY_F21") return KEY_F21;
    if (name == "KEY_F22") return KEY_F22;
    if (name == "KEY_F23") return KEY_F23;
    if (name == "KEY_F24") return KEY_F24;
    return -1;
}

static void applyGkeyMappings() {
    auto& gmap = g_config.gkeyMap();
    for (int g = 0; g < 6; g++) {
        char gname[4];
        snprintf(gname, sizeof(gname), "G%d", g + 1);
        auto it = gmap.find(gname);
        if (it != gmap.end()) {
            int code = keyNameToCode(it->second);
            if (code >= 0)
                g_bragi.setGkeyMapping(g, code);
        }
    }
}

// ── API command handlers ────────────────────────────────────────

static json handleListDevices(const json&) {
    auto devices = g_devMgr.enumerate();
    json arr = json::array();
    for (const auto& d : devices) {
        arr.push_back({
            {"path", d.path},
            {"name", d.name},
            {"vid", d.vid},
            {"pid", d.pid},
            {"isKeyboard", d.isKeyboard},
            {"isMouse", d.isMouse},
            {"phys", d.phys}
        });
    }
    return {{"devices", arr}};
}

static json handleGetStatus(const json&) {
    json grabbed = json::array();
    for (const auto& gd : g_genericDevices) {
        grabbed.push_back({
            {"path", gd.path},
            {"name", gd.name},
            {"type", "generic"},
            {"numLockOn", gd.numLockOn}
        });
    }
    if (g_bragi.isInitialized()) {
        std::string k100Path = "corsair-k100";
        for (const auto& dc : g_config.devices()) {
            if (dc.type == "corsair-k100") { k100Path = dc.path; break; }
        }
        grabbed.push_back({
            {"path", k100Path},
            {"name", "Corsair K100 RGB (BRAGI)"},
            {"type", "corsair-k100"},
            {"gkeys", true}
        });
    }

    return {
        {"running", true},
        {"grabbedDevices", grabbed},
        {"bragiInitialized", g_bragi.isInitialized()}
    };
}

static json handleGetConfig(const json&) {
    return g_config.toJson();
}

static json handleSetConfig(const json& req) {
    if (!req.contains("config")) {
        return {{"error", "Missing 'config' field"}};
    }
    g_config.fromJson(req["config"]);
    if (g_config.save()) {
        return {{"ok", true}};
    }
    return {{"error", "Failed to save config"}};
}

static json handleGrab(const json& req) {
    if (!req.contains("path")) {
        return {{"error", "Missing 'path' field"}};
    }
    std::string path = req["path"].get<std::string>();
    bool grab = req.value("grab", true);

    // Find device in config
    DeviceConfig* dcPtr = nullptr;
    for (auto& dc : g_config.devices()) {
        if (dc.path == path) { dcPtr = &dc; break; }
    }

    // Device not in config — add it first if grabbing
    if (!dcPtr && grab) {
        if (path == "corsair-k100") {
            DeviceConfig dc;
            dc.path = "corsair-k100";
            dc.name = "Corsair K100 RGB";
            dc.vid = "1b1c";
            dc.pid = "1bc5";
            dc.grab = false; // will be set below
            dc.type = "corsair-k100";
            g_config.devices().push_back(dc);
            dcPtr = &g_config.devices().back();
        } else {
            auto devices = g_devMgr.enumerate();
            for (const auto& d : devices) {
                if (d.path == path) {
                    DeviceConfig dc;
                    dc.path = d.path;
                    dc.name = d.name;
                    dc.vid = d.vid;
                    dc.pid = d.pid;
                    dc.grab = false; // will be set below
                    dc.type = "generic";
                    g_config.devices().push_back(dc);
                    dcPtr = &g_config.devices().back();
                    break;
                }
            }
        }
        if (!dcPtr) return {{"error", "Device not found: " + path}};
    } else if (!dcPtr && !grab) {
        return {{"ok", true}, {"note", "Device not in config, nothing to ungrab"}};
    }

    struct epoll_event ev = {};
    ev.events = EPOLLIN;

    if (dcPtr->type == "corsair-k100") {
        if (grab) {
            if (g_bragi.isInitialized()) {
                return {{"ok", true}, {"note", "Already grabbed"}};
            }
            if (!g_bragi.init()) {
                return {{"error", "Failed to init Corsair K100 BRAGI"}};
            }
            applyGkeyMappings();
            ev.data.fd = g_bragi.nkroFd();
            epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_bragi.nkroFd(), &ev);
            dcPtr->grab = true;
            g_config.save();
            fprintf(stderr, "[grab] Corsair K100 grabbed\n");
            return {{"ok", true}};
        } else {
            if (!g_bragi.isInitialized()) {
                dcPtr->grab = false;
                g_config.save();
                return {{"ok", true}, {"note", "Already ungrabbed"}};
            }
            // Remove from epoll BEFORE shutdown (shutdown closes the fd)
            epoll_ctl(g_epfd, EPOLL_CTL_DEL, g_bragi.nkroFd(), nullptr);
            g_bragi.shutdown();
            dcPtr->grab = false;
            g_config.save();
            fprintf(stderr, "[grab] Corsair K100 ungrabbed\n");
            return {{"ok", true}};
        }
    } else {
        // Generic device
        if (grab) {
            // Check not already grabbed
            for (const auto& gd : g_genericDevices) {
                if (gd.path == path) {
                    return {{"ok", true}, {"note", "Already grabbed"}};
                }
            }
            GenericDevice gd;
            gd.path = dcPtr->path;
            gd.name = dcPtr->name;
            gd.evdevFd = g_devMgr.grabDevice(dcPtr->path);
            if (gd.evdevFd < 0) {
                return {{"error", "Cannot grab device: " + path}};
            }
            std::string uinputName = "keyboard-remap: " + dcPtr->name;
            if (!gd.emitter.create(uinputName, 0, 0)) {
                g_devMgr.ungrabDevice(gd.evdevFd);
                return {{"error", "Cannot create uinput for: " + path}};
            }
            ev.data.fd = gd.evdevFd;
            epoll_ctl(g_epfd, EPOLL_CTL_ADD, gd.evdevFd, &ev);
            g_genericDevices.push_back(std::move(gd));
            dcPtr->grab = true;
            g_config.save();
            fprintf(stderr, "[grab] Generic device grabbed: %s\n", path.c_str());
            return {{"ok", true}};
        } else {
            // Find and ungrab
            for (auto it = g_genericDevices.begin(); it != g_genericDevices.end(); ++it) {
                if (it->path == path) {
                    epoll_ctl(g_epfd, EPOLL_CTL_DEL, it->evdevFd, nullptr);
                    it->emitter.destroy();
                    g_devMgr.ungrabDevice(it->evdevFd);
                    g_genericDevices.erase(it);
                    dcPtr->grab = false;
                    g_config.save();
                    fprintf(stderr, "[grab] Generic device ungrabbed: %s\n", path.c_str());
                    return {{"ok", true}};
                }
            }
            dcPtr->grab = false;
            g_config.save();
            return {{"ok", true}, {"note", "Already ungrabbed"}};
        }
    }
}

// ── Usage ───────────────────────────────────────────────────────

static void usage(const char* prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Keyboard input device manager and remapper\n\n");
    printf("Options:\n");
    printf("  --config PATH   Config file (default: %s)\n", DEFAULT_CONFIG);
    printf("  --socket PATH   UDS path (default: %s)\n", DEFAULT_SOCKET);
    printf("  -h, --help      Show this help\n");
}

// ── Main ────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::string configPath = DEFAULT_CONFIG;
    std::string socketPath = DEFAULT_SOCKET;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configPath = argv[++i];
        } else if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
            socketPath = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
    }

    // Load config
    if (!g_config.load(configPath)) {
        fprintf(stderr, "[main] Warning: using empty config\n");
    }

    // Init API server
    if (!g_api.init(socketPath)) {
        fprintf(stderr, "[main] Failed to init API server\n");
        return 1;
    }

    // Register API commands
    g_api.registerCommand("listDevices", handleListDevices);
    g_api.registerCommand("getStatus", handleGetStatus);
    g_api.registerCommand("getConfig", handleGetConfig);
    g_api.registerCommand("setConfig", handleSetConfig);
    g_api.registerCommand("grab", handleGrab);

    // Set up signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    // Create epoll instance
    g_epfd = epoll_create1(0);
    if (g_epfd < 0) {
        fprintf(stderr, "[main] epoll_create1 failed: %s\n", strerror(errno));
        return 1;
    }

    // Add API listener to epoll
    struct epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = g_api.listenerFd();
    epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_api.listenerFd(), &ev);

    // Initialize configured devices
    for (const auto& dc : g_config.devices()) {
        if (!dc.grab) continue;

        if (dc.type == "corsair-k100") {
            // Initialize BRAGI K100
            if (g_bragi.init()) {
                applyGkeyMappings();
                ev.events = EPOLLIN;
                ev.data.fd = g_bragi.nkroFd();
                epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_bragi.nkroFd(), &ev);
            } else {
                fprintf(stderr, "[main] Failed to init Corsair K100 BRAGI\n");
            }
        } else {
            // Generic device: grab evdev, create uinput, add to epoll
            GenericDevice gd;
            gd.path = dc.path;
            gd.name = dc.name;
            gd.evdevFd = g_devMgr.grabDevice(dc.path);
            if (gd.evdevFd < 0) {
                fprintf(stderr, "[main] Cannot grab %s, skipping\n", dc.path.c_str());
                continue;
            }

            std::string uinputName = "keyboard-remap: " + dc.name;
            if (!gd.emitter.create(uinputName, 0, 0)) {
                fprintf(stderr, "[main] Cannot create uinput for %s\n", dc.path.c_str());
                g_devMgr.ungrabDevice(gd.evdevFd);
                continue;
            }

            ev.events = EPOLLIN;
            ev.data.fd = gd.evdevFd;
            epoll_ctl(g_epfd, EPOLL_CTL_ADD, gd.evdevFd, &ev);

            g_genericDevices.push_back(std::move(gd));
        }
    }

    fprintf(stderr, "[main] Running — %zu generic devices, K100 %s\n",
            g_genericDevices.size(),
            g_bragi.isInitialized() ? "active" : "not found");

    // ── Main epoll loop ─────────────────────────────────────────

    struct epoll_event events[MAX_EVENTS];

    while (g_running) {
        int nfds = epoll_wait(g_epfd, events, MAX_EVENTS, 100);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            // API listener — accept new client
            if (fd == g_api.listenerFd()) {
                int clientFd = g_api.acceptClient();
                if (clientFd >= 0) {
                    ev.events = EPOLLIN;
                    ev.data.fd = clientFd;
                    epoll_ctl(g_epfd, EPOLL_CTL_ADD, clientFd, &ev);
                }
                continue;
            }

            // BRAGI K100 NKRO data
            if (g_bragi.isInitialized() && fd == g_bragi.nkroFd()) {
                g_bragi.processNkroPacket();
                continue;
            }

            // Generic evdev event
            bool isEvdev = false;
            for (auto& gd : g_genericDevices) {
                if (fd == gd.evdevFd) {
                    isEvdev = true;
                    struct input_event ie;
                    while (read(fd, &ie, sizeof(ie)) == (ssize_t)sizeof(ie)) {
                        if (ie.type == EV_KEY) {
                            // Track NumLock state
                            if (ie.code == KEY_NUMLOCK && ie.value == 1) {
                                gd.numLockOn = !gd.numLockOn;
                                fprintf(stderr, "[device] NumLock %s\n",
                                        gd.numLockOn ? "ON (regular)" : "OFF (custom)");
                            }

                            // NumLock OFF = custom mode: remap numpad keys
                            int code = ie.code;
                            if (!gd.numLockOn) {
                                code = remapNumpad(code);
                            }

                            gd.emitter.emitKey(code, ie.value);
                        } else if (ie.type == EV_SYN) {
                            gd.emitter.emitSyn();
                        } else {
                            // Pass through other event types (EV_MSC, etc.)
                            // For now just forward SYN and KEY
                        }
                    }
                    break;
                }
            }
            if (isEvdev) continue;

            // API client data
            bool isClient = false;
            for (int clientFd : g_api.clients()) {
                if (fd == clientFd) {
                    isClient = true;
                    if (g_api.processClient(fd)) {
                        epoll_ctl(g_epfd, EPOLL_CTL_DEL, fd, nullptr);
                    }
                    break;
                }
            }
            if (isClient) continue;
        }
    }

    // ── Shutdown ────────────────────────────────────────────────

    fprintf(stderr, "\n[main] Shutting down...\n");

    // Release generic devices
    for (auto& gd : g_genericDevices) {
        gd.emitter.destroy();
        g_devMgr.ungrabDevice(gd.evdevFd);
    }
    g_genericDevices.clear();

    // Shutdown BRAGI K100
    g_bragi.shutdown();

    // Shutdown API
    g_api.shutdown();

    close(g_epfd);
    fprintf(stderr, "[main] Done.\n");
    return 0;
}
