#!/usr/bin/env python3

import argparse
import sys
import time

import serial


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def make_request(payload: bytes) -> bytes:
    frame = bytes((0x55, 0xAA, 0x01, len(payload))) + payload
    crc = crc16_modbus(frame)
    return frame + crc.to_bytes(2, "little")


def main() -> int:
    parser = argparse.ArgumentParser(description="STM32 RS-485 request/response test")
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--count", type=int, default=20)
    args = parser.parse_args()

    payload = bytes((0xDE, 0xAD, 0xBE, 0xEF))
    request = make_request(payload)
    expected = make_request(payload)
    expected = expected[:2] + bytes((0x81,)) + expected[3:]
    expected = expected[:8] + crc16_modbus(expected[:8]).to_bytes(2, "little")
    passed = 0

    print(f"Порт: {args.port}, {args.baud} 8N1")
    print(f"Запросов: {args.count}")

    with serial.Serial(args.port, args.baud, timeout=0.5) as connection:
        connection.reset_input_buffer()
        for number in range(1, args.count + 1):
            connection.write(request)
            connection.flush()
            response = connection.read(10)
            if response == expected:
                passed += 1
                print(f"{number:02d}: PASS  {response.hex(' ')}")
            else:
                print(
                    f"{number:02d}: FAIL  получено "
                    f"{response.hex(' ') if response else '<нет ответа>'}"
                )
            time.sleep(0.05)

    print(f"Успешно: {passed}/{args.count}")
    if passed != args.count:
        print("FAIL: проверьте A/B, GND, PA9/PA10 и PB0.", file=sys.stderr)
        return 1
    print("PASS: приём, передача, CRC и управление направлением работают.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
