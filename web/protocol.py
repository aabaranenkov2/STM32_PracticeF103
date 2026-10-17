"""Binary protocol shared by the lot-sensor gateway and tests."""

from __future__ import annotations

import struct
from dataclasses import dataclass

SYNC = b"\x55\xaa"
VERSION = 1
MAX_PAYLOAD = 64
RESPONSE_FLAG = 0x80

CMD_GET_STATUS = 0x01
CMD_MOVE_UP = 0x10
CMD_MOVE_DOWN = 0x11
CMD_STOP = 0x12
CMD_PARK = 0x13
CMD_MEASURE_ONCE = 0x14
CMD_SET_AUTO = 0x15
CMD_CLEAR_ERROR = 0x16
CMD_SET_MANUAL_PWM = 0x17
CMD_RESET_ENCODERS = 0x18
CMD_MEASURE_AUTO_ONCE = 0x19
CMD_SET_CONFIG = 0x20
CMD_GET_CONFIG = 0x21
CMD_SET_BUNKER_CONFIG = 0x22
CMD_GET_BUNKER_CONFIG = 0x23
CMD_GET_CALIBRATION_CAPTURE = 0x24
CMD_GET_DIAGNOSTICS = 0x25
CMD_MEASURE_AUTO_ONCE_FAST = 0x26

STATE_NAMES = {
    0: "Инициализация",
    1: "Готов",
    2: "Ручной подъём",
    3: "Ручное опускание",
    4: "Парковка",
    5: "Измерение вниз",
    6: "Контакт с продуктом",
    7: "Подъём",
    8: "Ошибка",
    9: "Ожидание отпускания защиты",
    10: "Проверка парковки вниз",
    11: "Возврат на парковку",
    12: "Остановка перед возвратом",
    13: "Ожидание перед измерением",
    14: "Выборка слабины",
    15: "Плавный разгон вверх",
    16: "Ожидание механизмов",
    17: "Остановка перед парковкой",
}

ERROR_NAMES = {
    0: "Превышено время движения",
    1: "Нет импульсов Enc_FA_White/Enc_FB_Green",
    2: "Нет импульсов Mot_FG_Yellow",
    3: "Парковка не найдена",
    4: "Расхождение Enc и Mot_FG",
    5: "Ошибка UART",
    6: "Ошибка конфигурации",
    7: "Переполнение приёмного буфера",
    8: "Груз зажат — работа датчика запрещена",
    9: "Проверка парковки не выполнена",
    10: "Тест Mot_FG: подъём не совпал с Enc",
    11: "Груз не стабилизировался после контакта",
    12: "Подъём не начался",
    13: "Превышен максимальный путь спуска",
}


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def encode_frame(command: int, sequence: int, payload: bytes = b"") -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload is too long")
    body = bytes((VERSION, command & 0xFF, sequence & 0xFF, len(payload))) + payload
    return SYNC + body + struct.pack("<H", crc16_modbus(body))


@dataclass(frozen=True)
class Frame:
    command: int
    sequence: int
    payload: bytes


class FrameParser:
    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> list[Frame]:
        self._buffer.extend(data)
        frames: list[Frame] = []
        while True:
            sync_at = self._buffer.find(SYNC)
            if sync_at < 0:
                self._buffer[:] = self._buffer[-1:] if self._buffer[-1:] == b"\x55" else b""
                break
            if sync_at:
                del self._buffer[:sync_at]
            if len(self._buffer) < 6:
                break
            payload_length = self._buffer[5]
            if self._buffer[2] != VERSION or payload_length > MAX_PAYLOAD:
                del self._buffer[0]
                continue
            frame_length = payload_length + 8
            if len(self._buffer) < frame_length:
                break
            raw = bytes(self._buffer[:frame_length])
            del self._buffer[:frame_length]
            expected_crc = struct.unpack_from("<H", raw, frame_length - 2)[0]
            if crc16_modbus(raw[2:-2]) != expected_crc:
                self._buffer[:0] = raw[1:]
                continue
            frames.append(Frame(raw[3], raw[4], raw[6:-2]))
        return frames


def decode_status(payload: bytes) -> dict:
    if len(payload) != 64 or payload[0] != 0:
        raise ValueError("invalid status response")
    state, errors, flags = payload[1], struct.unpack_from("<H", payload, 2)[0], payload[4]
    error_names = [name for bit, name in ERROR_NAMES.items() if errors & (1 << bit)]
    return {
        "state": state,
        "state_name": STATE_NAMES.get(state, f"Неизвестно ({state})"),
        "errors": errors,
        "error_names": error_names,
        "auto": bool(flags & 0x01),
        "motor_enabled": bool(flags & 0x02),
        "motor_up": bool(flags & 0x04),
        "ground_detect": bool(flags & 0x08),
        "interlock_active": bool(flags & 0x10),
        "sensor_busy": bool(flags & 0x20),
        "encoder": struct.unpack_from("<i", payload, 5)[0],
        "fg_count": struct.unpack_from("<i", payload, 9)[0],
        "pwm": struct.unpack_from("<H", payload, 13)[0],
        "distance": struct.unpack_from("<i", payload, 15)[0],
        "measurement_count": struct.unpack_from("<I", payload, 19)[0],
        "buttons": payload[23],
        "distance_mm": struct.unpack_from("<i", payload, 24)[0] / 10,
        "motor_distance_mm": struct.unpack_from("<i", payload, 28)[0] / 10,
        "encoder_speed_cps": struct.unpack_from("<I", payload, 32)[0],
        "descent_distance_mm": struct.unpack_from("<i", payload, 36)[0] / 10,
        "ascent_distance_mm": struct.unpack_from("<i", payload, 40)[0] / 10,
        "travel_difference_mm": struct.unpack_from("<i", payload, 44)[0] / 10,
        "descent_motor_distance_mm": struct.unpack_from("<i", payload, 48)[0] / 10,
        "saved_descent_mm": struct.unpack_from("<i", payload, 52)[0] / 10,
        "result_mm": struct.unpack_from("<i", payload, 56)[0] / 10,
        "saved_ascent_mm": struct.unpack_from("<i", payload, 60)[0] / 10,
    }


def decode_calibration_capture(payload: bytes) -> dict:
    if len(payload) != 18 or payload[0] != 0:
        raise ValueError("invalid calibration capture response")
    flags = payload[1]
    return {
        "down_valid": bool(flags & 0x01),
        "up_valid": bool(flags & 0x02),
        "raw_down_counts": struct.unpack_from("<i", payload, 2)[0],
        "raw_up_counts": struct.unpack_from("<i", payload, 6)[0],
        "raw_down_fg_count": struct.unpack_from("<i", payload, 10)[0],
        "raw_up_fg_count": struct.unpack_from("<i", payload, 14)[0],
    }

def decode_diagnostics(payload: bytes) -> dict:
    if len(payload) != 29 or payload[0] != 0:
        raise ValueError("invalid diagnostics response")
    values = struct.unpack_from("<IIIIIIHH", payload, 1)
    keys = (
        "sequence",
        "cycle_ms",
        "measurement_ms",
        "parking_ms",
        "descent_ms",
        "ascent_ms",
        "descent_retries",
        "park_retries",
    )
    return dict(zip(keys, values))


NOMINAL_SHAFT_DIAMETER_MM = 10.0
MEASURE_MM_PER_PULSE = 31.416 / 2400.0
MOTOR_PULSES_PER_REV = 9
MOTOR_CIRCUMFERENCE_UM = 125664
MOTOR_GEAR_RATIO_X10 = 188
LENGTH_CAL_MIN = 1000
LENGTH_CAL_MAX = 200000
LENGTH_CAL_OFFSET_MIN = -500000
LENGTH_CAL_OFFSET_MAX = 500000
MIN_CALIBRATION_TRAVEL_MM = 10.0


def decode_config(payload: bytes) -> dict:
    if len(payload) != 63 or payload[0] != 0:
        raise ValueError("invalid config response")
    values = struct.unpack_from("<HHHHHHHHHHHHIIIiHHHhHIIHh", payload, 1)
    keys = (
        "run_pwm",
        "park_pwm",
        "contact_timeout_ms",
        "stall_timeout_ms",
        "mismatch_mm",
        "park_release_ms",
        "park_test_ms",
        "park_test_pulses",
        "contact_speed_percent",
        "contact_ignore_ms",
        "slowdown_remaining_percent",
        "slowdown_divisor_tenths",
        "auto_interval_ms",
        "length_cal_x10000",
        "motor_cal_x10000",
        "length_cal_offset_tenth_mm",
        "brake_comp_mm",
        "load_height_mm",
        "motor_encoder_enabled",
        "descent_correction_mm",
        "error_measurement_limit",
        "motion_timeout_ms",
        "max_descent_mm",
        "takeup_pwm",
        "ascent_correction_mm",
    )
    return dict(zip(keys, values))


def encode_config(config: dict) -> bytes:
    return struct.pack(
        "<HHHHHHHHHHHHIIIiHHHhHIIHh",
        int(config["run_pwm"]),
        int(config["park_pwm"]),
        int(config["contact_timeout_ms"]),
        int(config["stall_timeout_ms"]),
        int(config.get("mismatch_mm", config.get("mismatch_pulses", 100))),
        int(config["park_release_ms"]),
        int(config["park_test_ms"]),
        int(config["park_test_pulses"]),
        int(config["contact_speed_percent"]),
        int(config.get("contact_ignore_ms", 1000)),
        int(config["slowdown_remaining_percent"]),
        int(config["slowdown_divisor_tenths"]),
        int(config["auto_interval_ms"]),
        int(config["length_cal_x10000"]),
        int(config["motor_cal_x10000"]),
        int(config.get("length_cal_offset_tenth_mm", 0)),
        int(config.get("brake_comp_mm", 0)),
        int(config.get("load_height_mm", 110)),
        1 if int(config.get("motor_encoder_enabled", 0)) else 0,
        int(config.get("descent_correction_mm", 0)),
        int(config.get("error_measurement_limit", 2)),
        int(config.get("motion_timeout_ms", 60000)),
        int(config.get("max_descent_mm", 1000)),
        int(config.get("takeup_pwm", 945)),
        int(config.get("ascent_correction_mm", 0)),
    )


def decode_bunker_config(payload: bytes) -> dict:
    if len(payload) != 45 or payload[0] != 0:
        raise ValueError("invalid bunker config response")
    values = struct.unpack_from("<HHIIIIIIIIII", payload, 1)
    keys = (
        "max_fill_x10",
        "target_fill_x10",
        "model_stroke_mm",
        "upper_height_mm",
        "upper_top_diameter_mm",
        "bunker_diameter_mm",
        "cylinder_height_mm",
        "lower_height_mm",
        "lower_bottom_diameter_mm",
        "load_offset_mm",
        "density_x1000",
        "product_factor_x1000",
    )
    return dict(zip(keys, values))


def encode_bunker_config(config: dict) -> bytes:
    return struct.pack(
        "<HHIIIIIIIIII",
        int(config["max_fill_x10"]),
        int(config["target_fill_x10"]),
        int(config["model_stroke_mm"]),
        int(config["upper_height_mm"]),
        int(config["upper_top_diameter_mm"]),
        int(config["bunker_diameter_mm"]),
        int(config["cylinder_height_mm"]),
        int(config["lower_height_mm"]),
        int(config["lower_bottom_diameter_mm"]),
        int(config["load_offset_mm"]),
        int(config["density_x1000"]),
        int(config["product_factor_x1000"]),
    )


def length_cal_factor(config: dict) -> float:
    return int(config["length_cal_x10000"]) / 10000.0


def motor_cal_factor(config: dict) -> float:
    return int(config["motor_cal_x10000"]) / 10000.0


def length_cal_offset_mm(config: dict) -> float:
    return int(config.get("length_cal_offset_tenth_mm", 0)) / 10.0


def enc_nominal_mm(encoder_counts: int) -> float:
    return float(encoder_counts) * MEASURE_MM_PER_PULSE


def clamp_length_cal_offset_tenth_mm(value: float) -> int:
    scaled = int(round(value * 10.0))
    return max(LENGTH_CAL_OFFSET_MIN, min(LENGTH_CAL_OFFSET_MAX, scaled))


def calc_linear_length_cal(
    enc1: int,
    mm1: float,
    enc2: int,
    mm2: float,
) -> tuple[int, int]:
    nom1 = enc_nominal_mm(enc1)
    nom2 = enc_nominal_mm(enc2)
    delta_nom = nom2 - nom1
    if abs(delta_nom) < MIN_CALIBRATION_TRAVEL_MM:
        raise ValueError(
            f"точки должны быть разнесены не меньше "
            f"{MIN_CALIBRATION_TRAVEL_MM:.0f} мм по Enc"
        )
    slope = (float(mm2) - float(mm1)) / delta_nom
    offset_mm = float(mm1) - slope * nom1
    return (
        clamp_length_cal_x10000(slope),
        clamp_length_cal_offset_tenth_mm(offset_mm),
    )


def apply_linear_length_cal(
    encoder_counts: int,
    length_cal_x10000: int,
    length_cal_offset_tenth_mm: int,
) -> float:
    tenth_mm = (
        encoder_counts * 31416.0 / (2400.0 * 100.0) *
        length_cal_x10000 / 10000.0 +
        length_cal_offset_tenth_mm
    )
    return tenth_mm / 10.0


def fg_raw_mm(fg_count: int) -> float:
    magnitude = abs(int(fg_count))
    tenth_mm = (
        magnitude * MOTOR_CIRCUMFERENCE_UM /
        (MOTOR_PULSES_PER_REV * MOTOR_GEAR_RATIO_X10 * 10)
    )
    mm = tenth_mm / 10.0
    return -mm if fg_count < 0 else mm


def clamp_length_cal_x10000(value: float) -> int:
    scaled = int(round(value * 10000.0))
    return max(LENGTH_CAL_MIN, min(LENGTH_CAL_MAX, scaled))


def calc_manual_length_cal_x10000(reference_mm: float, encoder_counts: int) -> int:
    raw_mm = abs(int(encoder_counts)) * MEASURE_MM_PER_PULSE
    if raw_mm < 0.01:
        raise ValueError("энкодер не показывает ход — опустите груз на грунт")
    return clamp_length_cal_x10000(float(reference_mm) / raw_mm)


def calc_auto_length_cal_x10000(shaft_diameter_mm: float) -> int:
    if shaft_diameter_mm <= 0.0:
        raise ValueError("диаметр вала должен быть больше 0")
    return clamp_length_cal_x10000(
        float(shaft_diameter_mm) / NOMINAL_SHAFT_DIAMETER_MM
    )


def calc_motor_cal_x10000(reference_mm: float, fg_count: int) -> int:
    raw_mm = fg_raw_mm(fg_count)
    if abs(reference_mm) < MIN_CALIBRATION_TRAVEL_MM:
        raise ValueError(
            f"эталонный Enc должен пройти не меньше {MIN_CALIBRATION_TRAVEL_MM:.0f} мм"
        )
    if abs(raw_mm) < MIN_CALIBRATION_TRAVEL_MM:
        raise ValueError(
            f"Mot_FG должен пройти не меньше {MIN_CALIBRATION_TRAVEL_MM:.0f} мм"
        )
    if reference_mm * raw_mm <= 0.0:
        raise ValueError("Enc и Mot_FG должны двигаться в одну сторону")
    return clamp_length_cal_x10000(reference_mm / raw_mm)
