export interface KeyDef {
  led: number;
  label: string;
  x: number;
  y: number;
  w: number;
  h: number;
}

export interface BarLed {
  led: number;
  x: number;
  y: number;
  w: number;
  h: number;
}

// ── Main keyboard keys ──────────────────────────────────────────
// LED indices match HID Usage IDs (same as bragiToLinux NKRO mapping)

export const KEYS: KeyDef[] = [
  // Row 0: G1 + Esc + F-keys + PrtSc/ScrLk/Pause
  { led: 131, label: 'G1', x: 0, y: 0, w: 1, h: 1 },
  { led: 41, label: 'Esc', x: 1.5, y: 0, w: 1, h: 1 },
  { led: 58, label: 'F1', x: 3.5, y: 0, w: 1, h: 1 },
  { led: 59, label: 'F2', x: 4.5, y: 0, w: 1, h: 1 },
  { led: 60, label: 'F3', x: 5.5, y: 0, w: 1, h: 1 },
  { led: 61, label: 'F4', x: 6.5, y: 0, w: 1, h: 1 },
  { led: 62, label: 'F5', x: 8, y: 0, w: 1, h: 1 },
  { led: 63, label: 'F6', x: 9, y: 0, w: 1, h: 1 },
  { led: 64, label: 'F7', x: 10, y: 0, w: 1, h: 1 },
  { led: 65, label: 'F8', x: 11, y: 0, w: 1, h: 1 },
  { led: 66, label: 'F9', x: 12.5, y: 0, w: 1, h: 1 },
  { led: 67, label: 'F10', x: 13.5, y: 0, w: 1, h: 1 },
  { led: 68, label: 'F11', x: 14.5, y: 0, w: 1, h: 1 },
  { led: 69, label: 'F12', x: 15.5, y: 0, w: 1, h: 1 },
  { led: 70, label: 'PrtSc', x: 16.75, y: 0, w: 1, h: 1 },
  { led: 71, label: 'ScrLk', x: 17.75, y: 0, w: 1, h: 1 },
  { led: 72, label: 'Pause', x: 18.75, y: 0, w: 1, h: 1 },

  // Row 1: G2 + Number row + Nav + Numpad
  { led: 132, label: 'G2', x: 0, y: 1.5, w: 1, h: 1 },
  { led: 53, label: '`', x: 1.5, y: 1.5, w: 1, h: 1 },
  { led: 30, label: '1', x: 2.5, y: 1.5, w: 1, h: 1 },
  { led: 31, label: '2', x: 3.5, y: 1.5, w: 1, h: 1 },
  { led: 32, label: '3', x: 4.5, y: 1.5, w: 1, h: 1 },
  { led: 33, label: '4', x: 5.5, y: 1.5, w: 1, h: 1 },
  { led: 34, label: '5', x: 6.5, y: 1.5, w: 1, h: 1 },
  { led: 35, label: '6', x: 7.5, y: 1.5, w: 1, h: 1 },
  { led: 36, label: '7', x: 8.5, y: 1.5, w: 1, h: 1 },
  { led: 37, label: '8', x: 9.5, y: 1.5, w: 1, h: 1 },
  { led: 38, label: '9', x: 10.5, y: 1.5, w: 1, h: 1 },
  { led: 39, label: '0', x: 11.5, y: 1.5, w: 1, h: 1 },
  { led: 45, label: '-', x: 12.5, y: 1.5, w: 1, h: 1 },
  { led: 46, label: '=', x: 13.5, y: 1.5, w: 1, h: 1 },
  { led: 42, label: 'Bksp', x: 14.5, y: 1.5, w: 2, h: 1 },
  { led: 73, label: 'Ins', x: 16.75, y: 1.5, w: 1, h: 1 },
  { led: 74, label: 'Home', x: 17.75, y: 1.5, w: 1, h: 1 },
  { led: 75, label: 'PgUp', x: 18.75, y: 1.5, w: 1, h: 1 },
  { led: 83, label: 'Num', x: 20, y: 1.5, w: 1, h: 1 },
  { led: 84, label: '/', x: 21, y: 1.5, w: 1, h: 1 },
  { led: 85, label: '*', x: 22, y: 1.5, w: 1, h: 1 },
  { led: 86, label: '-', x: 23, y: 1.5, w: 1, h: 1 },

  // Row 2: G3 + QWERTY + Nav + Numpad
  { led: 133, label: 'G3', x: 0, y: 2.5, w: 1, h: 1 },
  { led: 43, label: 'Tab', x: 1.5, y: 2.5, w: 1.5, h: 1 },
  { led: 20, label: 'Q', x: 3, y: 2.5, w: 1, h: 1 },
  { led: 26, label: 'W', x: 4, y: 2.5, w: 1, h: 1 },
  { led: 8, label: 'E', x: 5, y: 2.5, w: 1, h: 1 },
  { led: 21, label: 'R', x: 6, y: 2.5, w: 1, h: 1 },
  { led: 23, label: 'T', x: 7, y: 2.5, w: 1, h: 1 },
  { led: 28, label: 'Y', x: 8, y: 2.5, w: 1, h: 1 },
  { led: 24, label: 'U', x: 9, y: 2.5, w: 1, h: 1 },
  { led: 12, label: 'I', x: 10, y: 2.5, w: 1, h: 1 },
  { led: 18, label: 'O', x: 11, y: 2.5, w: 1, h: 1 },
  { led: 19, label: 'P', x: 12, y: 2.5, w: 1, h: 1 },
  { led: 47, label: '[', x: 13, y: 2.5, w: 1, h: 1 },
  { led: 48, label: ']', x: 14, y: 2.5, w: 1, h: 1 },
  { led: 49, label: '\\', x: 15, y: 2.5, w: 1.5, h: 1 },
  { led: 76, label: 'Del', x: 16.75, y: 2.5, w: 1, h: 1 },
  { led: 77, label: 'End', x: 17.75, y: 2.5, w: 1, h: 1 },
  { led: 78, label: 'PgDn', x: 18.75, y: 2.5, w: 1, h: 1 },
  { led: 95, label: '7', x: 20, y: 2.5, w: 1, h: 1 },
  { led: 96, label: '8', x: 21, y: 2.5, w: 1, h: 1 },
  { led: 97, label: '9', x: 22, y: 2.5, w: 1, h: 1 },
  { led: 87, label: '+', x: 23, y: 2.5, w: 1, h: 2 },

  // Row 3: G4 + Home row + Numpad
  { led: 134, label: 'G4', x: 0, y: 3.5, w: 1, h: 1 },
  { led: 57, label: 'Caps', x: 1.5, y: 3.5, w: 1.75, h: 1 },
  { led: 4, label: 'A', x: 3.25, y: 3.5, w: 1, h: 1 },
  { led: 22, label: 'S', x: 4.25, y: 3.5, w: 1, h: 1 },
  { led: 7, label: 'D', x: 5.25, y: 3.5, w: 1, h: 1 },
  { led: 9, label: 'F', x: 6.25, y: 3.5, w: 1, h: 1 },
  { led: 10, label: 'G', x: 7.25, y: 3.5, w: 1, h: 1 },
  { led: 11, label: 'H', x: 8.25, y: 3.5, w: 1, h: 1 },
  { led: 13, label: 'J', x: 9.25, y: 3.5, w: 1, h: 1 },
  { led: 14, label: 'K', x: 10.25, y: 3.5, w: 1, h: 1 },
  { led: 15, label: 'L', x: 11.25, y: 3.5, w: 1, h: 1 },
  { led: 51, label: ';', x: 12.25, y: 3.5, w: 1, h: 1 },
  { led: 52, label: "'", x: 13.25, y: 3.5, w: 1, h: 1 },
  { led: 40, label: 'Enter', x: 14.25, y: 3.5, w: 2.25, h: 1 },
  { led: 92, label: '4', x: 20, y: 3.5, w: 1, h: 1 },
  { led: 93, label: '5', x: 21, y: 3.5, w: 1, h: 1 },
  { led: 94, label: '6', x: 22, y: 3.5, w: 1, h: 1 },

  // Row 4: G5 + Shift row + Arrow + Numpad
  { led: 135, label: 'G5', x: 0, y: 4.5, w: 1, h: 1 },
  { led: 106, label: 'Shift', x: 1.5, y: 4.5, w: 2.25, h: 1 },
  { led: 29, label: 'Z', x: 3.75, y: 4.5, w: 1, h: 1 },
  { led: 27, label: 'X', x: 4.75, y: 4.5, w: 1, h: 1 },
  { led: 6, label: 'C', x: 5.75, y: 4.5, w: 1, h: 1 },
  { led: 25, label: 'V', x: 6.75, y: 4.5, w: 1, h: 1 },
  { led: 5, label: 'B', x: 7.75, y: 4.5, w: 1, h: 1 },
  { led: 17, label: 'N', x: 8.75, y: 4.5, w: 1, h: 1 },
  { led: 16, label: 'M', x: 9.75, y: 4.5, w: 1, h: 1 },
  { led: 54, label: ',', x: 10.75, y: 4.5, w: 1, h: 1 },
  { led: 55, label: '.', x: 11.75, y: 4.5, w: 1, h: 1 },
  { led: 56, label: '/', x: 12.75, y: 4.5, w: 1, h: 1 },
  { led: 110, label: 'Shift', x: 13.75, y: 4.5, w: 2.75, h: 1 },
  { led: 82, label: '\u2191', x: 17.75, y: 4.5, w: 1, h: 1 },
  { led: 89, label: '1', x: 20, y: 4.5, w: 1, h: 1 },
  { led: 90, label: '2', x: 21, y: 4.5, w: 1, h: 1 },
  { led: 91, label: '3', x: 22, y: 4.5, w: 1, h: 1 },
  { led: 88, label: 'Ent', x: 23, y: 4.5, w: 1, h: 2 },

  // Row 5: G6 + Space row + Arrows + Numpad
  { led: 136, label: 'G6', x: 0, y: 5.5, w: 1, h: 1 },
  { led: 105, label: 'Ctrl', x: 1.5, y: 5.5, w: 1.25, h: 1 },
  { led: 108, label: 'Win', x: 2.75, y: 5.5, w: 1.25, h: 1 },
  { led: 107, label: 'Alt', x: 4, y: 5.5, w: 1.25, h: 1 },
  { led: 44, label: '', x: 5.25, y: 5.5, w: 6.25, h: 1 },
  { led: 111, label: 'Alt', x: 11.5, y: 5.5, w: 1.25, h: 1 },
  { led: 122, label: 'Fn', x: 12.75, y: 5.5, w: 1.25, h: 1 },
  { led: 101, label: 'Menu', x: 14, y: 5.5, w: 1.25, h: 1 },
  { led: 109, label: 'Ctrl', x: 15.25, y: 5.5, w: 1.25, h: 1 },
  { led: 80, label: '\u2190', x: 16.75, y: 5.5, w: 1, h: 1 },
  { led: 81, label: '\u2193', x: 17.75, y: 5.5, w: 1, h: 1 },
  { led: 79, label: '\u2192', x: 18.75, y: 5.5, w: 1, h: 1 },
  { led: 98, label: '0', x: 20, y: 5.5, w: 2, h: 1 },
  { led: 99, label: '.', x: 22, y: 5.5, w: 1, h: 1 },
];

// ── Light bars ──────────────────────────────────────────────────

// Top: 22 LEDs (138-159) spanning full keyboard width
export const TOP_BAR: BarLed[] = Array.from({ length: 22 }, (_, i) => ({
  led: 138 + i,
  x: i * (24 / 22),
  y: -0.55,
  w: 24 / 22 - 0.1,
  h: 0.3,
}));

// Left: 11 LEDs (160-170) down the left edge
export const LEFT_BAR: BarLed[] = Array.from({ length: 11 }, (_, i) => ({
  led: 160 + i,
  x: -0.55,
  y: i * (6.5 / 11),
  w: 0.3,
  h: 6.5 / 11 - 0.08,
}));

// Right: 11 LEDs (171-181) down the right edge
export const RIGHT_BAR: BarLed[] = Array.from({ length: 11 }, (_, i) => ({
  led: 171 + i,
  x: 24.25,
  y: i * (6.5 / 11),
  w: 0.3,
  h: 6.5 / 11 - 0.08,
}));

// ── Control wheel ───────────────────────────────────────────────

// iCUE control wheel is upper-LEFT, above the Esc-F1 gap
export const WHEEL = {
  cx: 2.5,
  cy: -0.6,
  button: 137,                                       // center button
  ring: [189, 182, 183, 184, 185, 186, 187, 188],   // 8 ring segments, clockwise from top-right
};

// ── Logo (centered in top strip) ────────────────────────────────

export const LOGO: BarLed[] = [
  { led: 190, x: 11.2, y: -0.55, w: 0.6, h: 0.3 },
  { led: 191, x: 11.8, y: -0.55, w: 0.6, h: 0.3 },
  { led: 192, x: 12.4, y: -0.55, w: 0.6, h: 0.3 },
];
