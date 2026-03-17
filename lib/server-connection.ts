import net from 'net';

export const SOCKET_PATH = '/run/keyboard-server.sock';

export function sendToServer(commandObj: Record<string, unknown>, timeoutMs = 2000): Promise<string> {
  return new Promise((resolve, reject) => {
    const client = net.createConnection(SOCKET_PATH);
    let response = '';
    let done = false;

    const finish = (fn: (val: any) => void, val: any) => {
      if (done) return;
      done = true;
      client.destroy();
      fn(val);
    };

    client.on('connect', () => {
      client.write(JSON.stringify(commandObj) + '\n');
    });

    client.on('data', (data) => {
      response += data.toString();
      if (response.endsWith('\n')) {
        finish(resolve, response.trim());
      }
    });

    client.on('error', (err) => finish(reject, err));
    client.on('close', () => finish(resolve, response.trim()));

    setTimeout(() => {
      finish(reject, new Error('Keyboard server connection timeout'));
    }, timeoutMs);
  });
}

export async function sendCommand(command: string, args: Record<string, unknown> = {}): Promise<any> {
  const response = await sendToServer({ command, ...args });
  return JSON.parse(response);
}
