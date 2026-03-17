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

interface GrabbedDevice {
  path: string;
  name: string;
  type: string;
  numLockOn?: boolean;
  gkeys?: boolean;
}

interface Status {
  running: boolean;
  grabbedDevices: GrabbedDevice[];
  bragiInitialized: boolean;
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
  const [status, setStatus] = useState<Status | null>(null);
  const [config, setConfig] = useState<Config | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

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
        setStatus(await statusRes.json());
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

  const toggleGrab = async (dev: Device) => {
    const currentlyGrabbed = isGrabbed(dev.path);
    try {
      const res = await fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          ...config,
          devices: currentlyGrabbed
            ? config!.devices.filter(d => d.path !== dev.path)
            : [...(config?.devices || []), {
                path: dev.path,
                name: dev.name,
                vid: dev.vid,
                pid: dev.pid,
                grab: true,
                type: isCorsairK100(dev) ? 'corsair-k100' : 'generic',
              }],
        }),
      });
      if (res.ok) fetchAll();
    } catch { /* ignore */ }
  };

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

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Device List */}
        <div className="bg-zinc-900 rounded-lg border border-zinc-800 p-4">
          <h2 className="text-lg font-semibold mb-3">Input Devices</h2>
          <div className="space-y-2">
            {devices.filter(d => d.isKeyboard || d.isMouse).map(dev => (
              <div
                key={dev.path}
                className={`p-3 rounded border ${
                  isGrabbed(dev.path)
                    ? 'bg-zinc-800 border-blue-700'
                    : 'bg-zinc-850 border-zinc-700'
                }`}
              >
                <div className="flex items-center justify-between">
                  <div className="flex-1 min-w-0">
                    <div className="flex items-center gap-2">
                      <span className="font-medium truncate">{dev.name || 'Unknown'}</span>
                      {isCorsairK100(dev) && (
                        <span className="px-1.5 py-0.5 text-xs bg-purple-900 text-purple-300 rounded">
                          G-Keys
                        </span>
                      )}
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
                    onClick={() => toggleGrab(dev)}
                    className={`ml-3 px-3 py-1.5 rounded text-sm font-medium transition-colors ${
                      isGrabbed(dev.path)
                        ? 'bg-blue-700 hover:bg-blue-600 text-white'
                        : 'bg-zinc-700 hover:bg-zinc-600 text-zinc-300'
                    }`}
                  >
                    {isGrabbed(dev.path) ? 'Grabbed' : 'Grab'}
                  </button>
                </div>
              </div>
            ))}
            {devices.filter(d => d.isKeyboard || d.isMouse).length === 0 && (
              <div className="text-zinc-500 text-sm p-3">No input devices found</div>
            )}
          </div>
        </div>

        {/* Status Panel */}
        <div className="space-y-6">
          <div className="bg-zinc-900 rounded-lg border border-zinc-800 p-4">
            <h2 className="text-lg font-semibold mb-3">Status</h2>
            {status ? (
              <div className="space-y-3">
                <div className="flex items-center gap-2">
                  <div className={`w-2 h-2 rounded-full ${status.running ? 'bg-green-500' : 'bg-red-500'}`} />
                  <span className="text-sm">
                    Server {status.running ? 'running' : 'stopped'}
                  </span>
                </div>

                {status.bragiInitialized && (
                  <div className="flex items-center gap-2">
                    <div className="w-2 h-2 rounded-full bg-purple-500" />
                    <span className="text-sm">Corsair K100 BRAGI active</span>
                  </div>
                )}

                <div className="mt-3">
                  <h3 className="text-sm font-medium text-zinc-400 mb-2">Grabbed Devices</h3>
                  {status.grabbedDevices.length > 0 ? (
                    <div className="space-y-1">
                      {status.grabbedDevices.map((gd, i) => (
                        <div key={i} className="text-sm p-2 bg-zinc-800 rounded flex items-center justify-between">
                          <span className="truncate">{gd.name}</span>
                          <div className="flex items-center gap-2 ml-2">
                            {gd.type === 'corsair-k100' && (
                              <span className="px-1.5 py-0.5 text-xs bg-purple-900 text-purple-300 rounded">
                                BRAGI
                              </span>
                            )}
                            {gd.numLockOn !== undefined && (
                              <span className={`px-1.5 py-0.5 text-xs rounded ${
                                gd.numLockOn
                                  ? 'bg-green-900 text-green-300'
                                  : 'bg-amber-900 text-amber-300'
                              }`}>
                                {gd.numLockOn ? 'Regular' : 'Custom'}
                              </span>
                            )}
                          </div>
                        </div>
                      ))}
                    </div>
                  ) : (
                    <div className="text-zinc-500 text-sm">No devices grabbed</div>
                  )}
                </div>
              </div>
            ) : (
              <div className="text-zinc-500 text-sm">No status available</div>
            )}
          </div>

          {/* G-Key Mappings */}
          {config && Object.keys(config.gkeyMap).length > 0 && (
            <div className="bg-zinc-900 rounded-lg border border-zinc-800 p-4">
              <h2 className="text-lg font-semibold mb-3">G-Key Mappings</h2>
              <div className="grid grid-cols-2 gap-2">
                {Object.entries(config.gkeyMap).map(([gkey, mapping]) => (
                  <div key={gkey} className="flex items-center justify-between p-2 bg-zinc-800 rounded text-sm">
                    <span className="font-medium text-purple-300">{gkey}</span>
                    <span className="text-zinc-400">{mapping}</span>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* NumLock Custom Mode */}
          {config && Object.keys(config.numpadCustomMode).length > 0 && (
            <div className="bg-zinc-900 rounded-lg border border-zinc-800 p-4">
              <h2 className="text-lg font-semibold mb-3">NumLock Custom Mode</h2>
              <p className="text-xs text-zinc-500 mb-2">NumLock OFF remaps numpad keys to:</p>
              <div className="grid grid-cols-3 gap-2">
                {Object.entries(config.numpadCustomMode).map(([from, to]) => (
                  <div key={from} className="flex items-center justify-between p-2 bg-zinc-800 rounded text-sm">
                    <span className="text-zinc-400">{from}</span>
                    <span className="text-zinc-300">{to}</span>
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
