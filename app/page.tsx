'use client';

import { useState, useEffect, useCallback } from 'react';

interface Device {
  path: string;
  name: string;
  vid: string;
  pid: string;
  isKeyboard: boolean;
  isMouse: boolean;
  phys: string;
}

interface Config {
  devices: Array<{
    path: string;
    name: string;
    vid: string;
    pid: string;
    grab: boolean;
    type: string;
  }>;
  gkeyMap: Record<string, string>;
  numpadCustomMode: Record<string, string>;
}

export default function Home() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [config, setConfig] = useState<Config | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);
  const [showDevicePicker, setShowDevicePicker] = useState(false);

  const fetchAll = useCallback(async () => {
    try {
      const [devRes, statusRes, configRes] = await Promise.all([
        fetch('/api/devices'),
        fetch('/api/status'),
        fetch('/api/config'),
      ]);

      if (devRes.ok) {
        const data = await devRes.json();
        setDevices(data.devices || []);
      }
      if (statusRes.ok) {
        setError(null);
      } else {
        setError('Cannot connect to keyboard server');
      }
      if (configRes.ok) {
        setConfig(await configRes.json());
      }
    } catch {
      setError('Cannot connect to keyboard server');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchAll();
    const interval = setInterval(fetchAll, 3000);
    return () => clearInterval(interval);
  }, [fetchAll]);

  const isGrabbed = (path: string) => {
    return config?.devices.some(d => d.path === path && d.grab) || false;
  };

  const isCorsairK100 = (dev: Device) => {
    return dev.vid === '1b1c' && dev.pid === '1bc5' && dev.isKeyboard;
  };

  const addDevice = async (dev: Device) => {
    try {
      const res = await fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          ...config,
          devices: [...(config?.devices || []), {
            path: dev.path,
            name: dev.name,
            vid: dev.vid,
            pid: dev.pid,
            grab: true,
            type: isCorsairK100(dev) ? 'corsair-k100' : 'generic',
          }],
        }),
      });
      if (res.ok) {
        setShowDevicePicker(false);
        fetchAll();
      }
    } catch { /* ignore */ }
  };

  const removeDevice = async (path: string) => {
    try {
      const res = await fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          ...config,
          devices: config!.devices.filter(d => d.path !== path),
        }),
      });
      if (res.ok) fetchAll();
    } catch { /* ignore */ }
  };

  const involvedDevices = config?.devices.filter(d => d.grab) || [];
  const availableDevices = devices
    .filter(d => d.isKeyboard || d.isMouse)
    .filter(d => !isGrabbed(d.path));

  if (loading) {
    return (
      <div className="min-h-screen bg-zinc-950 text-zinc-100 flex items-center justify-center">
        <div className="text-zinc-500">Loading...</div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 p-6">
      <h1 className="text-2xl font-bold mb-6">keyBoard</h1>

      {error && (
        <div className="mb-4 p-3 bg-red-950 border border-red-800 rounded text-red-300 text-sm">
          {error}
        </div>
      )}

      <div className="max-w-2xl">
        {/* Involved Devices */}
        <div className="bg-zinc-900 rounded-lg border border-zinc-800 p-4">
          <div className="flex items-center justify-between mb-3">
            <h2 className="text-lg font-semibold">Devices</h2>
            <button
              onClick={() => setShowDevicePicker(!showDevicePicker)}
              className="px-3 py-1.5 rounded text-sm font-medium bg-blue-700 hover:bg-blue-600 text-white transition-colors"
            >
              {showDevicePicker ? 'Cancel' : 'Add Device'}
            </button>
          </div>

          {/* Device Picker */}
          {showDevicePicker && (
            <div className="mb-4 p-3 bg-zinc-800 rounded border border-zinc-700">
              <h3 className="text-sm font-medium text-zinc-400 mb-2">Input Devices</h3>
              <div className="space-y-2">
                {availableDevices.map(dev => (
                  <div
                    key={dev.path}
                    className="p-3 rounded border border-zinc-600 bg-zinc-850 flex items-center justify-between"
                  >
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center gap-2">
                        <span className="font-medium truncate">{dev.name || 'Unknown'}</span>
                        {dev.isKeyboard && (
                          <span className="px-1.5 py-0.5 text-xs bg-zinc-700 text-zinc-300 rounded">
                            kbd
                          </span>
                        )}
                        {dev.isMouse && (
                          <span className="px-1.5 py-0.5 text-xs bg-zinc-700 text-zinc-300 rounded">
                            mouse
                          </span>
                        )}
                      </div>
                      <div className="text-xs text-zinc-500 mt-1">
                        {dev.path} &middot; {dev.vid}:{dev.pid}
                      </div>
                    </div>
                    <button
                      onClick={() => addDevice(dev)}
                      className="ml-3 px-3 py-1.5 rounded text-sm font-medium bg-zinc-700 hover:bg-zinc-600 text-zinc-300 transition-colors"
                    >
                      Add
                    </button>
                  </div>
                ))}
                {availableDevices.length === 0 && (
                  <div className="text-zinc-500 text-sm p-2">No available devices to add</div>
                )}
              </div>
            </div>
          )}

          {/* Current Devices List */}
          <div className="space-y-2">
            {involvedDevices.map(dev => (
              <div
                key={dev.path}
                className="p-3 rounded border border-blue-700 bg-zinc-800 flex items-center justify-between"
              >
                <div className="flex-1 min-w-0">
                  <div className="flex items-center gap-2">
                    <span className="font-medium truncate">{dev.name}</span>
                    <span className="px-1.5 py-0.5 text-xs bg-zinc-700 text-zinc-300 rounded">
                      {dev.type}
                    </span>
                  </div>
                  <div className="text-xs text-zinc-500 mt-1">
                    {dev.path} &middot; {dev.vid}:{dev.pid}
                  </div>
                </div>
                <button
                  onClick={() => removeDevice(dev.path)}
                  className="ml-3 px-3 py-1.5 rounded text-sm font-medium bg-red-900 hover:bg-red-800 text-red-300 transition-colors"
                >
                  Remove
                </button>
              </div>
            ))}
            {involvedDevices.length === 0 && (
              <div className="text-zinc-500 text-sm p-3">No devices configured</div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
