# CLAUDE.md — keyBoard

## Overview

Input device manager for Linux — grabs keyboards/mice via evdev, remaps keys, and supports Corsair K100 G-keys via the BRAGI protocol. Two components:

- **C++ Server** (`server/`): Runs as systemd service, grabs evdev devices, creates uinput virtual devices, handles BRAGI protocol for K100, exposes a JSON API over Unix domain socket
- **Next.js Frontend** (`app/`): Device management UI, LED color control, communicates with server via API routes → UDS

## Build

```bash
cd server && ./build.sh          # Build C++ server
./build.sh -rebuild              # Full rebuild (clears build/)
```

The server binary is output to `server/keyboard-server`. Managed by daemon app commands:
```bash
d restartApp --app keyBoard      # Restart server + frontend (dev + prod)
d stopApp --app keyBoard
d startApp --app keyBoard
```

## Architecture

```
Browser → Next.js API routes → UDS (/run/keyboard-server.sock) → C++ server
                                                                    ↓
                                                              evdev grab + uinput emit
                                                              BRAGI K100 (hidraw)
```

### Server Components

| File | Purpose |
|------|---------|
| `main.cpp` | Epoll loop, API command handlers (listDevices, getStatus, getConfig, setConfig, grab, setColor) |
| `BragiK100.cpp/h` | BRAGI protocol: SW/HW mode, NKRO input, G-key mapping, LED color control |
| `DeviceManager.cpp/h` | evdev enumeration and EVIOCGRAB |
| `HidrawManager.cpp/h` | hidraw device discovery by VID/PID/interface |
| `UInputEmitter.cpp/h` | uinput virtual device creation and key emission |
| `ConfigManager.cpp/h` | JSON config load/save (`server/config/keyboard.json`) |
| `ApiServer.cpp/h` | Unix domain socket JSON API server |

### Frontend Routes

| Route | Purpose |
|-------|---------|
| `/` (`app/page.tsx`) | Main page: device list, grab/ungrab, device detail dialog |
| `/color` (`app/color/page.tsx`) | LED color picker and presets for K100 |
| `/api/devices` | → `listDevices` |
| `/api/status` | → `getStatus` |
| `/api/config` | → `getConfig` / `setConfig` |
| `/api/grab` | → `grab` |
| `/api/color` | → `setColor` |

### UDS Communication

`lib/server-connection.ts` connects to `/run/keyboard-server.sock`, sends JSON `{command, ...args}`, reads newline-terminated JSON response.

## Corsair K100 BRAGI Protocol

The K100 uses Corsair's BRAGI protocol over USB HID (1024-byte packets on interface 1).

### Key Facts
- **Software mode** (0x02): Host controls input AND LEDs. Required for G-key detection. LEDs go black if host doesn't send RGB data.
- **Hardware mode** (0x01): Keyboard controls everything from onboard profile. No G-key detection possible.
- **Resource 0x0001** (RES_LIGHTING): NOT supported on K100 (returns 0x06)
- **Resource 0x0022** (ALT_LIGHTING): Key LEDs — write interleaved RGB here
- **Resource 0x002E** (LIGHTING_EXTRA): Do NOT write — interferes with key LEDs on 0x0022
- **192 addressable LED zones** (indices 0-191, LED 192 excluded)
- **RGB format**: Interleaved with 2-byte zero padding — `[0x00, 0x00, R0, G0, B0, R1, G1, B1, ...]` = 579 bytes
- **Do NOT use 0x12 header byte** — causes severe keyboard input lag; use `0x00, 0x00` padding
- **G-keys**: NKRO indices 131-136 on interface 2 (hidraw), mapped to F13-F18 by default
- **SET packet format**: `{0x08, 0x01, prop_id, 0x00, value}` — note padding byte at offset 3
- **CMD 0x0C crashes the keyboard** — never send it

### Soft Ungrab/Regrab

Toggle grab without mode switch to preserve LED state:
- `ungrab()`: Release keys, destroy uinput, ungrab evdev. Stays in SW mode.
- `regrab()`: Drain stale NKRO packets, re-grab evdev, re-create uinput.
- Full `shutdown()` (HW mode restore) only on service exit.

### USB Interfaces (VID 1b1c PID 1bc5)
| Interface | hidraw | Purpose |
|-----------|--------|---------|
| 0 | hidraw6 | Standard keyboard HID (NKRO) — grabbed to suppress duplicates |
| 1 | hidraw7 | BRAGI control (1024B packets) |
| 2 | hidraw8 | BRAGI NKRO input (G-keys detected here) |
| 3 | hidraw9 | Mouse/scroll wheel |

## Config

`server/config/keyboard.json` — persisted by the server. Contains device list, G-key mappings, numpad custom mode mappings.

## Dependencies

### Server (C++)
CMake 3.10+, C++17, nlohmann/json (header-only), libevdev headers, linux/hidraw.h, linux/uinput.h

### Frontend
Next.js 16, React 19, TypeScript, Tailwind CSS 4
