#!/usr/bin/env python3
import re
import subprocess
import time
import serial
import serial.tools.list_ports

UPDATE_INTERVAL = 0.05  # minimum seconds between osascript calls

def find_port():
    for p in serial.tools.list_ports.comports():
        if 'usbmodem' in p.device or 'usbserial' in p.device:
            return p.device
    return None

def set_volume(vol):
    subprocess.run(
        ['osascript', '-e', f'set volume output volume {vol}'],
        capture_output=True
    )

def main():
    port = find_port()
    if not port:
        print("No serial port found. Is the Pico connected?")
        return

    print(f"Connected: {port}")
    print("Dial controls volume (0-100). Ctrl+C to quit.\n")

    last_volume = -1
    last_update = 0.0

    with serial.Serial(port, 115200, timeout=1) as ser:
        while True:
            line = ser.read_until(b'\r').decode(errors='ignore').strip()

            match = re.search(r'Dial:\s*([-\d]+)', line)
            if not match:
                continue

            dial = int(match.group(1))

            now = time.time()
            if dial != last_volume and (now - last_update) >= UPDATE_INTERVAL:
                set_volume(dial)
                print(f"Volume: {dial:3d}%", end='\r')
                last_volume = dial
                last_update = now

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nDone.")
