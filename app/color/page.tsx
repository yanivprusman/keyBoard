'use client';

import { useState, useEffect, useRef, useCallback } from 'react';
import { KEYS, TOP_BAR, LEFT_BAR, RIGHT_BAR, WHEEL, LOGO, type KeyDef, type BarLed } from './k100-layout';

const UNIT = 46;
const GAP = 2;
const OX = 36;
const OY = 44;
const OFF_COLOR = '#111113';

function luminance(hex: string) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return (r * 299 + g * 587 + b * 114) / 1000;
}

function hexToRgb(hex: string) {
  return {
    r: parseInt(hex.slice(1, 3), 16),
    g: parseInt(hex.slice(3, 5), 16),
    b: parseInt(hex.slice(5, 7), 16),
  };
}

export default function ColorPage() {
  const [ledColors, setLedColors] = useState<Record<number, string>>({});
  const [pickerColor, setPickerColor] = useState('#00c8ff');
  const [selectedKey, setSelectedKey] = useState<number | null>(null);
  const [error, setError] = useState<string | null>(null);
  const debounceRef = useRef<ReturnType<typeof setTimeout>>();

  useEffect(() => {
    try {
      const saved = localStorage.getItem('k100-leds');
      if (saved) setLedColors(JSON.parse(saved));
    } catch { /* ignore */ }
  }, []);

  const persist = useCallback((colors: Record<number, string>) => {
    setLedColors(colors);
    localStorage.setItem('k100-leds', JSON.stringify(colors));
  }, []);

  const sendColor = useCallback(async (body: { r: number; g: number; b: number; led?: number }) => {
    try {
      const res = await fetch('/api/color', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      });
      const data = await res.json();
      if (data.error) setError(data.error);
      else setError(null);
    } catch {
      setError('Cannot connect to keyboard server');
    }
  }, []);

  const applyAll = useCallback((hex: string) => {
    const next: Record<number, string> = {};
    for (let i = 0; i < 193; i++) next[i] = hex;
    persist(next);
    const { r, g, b } = hexToRgb(hex);
    clearTimeout(debounceRef.current);
    debounceRef.current = setTimeout(() => sendColor({ r, g, b }), 150);
  }, [persist, sendColor]);

  const applyKey = useCallback((led: number, hex: string) => {
    persist({ ...ledColors, [led]: hex });
    const { r, g, b } = hexToRgb(hex);
    clearTimeout(debounceRef.current);
    debounceRef.current = setTimeout(() => sendColor({ led, r, g, b }), 150);
  }, [persist, sendColor, ledColors]);

  const handlePickerChange = (hex: string) => {
    setPickerColor(hex);
    if (selectedKey !== null) {
      applyKey(selectedKey, hex);
    } else {
      applyAll(hex);
    }
  };

  const handleKeyClick = (led: number) => {
    if (selectedKey === led) {
      setSelectedKey(null);
    } else {
      setSelectedKey(led);
      const c = ledColors[led];
      if (c) setPickerColor(c);
    }
  };

  const color = (led: number) => ledColors[led] || OFF_COLOR;
  const isLit = (led: number) => {
    const c = color(led);
    return c !== OFF_COLOR && c !== '#000000';
  };
  const textColor = (led: number) =>
    luminance(color(led)) > 80 ? 'rgba(0,0,0,0.6)' : 'rgba(255,255,255,0.4)';

  const containerW = OX * 2 + 24.6 * UNIT;
  const containerH = OY + 6.5 * UNIT + 20;

  const renderBar = (b: BarLed) => {
    const c = color(b.led);
    const lit = isLit(b.led);
    return (
      <div
        key={`bar-${b.led}`}
        onClick={() => handleKeyClick(b.led)}
        style={{
          position: 'absolute',
          left: OX + b.x * UNIT,
          top: OY + b.y * UNIT,
          width: b.w * UNIT,
          height: b.h * UNIT,
          backgroundColor: c,
          borderRadius: 2,
          boxShadow: lit ? `0 0 8px ${c}80` : 'none',
          cursor: 'pointer',
          outline: selectedKey === b.led ? '2px solid #fff' : 'none',
        }}
      />
    );
  };

  const renderKey = (key: KeyDef) => {
    const c = color(key.led);
    const lit = isLit(key.led);
    const selected = selectedKey === key.led;
    return (
      <div
        key={`key-${key.led}`}
        onClick={() => handleKeyClick(key.led)}
        style={{
          position: 'absolute',
          left: OX + key.x * UNIT + GAP,
          top: OY + key.y * UNIT + GAP,
          width: key.w * UNIT - GAP * 2,
          height: key.h * UNIT - GAP * 2,
          backgroundColor: c,
          borderRadius: 5,
          border: selected ? '2px solid #fff' : `1px solid ${lit ? `${c}60` : '#222'}`,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          boxShadow: lit ? `inset 0 0 8px ${c}30` : 'none',
          cursor: 'pointer',
        }}
      >
        <span
          style={{
            color: textColor(key.led),
            fontSize: key.w > 1.5 ? 10 : 9,
            fontWeight: 500,
            userSelect: 'none',
            letterSpacing: '0.02em',
          }}
        >
          {key.label}
        </span>
      </div>
    );
  };

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex flex-col items-center py-8 overflow-x-auto">
      {error && (
        <div className="mb-4 px-4 py-2 bg-red-950/60 border border-red-800/50 rounded-lg text-red-300 text-sm max-w-lg">
          {error}
        </div>
      )}

      {/* Keyboard */}
      <div
        className="relative"
        style={{ width: containerW, height: containerH }}
        onClick={(e) => {
          if (e.target === e.currentTarget) setSelectedKey(null);
        }}
      >
        {/* Case background */}
        <div
          style={{
            position: 'absolute',
            left: OX - 6,
            top: OY - 6,
            width: 24 * UNIT + 12,
            height: 6.5 * UNIT + 12,
            borderRadius: 10,
            backgroundColor: '#0a0a0c',
          }}
        />

        {/* Light bars */}
        {TOP_BAR.map(renderBar)}
        {LEFT_BAR.map(renderBar)}
        {RIGHT_BAR.map(renderBar)}
        {LOGO.map(renderBar)}

        {/* Keys */}
        {KEYS.map(renderKey)}

        {/* Wheel: center button */}
        <div
          onClick={() => handleKeyClick(WHEEL.button)}
          style={{
            position: 'absolute',
            left: OX + WHEEL.cx * UNIT - 14,
            top: OY + WHEEL.cy * UNIT - 14,
            width: 28,
            height: 28,
            borderRadius: '50%',
            backgroundColor: color(WHEEL.button),
            border: selectedKey === WHEEL.button ? '2px solid #fff' : `1px solid ${isLit(WHEEL.button) ? color(WHEEL.button) + '60' : '#222'}`,
            cursor: 'pointer',
          }}
        />

        {/* Wheel: ring LEDs */}
        {WHEEL.ring.map((led, i) => {
          const angle = ((i * 45) - 90) * Math.PI / 180;
          const r = UNIT * 0.55;
          const cx = OX + WHEEL.cx * UNIT + Math.cos(angle) * r;
          const cy = OY + WHEEL.cy * UNIT + Math.sin(angle) * r;
          const c = color(led);
          const lit = isLit(led);
          return (
            <div
              key={`wheel-${led}`}
              onClick={() => handleKeyClick(led)}
              style={{
                position: 'absolute',
                left: cx - 5,
                top: cy - 5,
                width: 10,
                height: 10,
                borderRadius: '50%',
                backgroundColor: c,
                border: selectedKey === led ? '2px solid #fff' : `1px solid ${lit ? c + '60' : '#222'}`,
                boxShadow: lit ? `0 0 6px ${c}80` : 'none',
                cursor: 'pointer',
              }}
            />
          );
        })}
      </div>

      {/* Controls */}
      <div className="mt-6 flex items-center gap-3">
        <input
          type="color"
          value={pickerColor}
          onChange={(e) => handlePickerChange(e.target.value)}
          className="w-12 h-12 rounded-lg border border-zinc-700 bg-transparent cursor-pointer"
        />
        <div className="text-xs text-zinc-500">
          {selectedKey !== null ? (
            <span>LED {selectedKey} <button className="text-zinc-400 underline ml-1" onClick={() => setSelectedKey(null)}>deselect</button></span>
          ) : (
            <span>All keys</span>
          )}
        </div>
      </div>
    </div>
  );
}
