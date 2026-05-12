#!/usr/bin/env python3
import serial
import struct
import time

PORT = "/dev/ttyUSB0"   # поменяй на свой порт
BAUD = 115200

PROTO_VERSION = 1
CMD_PING = 0x01


def crc16_ccitt_false(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def make_frame(payload: bytes) -> bytes:
    length = len(payload)
    head = struct.pack("<H", length)
    crc = crc16_ccitt_false(head + payload)
    return head + payload + struct.pack("<H", crc)


def read_frame(ser: serial.Serial):
    head = ser.read(2)
    if len(head) != 2:
        return None

    length = struct.unpack("<H", head)[0]
    payload = ser.read(length)
    crc_bytes = ser.read(2)

    if len(payload) != length or len(crc_bytes) != 2:
        return None

    rx_crc = struct.unpack("<H", crc_bytes)[0]
    calc_crc = crc16_ccitt_false(head + payload)

    return {
        "length": length,
        "payload": payload,
        "rx_crc": rx_crc,
        "calc_crc": calc_crc,
        "crc_ok": rx_crc == calc_crc,
    }


with serial.Serial(PORT, BAUD, timeout=2) as ser:
    time.sleep(0.2)
    ser.reset_input_buffer()

    seq = 1
    payload = struct.pack("<BBH", PROTO_VERSION, CMD_PING, seq)
    frame = make_frame(payload)

    print("TX:", frame.hex(" "))
    ser.write(frame)
    ser.flush()

    while True:
        resp = read_frame(ser)
        if resp is None:
            print("timeout/no frame")
            break

        print("RX:", resp["payload"].hex(" "))
        print("CRC OK:", resp["crc_ok"])

        # если пришёл log frame — пропускаем и ждём ответ на ping
        if len(resp["payload"]) >= 2:
            cmd = resp["payload"][1]
            print("CMD:", hex(cmd))

        break