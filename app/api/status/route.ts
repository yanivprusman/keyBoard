import { NextResponse } from 'next/server';
import { sendCommand } from '@/lib/server-connection';

export async function GET() {
  try {
    const result = await sendCommand('getStatus');
    return NextResponse.json(result);
  } catch (err) {
    return NextResponse.json(
      { error: 'Cannot connect to keyboard server', details: String(err) },
      { status: 503 }
    );
  }
}
