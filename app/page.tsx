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
  const [detailDevice, setDetailDevice] = useState<Config['devices'][number] | null>(null);

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

  const toggleDeviceType = async (dev: Config['devices'][number]) => {
    const newType = dev.type === 'corsair-k100' ? 'generic' : 'corsair-k100';
    const updated = {
      ...config,
      devices: config!.devices.map(d => d.path === dev.path ? { ...d, type: newType } : d),
    };
    try {
      const res = await fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(updated),
      });
      if (res.ok) {
        setDetailDevice({ ...dev, type: newType });
        fetchAll();
      }
    } catch { /* ignore */ }
  };

  const involvedDevices = config?.devices.filter(d => d.grab) || [];
  const availableDevices = devices
    .filter(d => d.isKeyboard || d.isMouse)
    .filter(d => !isGrabbed(d.path));

  if (loading) {
    return (
      <div data-id="loading" className="min-h-screen bg-zinc-950 text-zinc-100 flex items-center justify-center">
        <div className="text-zinc-500 text-lg">Loading...</div>
      </div>
    );
  }

  return (
    <div data-id="page-root" className="min-h-screen bg-zinc-950 text-zinc-100 px-10 py-8">
      <h1 data-id="page-title" className="text-3xl font-bold mb-8 tracking-tight">keyBoard</h1>

      {error && (
        <div data-id="error-banner" className="mb-6 p-4 bg-amber-950/50 border border-amber-800/60 rounded-lg text-amber-300 text-sm">
          {error}
        </div>
      )}

      {/* Involved Devices */}
      <div data-id="devices-section" className="bg-zinc-900/80 rounded-xl border border-zinc-800/80 p-6 shadow-lg">
        <div className="flex items-center justify-between mb-5">
          <h2 data-id="devices-title" className="text-xl font-semibold tracking-tight">Devices</h2>
          <button
            data-id="add-device-button"
            onClick={() => setShowDevicePicker(!showDevicePicker)}
            className="px-4 py-2 rounded-lg text-sm font-medium bg-blue-600 hover:bg-blue-500 text-white transition-colors shadow-sm"
          >
            {showDevicePicker ? 'Cancel' : 'Add Device'}
          </button>
        </div>

        {/* Device Picker */}
        {showDevicePicker && (
          <div data-id="device-picker" className="mb-5 p-4 bg-zinc-800/60 rounded-lg border border-zinc-700/60">
            <h3 data-id="picker-title" className="text-sm font-medium text-zinc-400 mb-3 uppercase tracking-wide">Available Input Devices</h3>
            <div className="space-y-2">
              {availableDevices.map(dev => (
                <div
                  key={dev.path}
                  data-id={`picker-device-${dev.path}`}
                  className="p-3.5 rounded-lg border border-zinc-700/50 bg-zinc-800/40 hover:bg-zinc-800/80 flex items-center justify-between transition-colors"
                >
                  <div className="flex-1 min-w-0">
                    <div className="flex items-center gap-2">
                      <span data-id={`picker-name-${dev.path}`} className="font-medium truncate">{dev.name || 'Unknown'}</span>
                      {dev.isKeyboard && (
                        <span data-id={`picker-badge-kbd-${dev.path}`} className="px-2 py-0.5 text-xs bg-zinc-700/80 text-zinc-300 rounded-md font-mono">
                          kbd
                        </span>
                      )}
                      {dev.isMouse && (
                        <span data-id={`picker-badge-mouse-${dev.path}`} className="px-2 py-0.5 text-xs bg-zinc-700/80 text-zinc-300 rounded-md font-mono">
                          mouse
                        </span>
                      )}
                    </div>
                    <div data-id={`picker-info-${dev.path}`} className="text-xs text-zinc-500 mt-1.5 font-mono">
                      {dev.path} &middot; {dev.vid}:{dev.pid}
                    </div>
                  </div>
                  <button
                    data-id={`picker-add-${dev.path}`}
                    onClick={() => addDevice(dev)}
                    className="ml-4 px-4 py-2 rounded-lg text-sm font-medium bg-zinc-700 hover:bg-zinc-600 text-zinc-200 transition-colors"
                  >
                    Add
                  </button>
                </div>
              ))}
              {availableDevices.length === 0 && (
                <div data-id="no-available-devices" className="text-zinc-500 text-sm p-3">No available devices to add</div>
              )}
            </div>
          </div>
        )}

        {/* Current Devices List */}
        <div data-id="device-list" className="space-y-3">
          {involvedDevices.map(dev => (
            <div
              key={dev.path}
              data-id={`device-${dev.path}`}
              onClick={() => setDetailDevice(dev)}
              className="p-4 rounded-lg border border-zinc-700/50 bg-zinc-800/50 flex items-center justify-between cursor-pointer hover:bg-zinc-800/80 transition-colors"
            >
              <span data-id={`device-name-${dev.path}`} className="font-medium truncate">{dev.name}</span>
              <button
                data-id={`device-remove-${dev.path}`}
                onClick={(e) => { e.stopPropagation(); removeDevice(dev.path); }}
                className="ml-4 px-4 py-2 rounded-lg text-sm font-medium bg-zinc-700 hover:bg-zinc-600 text-zinc-300 transition-colors"
              >
                Remove
              </button>
            </div>
          ))}
          {involvedDevices.length === 0 && (
            <div data-id="no-devices-configured" className="text-zinc-500 text-sm p-4">No devices configured</div>
          )}
        </div>
      </div>
      {/* Device Detail Dialog */}
      {detailDevice && (
        <div data-id="device-detail-overlay" className="fixed inset-0 bg-black/60 flex items-center justify-center z-50" onClick={() => setDetailDevice(null)}>
          <div data-id="device-detail-dialog" className="bg-zinc-900 border border-zinc-700 rounded-xl p-6 max-w-md w-full mx-4 shadow-2xl" onClick={e => e.stopPropagation()}>
            <h3 data-id="detail-name" className="text-lg font-semibold mb-4">{detailDevice.name}</h3>
            <div className="space-y-3 text-sm">
              <div className="flex justify-between">
                <span className="text-zinc-400">Path</span>
                <span data-id="detail-path" className="font-mono text-zinc-200">{detailDevice.path}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-zinc-400">VID:PID</span>
                <span data-id="detail-vidpid" className="font-mono text-zinc-200">{detailDevice.vid}:{detailDevice.pid}</span>
              </div>
              <div className="flex items-center justify-between">
                <div>
                  <span className="text-zinc-400">BRAGI protocol</span>
                  <p className="text-xs text-zinc-500 mt-0.5">G-key detection (G1-G6) via software mode</p>
                </div>
                <button
                  data-id="detail-bragi-toggle"
                  onClick={() => toggleDeviceType(detailDevice)}
                  className={`relative w-10 h-5 rounded-full transition-colors ${detailDevice.type === 'corsair-k100' ? 'bg-blue-600' : 'bg-zinc-600'}`}
                >
                  <span className={`absolute top-0.5 left-0.5 w-4 h-4 rounded-full bg-white transition-transform ${detailDevice.type === 'corsair-k100' ? 'translate-x-5' : ''}`} />
                </button>
              </div>
            </div>
            <button data-id="detail-close" onClick={() => setDetailDevice(null)} className="mt-5 w-full py-2 rounded-lg bg-zinc-700 hover:bg-zinc-600 text-sm font-medium text-zinc-200 transition-colors">
              Close
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
