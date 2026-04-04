#!/bin/bash
PORT=$(ls /dev/tty.usbmodem* 2>/dev/null | head -1)
if [ -z "$PORT" ]; then
    echo "No Pico serial device found"
    exit 1
fi
echo "Connecting to $PORT at 115200..."
echo "Press Ctrl+C to exit"
echo "---"
python3 -c "
import serial, time, sys
s = serial.Serial('$PORT', 115200, timeout=1)
while True:
    d = s.read(4096)
    if d:
        sys.stdout.buffer.write(d)
        sys.stdout.flush()
"
