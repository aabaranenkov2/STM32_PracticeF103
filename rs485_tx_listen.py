#!/usr/bin/env python3

import argparse
import time

import serial


EXPECTED_FRAME = bytes.fromhex("55 aa 81 04 de ad be ef fb 36")


def main() -> int:
    parser = argparse.ArgumentParser(description="Listen for STM32 RS-485 TX test")
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--seconds", type=float, default=6.0)
    args = parser.parse_args()

    received = bytearray()
    deadline = time.monotonic() + args.seconds

    print(f"Прослушивание {args.port}, {args.baud} 8N1...")
    with serial.Serial(args.port, args.baud, timeout=0.1) as connection:
        connection.reset_input_buffer()
        while time.monotonic() < deadline:
            received.extend(connection.read(256))

    frame_count = received.count(EXPECTED_FRAME)
    print(f"Получено байт: {len(received)}")
    print(f"Корректных кадров: {frame_count}")
    if received:
        print(f"Данные: {bytes(received[:80]).hex(' ')}")

    if frame_count == 0:
        print("FAIL: передающий тракт STM32→MAX485→USB не работает.")
        return 1

    print("PASS: передающий тракт RS-485 работает.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
