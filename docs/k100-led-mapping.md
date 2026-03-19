# Corsair K100 RGB — LED Index Mapping (BRAGI Protocol)

LED indices in the RGB buffer directly correspond to USB HID Usage IDs — the same indices used in `bragiToLinux[]` for NKRO key detection.

**193 LEDs total** (indices 0-192), addressed via planar RGB buffer.

## Key LEDs (Resource 0x0022)

### Status/Lock (0-1)
| LED | Key |
|-----|-----|
| 0 | Status indicator |
| 1 | Win Lock |

### Letters A-Z (4-29)
Sequential: LED 4=A, 5=B, 6=C, ... 29=Z

### Number Row (30-39)
LED 30=1, 31=2, 32=3, 33=4, 34=5, 35=6, 36=7, 37=8, 38=9, 39=0

### Standard Keys (40-56)
| LED | Key |
|-----|-----|
| 40 | Enter |
| 41 | Esc |
| 42 | Backspace |
| 43 | Tab |
| 44 | Space |
| 45 | Minus (-) |
| 46 | Equal (=) |
| 47 | Left Bracket ([) |
| 48 | Right Bracket (]) |
| 49 | Backslash (\) |
| 50 | ISO Hash (#) |
| 51 | Semicolon (;) |
| 52 | Apostrophe (') |
| 53 | Grave/Tilde (`) |
| 54 | Comma (,) |
| 55 | Period (.) |
| 56 | Slash (/) |

### CapsLock + F-Keys (57-69)
| LED | Key |
|-----|-----|
| 57 | CapsLock |
| 58-69 | F1-F12 |

### Navigation (70-82)
| LED | Key |
|-----|-----|
| 70 | PrtScn |
| 71 | ScrollLock |
| 72 | Pause |
| 73 | Insert |
| 74 | Home |
| 75 | PgUp |
| 76 | Delete |
| 77 | End |
| 78 | PgDn |
| 79 | Right |
| 80 | Left |
| 81 | Down |
| 82 | Up |

### Numpad (83-99)
| LED | Key |
|-----|-----|
| 83 | NumLock |
| 84 | Num / |
| 85 | Num * |
| 86 | Num - |
| 87 | Num + |
| 88 | NumEnter |
| 89-97 | Num 1-9 |
| 98 | Num 0 |
| 99 | Num . |

### Modifiers & Misc (100-115)
| LED | Key |
|-----|-----|
| 100 | ISO Backslash |
| 101 | Menu/Compose |
| 102 | Mute |
| 103 | Vol Up |
| 104 | Vol Down |
| 105 | LCtrl |
| 106 | LShift |
| 107 | LAlt |
| 108 | LWin |
| 109 | RCtrl |
| 110 | RShift |
| 111 | RAlt |
| 112 | RWin |
| 113 | Brightness |
| 115 | RO (Japanese) |

### Media & Fn (122-128)
| LED | Key |
|-----|-----|
| 122 | Fn |
| 123 | Stop |
| 124 | Play/Pause |
| 125 | Next Track |
| 126 | Prev Track |
| 128 | Profile Switch |

### G-Keys (131-136)
| LED | Key |
|-----|-----|
| 131 | G1 |
| 132 | G2 |
| 133 | G3 |
| 134 | G4 |
| 135 | G5 |
| 136 | G6 |

## Extra LEDs

### Control Wheel (137, 182-189)
The iCUE control wheel is in the **upper-left** of the keyboard (above Esc/F1 gap), NOT upper-right. The volume roller in the upper-right is a separate unlighted control.

| LED | Position |
|-----|----------|
| 137 | Wheel button |
| 189 | Ring pos 1 (top-right) |
| 182-188 | Ring pos 2-8 (clockwise) |

### Top Light Bar (138-159)
22 LEDs, left-to-right across the top edge.

### Left Light Bar (160-170)
11 LEDs, top-to-bottom on the left edge.

### Right Light Bar (171-181)
11 LEDs, top-to-bottom on the right edge.

### Logo (190-192)
Centered in the top strip between the iCUE wheel (left) and volume roller (right).

| LED | Position |
|-----|----------|
| 190 | Logo left |
| 191 | Logo center |
| 192 | Logo right (not addressable with 579-byte buffer) |

## Unused Indices
2, 3, 114, 116-121, 127, 129, 130

## Protocol Notes (Empirically Verified)

- **Resource 0x0022**: Interleaved **RGB** with **2-byte zero padding** at the start of the buffer.
  - Buffer layout: `[0x00, 0x00, R0, G0, B0, R1, G1, B1, ..., R191, G191, B191]` = 579 bytes total.
  - LED N occupies bytes `2 + N*3` (R), `3 + N*3` (G), `4 + N*3` (B).
  - With 579-byte buffer, 192 LEDs addressable (0-191). LED 192 is excluded.
- **DO NOT use 0x12 header byte** — ckb-next docs suggest `(0x12, 0x00)` header but this causes severe keyboard input lag. Use `(0x00, 0x00)` zero padding instead.
- **DO NOT write to resource 0x002E** — Writing to the light bar resource interferes with key LED colors on 0x0022, causing random keys to display wrong colors. The channel test showing "GBR" order was an artifact of 0x002E overriding 0x0022.
- **Resource 0x002E** (light bar/extra LEDs): Format unknown. Currently not written to. Light bar, wheel ring, and logo LEDs are not controlled.
- **Packet size**: K100 uses BRAGI jumbo packets (1024 bytes). The 579-byte payload fits in a single packet (no continuation needed).
- **Sources**: ckb-next `src/daemon/keymap.c` (`keymap_bragi[]`), OpenRGB `CorsairPeripheralV2Devices.cpp` (`corsair_k100_layout`). Note: ckb-next's documented format details (GBR order, 0x12 header) did NOT match our hardware — empirical testing was required.
