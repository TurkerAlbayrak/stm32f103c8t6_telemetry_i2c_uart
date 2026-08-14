#!/usr/bin/env python3
"""
ground_station.py

STM32 karttan (Blue Pill) gelen ikili telemetri cercevelerini seri port
uzerinden okuyup ayristiran basit bir "yer istasyonu" scripti.

Cerceve formati (telemetry.h ile birebir ayni olmali):
  [0]     SYNC_1        = 0xAA
  [1]     SYNC_2        = 0x55
  [2]     packet_id     (uint8)
  [3]     payload_len   (uint8) -- sizeof(TelemetryPayload_t) = 44 bayt olmali
  [4:48]  payload       (11 x float/uint32, little-endian, packed struct)
  [48]    checksum      (uint8, XOR)

Kurulum:
    pip install pyserial

Kullanim:
    python ground_station.py COM5          (Windows)
    python ground_station.py /dev/ttyUSB0  (Linux/Mac)

PICSimLab ile test icin: PICSimLab'in olusturdugu sanal seri portu
(Spare Parts / IO Virtual Term penceresinde veya PICSimLab ayarlarinda
gorunen COM/pty adini) buraya verin.
"""

import sys
import struct
import time

try:
    import serial
except ImportError:
    print("HATA: pyserial kurulu degil. Kurmak icin: pip install pyserial")
    sys.exit(1)

SYNC_1 = 0xAA
SYNC_2 = 0x55

# telemetry.h'deki TelemetryPayload_t ile BIREBIR ayni sira ve tipte olmali!
# '<' = little-endian, 'f' = float (4 byte), 'I' = uint32_t (4 byte)
PAYLOAD_FORMAT = "<10fI"   # 10 adet float + 1 adet uint32
PAYLOAD_SIZE = struct.calcsize(PAYLOAD_FORMAT)  # beklenen: 44 bayt

FIELD_NAMES = [
    "temperature_C", "pressure_hPa", "altitude_m",
    "accel_x_g", "accel_y_g", "accel_z_g",
    "gyro_x_dps", "gyro_y_dps", "gyro_z_dps",
    "imu_temp_C", "timestamp_ms",
]


def compute_checksum(len_byte: int, payload: bytes) -> int:
    """STM32 tarafindaki Telemetry_Checksum() ile birebir ayni algoritma."""
    chk = len_byte
    for b in payload:
        chk ^= b
    return chk & 0xFF


def read_frame(ser: "serial.Serial"):
    """Senkron baytlarini bulup tek bir cerceveyi okur ve ayristirir.
    Basarili olursa (packet_id, dict_of_values) dondurur, aksi halde None."""

    # 1) SYNC_1 baytini ara
    b = ser.read(1)
    if not b or b[0] != SYNC_1:
        return None

    b2 = ser.read(1)
    if not b2 or b2[0] != SYNC_2:
        return None

    header = ser.read(2)  # packet_id, payload_len
    if len(header) < 2:
        return None
    pkt_id, payload_len = header[0], header[1]

    if payload_len != PAYLOAD_SIZE:
        print(f"UYARI: beklenmeyen payload boyutu ({payload_len}, beklenen {PAYLOAD_SIZE}) - "
              f"struct formati STM32 tarafiyla uyusmuyor olabilir.")
        return None

    payload = ser.read(payload_len)
    if len(payload) < payload_len:
        return None

    chk_byte = ser.read(1)
    if len(chk_byte) < 1:
        return None
    received_chk = chk_byte[0]

    expected_chk = compute_checksum(payload_len, payload)
    if received_chk != expected_chk:
        print(f"UYARI: checksum hatasi! (paket #{pkt_id}, beklenen 0x{expected_chk:02X}, "
              f"gelen 0x{received_chk:02X}) - paket atlandi.")
        return None

    values = struct.unpack(PAYLOAD_FORMAT, payload)
    data = dict(zip(FIELD_NAMES, values))
    return pkt_id, data


def main():
    if len(sys.argv) < 2:
        print("Kullanim: python ground_station.py <PORT> [BAUD]")
        print("Ornek:    python ground_station.py COM5 115200")
        sys.exit(1)

    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print(f"Baglaniliyor: {port} @ {baud} baud ...")
    ser = serial.Serial(port, baud, timeout=1)

    last_id = None
    lost_count = 0
    received_count = 0

    print("Dinleniyor... (Ctrl+C ile cikis)\n")
    try:
        while True:
            result = read_frame(ser)
            if result is None:
                continue

            pkt_id, data = result
            received_count += 1

            if last_id is not None:
                expected_next = (last_id + 1) % 256
                if pkt_id != expected_next:
                    gap = (pkt_id - expected_next) % 256
                    lost_count += gap
                    print(f"  -> {gap} paket kaybolmus olabilir (id atlamasi tespit edildi)")
            last_id = pkt_id

            ts = time.strftime("%H:%M:%S")
            print(f"[{ts}] #{pkt_id:3d} | "
                  f"T={data['temperature_C']:6.2f}C  P={data['pressure_hPa']:8.2f}hPa  "
                  f"Alt={data['altitude_m']:7.1f}m  |  "
                  f"Ax={data['accel_x_g']:5.2f} Ay={data['accel_y_g']:5.2f} Az={data['accel_z_g']:5.2f} g  |  "
                  f"Gx={data['gyro_x_dps']:6.1f} Gy={data['gyro_y_dps']:6.1f} Gz={data['gyro_z_dps']:6.1f} dps  |  "
                  f"IMU_T={data['imu_temp_C']:5.1f}C  t={data['timestamp_ms']}ms")

    except KeyboardInterrupt:
        print(f"\nDurduruldu. Toplam alinan: {received_count}, tahmini kayip: {lost_count}")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
