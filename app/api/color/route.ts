import { NextResponse } from 'next/server';
import { sendCommand } from '@/lib/server-connection';

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const command = body.led !== undefined ? 'setKeyColor' : 'setColor';
    const result = await sendCommand(command, body);
    return NextResponse.json(result);
  } catch (err) {
    return NextResponse.json(
      { error: 'Cannot connect to keyboard server', details: String(err) },
      { status: 503 }
    );
  }
}
