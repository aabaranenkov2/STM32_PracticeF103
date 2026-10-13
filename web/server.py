#!/usr/bin/env python3
"""Local HTTP/RS-485 gateway for the STM32 lot sensor."""

from __future__ import annotations

import argparse
import json
import math
import os
from datetime import datetime
import queue
import sqlite3
import sys
import threading
import time
import webbrowser
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.error import URLError
from urllib.request import urlopen

if os.name == "nt":
    import msvcrt
else:
    import fcntl

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

from protocol import (
    CMD_CLEAR_ERROR,
    CMD_GET_BUNKER_CONFIG,
    CMD_GET_CALIBRATION_CAPTURE,
    CMD_GET_CONFIG,
    CMD_GET_STATUS,
    CMD_MEASURE_AUTO_ONCE,
    CMD_MOVE_DOWN,
    CMD_MOVE_UP,
    CMD_PARK,
    calc_auto_length_cal_x10000,
    calc_linear_length_cal,
    calc_manual_length_cal_x10000,
    calc_motor_cal_x10000,
    length_cal_factor,
    length_cal_offset_mm,
    motor_cal_factor,
    CMD_SET_AUTO,
    CMD_SET_BUNKER_CONFIG,
    CMD_SET_CONFIG,
    CMD_RESET_ENCODERS,
    CMD_SET_MANUAL_PWM,
    CMD_STOP,
    RESPONSE_FLAG,
    FrameParser,
    decode_config,
    decode_bunker_config,
    decode_calibration_capture,
    decode_status,
    encode_bunker_config,
    encode_config,
    encode_frame,
)
from simulator import VirtualController

CH340_IDS = {(0x1A86, 0x7523), (0x1A86, 0x5523)}
MANUAL_PWM_MIN_PERMILLE = 450
MANUAL_PWM_MAX_PERMILLE = 1000
COMMANDS = {
    "up": CMD_MOVE_UP,
    "down": CMD_MOVE_DOWN,
    "stop": CMD_STOP,
    "park": CMD_PARK,
    "measure_auto": CMD_MEASURE_AUTO_ONCE,
    "clear_error": CMD_CLEAR_ERROR,
    "reset_encoders": CMD_RESET_ENCODERS,
}

REPORT_DATE_WIDTH = 22
REPORT_VALUE_WIDTH = 24
REPORT_SEPARATOR = "  |  "


def resource_directory() -> Path:
    return Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent))


def application_version() -> str:
    version_path = resource_directory() / "VERSION"
    try:
        return version_path.read_text(encoding="utf-8").strip() or "dev"
    except OSError:
        project_version = Path(__file__).resolve().parent.parent / "VERSION"
        try:
            return project_version.read_text(encoding="utf-8").strip() or "dev"
        except OSError:
            return "dev"


def _frustum_partial_volume(
    height_m: float,
    full_height_m: float,
    bottom_radius_m: float,
    top_radius_m: float,
) -> float:
    if height_m <= 0.0 or full_height_m <= 0.0:
        return 0.0
    height = min(height_m, full_height_m)
    slope = (top_radius_m - bottom_radius_m) / full_height_m
    return math.pi * (
        bottom_radius_m * bottom_radius_m * height
        + bottom_radius_m * slope * height * height
        + slope * slope * height * height * height / 3.0
    )


def measurement_mass_tonnes(result_mm: float, config: dict) -> float:
    lower_height = float(config["lower_height_mm"]) / 1000.0
    cylinder_height = float(config["cylinder_height_mm"]) / 1000.0
    upper_height = float(config["upper_height_mm"]) / 1000.0
    total_height = lower_height + cylinder_height + upper_height
    fill_height = max(0.0, min(total_height, float(result_mm) / 1000.0))

    main_radius = float(config["bunker_diameter_mm"]) / 2000.0
    lower_radius = float(config["lower_bottom_diameter_mm"]) / 2000.0
    upper_radius = float(config["upper_top_diameter_mm"]) / 2000.0
    remaining = fill_height
    volume = _frustum_partial_volume(
        remaining, lower_height, lower_radius, main_radius
    )
    remaining = max(0.0, remaining - lower_height)
    cylinder_part = min(remaining, cylinder_height)
    volume += math.pi * main_radius * main_radius * cylinder_part
    remaining = max(0.0, remaining - cylinder_height)
    volume += _frustum_partial_volume(
        remaining, upper_height, main_radius, upper_radius
    )
    density = float(config["density_x1000"]) / 1000.0
    product_factor = float(config["product_factor_x1000"]) / 1000.0
    return max(0.0, volume * density * product_factor)


def report_header() -> str:
    return (
        f"{'Дата и время':<{REPORT_DATE_WIDTH}}{REPORT_SEPARATOR}"
        f"{'Результат, мм':>{REPORT_VALUE_WIDTH}}{REPORT_SEPARATOR}"
        f"{'Результат, тонн':>{REPORT_VALUE_WIDTH}}\n"
    )


def report_row(created: datetime, result_mm: float, tonnes: float) -> str:
    created_text = created.strftime("%Y-%m-%d %H:%M:%S")
    return (
        f"{created_text:<{REPORT_DATE_WIDTH}}{REPORT_SEPARATOR}"
        f"{result_mm:>{REPORT_VALUE_WIDTH}.1f}{REPORT_SEPARATOR}"
        f"{tonnes:>{REPORT_VALUE_WIDTH}.1f}\n"
    )


def data_directory() -> Path:
    if os.name == "nt":
        root = Path(os.environ.get("LOCALAPPDATA", Path.home()))
    else:
        root = Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share"))
    path = root / "LotSensor"
    path.mkdir(parents=True, exist_ok=True)
    return path


def reports_directory() -> Path:
    if os.name == "nt":
        documents = Path.home() / "Documents"
        path = documents / "Лотовый датчик" / "отчеты"
    else:
        path = Path(__file__).resolve().parent.parent / "отчеты"
    path.mkdir(parents=True, exist_ok=True)
    return path


def simulation_reports_directory() -> Path:
    path = reports_directory() / "симулятор"
    path.mkdir(parents=True, exist_ok=True)
    return path


def _startup_log(message: str) -> None:
    try:
        path = data_directory() / "startup.log"
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"{datetime.now():%Y-%m-%d %H:%M:%S} {message}\n")
    except OSError:
        pass


def _open_browser_when_ready(
    url: str,
    attempts: int = 20,
    delay_seconds: float = 0.25,
) -> bool:
    version_url = f"{url.rstrip('/')}/api/version"
    last_error = "локальный сервер не ответил"

    for attempt in range(max(1, attempts)):
        try:
            with urlopen(version_url, timeout=1.0) as response:
                payload = json.loads(response.read().decode("utf-8"))
                if response.status != HTTPStatus.OK or not payload.get("version"):
                    raise ValueError("API версии вернул некорректный ответ")

            with urlopen(url, timeout=2.0) as response:
                html = response.read()
                if (response.status != HTTPStatus.OK or
                        b"<!doctype html>" not in html.lower()):
                    raise ValueError("главная HTML-страница не готова")

            if webbrowser.open(url):
                _startup_log(
                    f"Страница открыта после попытки {attempt + 1}"
                )
                return True
            last_error = "Windows не приняла команду открытия браузера"
        except (OSError, URLError, UnicodeError, ValueError) as exc:
            last_error = str(exc)

        if attempt + 1 < attempts:
            time.sleep(max(0.0, delay_seconds))

    _startup_log(
        f"Не удалось открыть страницу за {max(1, attempts)} попыток: "
        f"{last_error}"
    )
    return False


class EventLog:
    def __init__(self, path: Path) -> None:
        self._lock = threading.Lock()
        self._database = sqlite3.connect(path, check_same_thread=False)
        self._database.execute(
            "CREATE TABLE IF NOT EXISTS events("
            "id INTEGER PRIMARY KEY, created TEXT DEFAULT CURRENT_TIMESTAMP,"
            "kind TEXT NOT NULL, message TEXT NOT NULL, data TEXT)"
        )
        self._database.commit()

    def add(self, kind: str, message: str, data: Any = None) -> None:
        with self._lock:
            self._database.execute(
                "INSERT INTO events(kind,message,data) VALUES(?,?,?)",
                (kind, message, None if data is None else json.dumps(data, ensure_ascii=False)),
            )
            self._database.commit()

    def recent(self, limit: int = 50) -> list[dict]:
        with self._lock:
            rows = self._database.execute(
                "SELECT created,kind,message,data FROM events ORDER BY id DESC LIMIT ?",
                (max(1, min(limit, 200)),),
            ).fetchall()
        return [
            {"created": row[0], "kind": row[1], "message": row[2],
             "data": json.loads(row[3]) if row[3] else None}
            for row in rows
        ]


@dataclass
class Request:
    command: int
    payload: bytes = b""
    done: threading.Event = field(default_factory=threading.Event)
    response: bytes | None = None
    error: str | None = None


class SerialGateway:
    def __init__(self, event_log: EventLog, port: str | None, simulate: bool) -> None:
        self._log = event_log
        self._requested_port = port
        self._simulate = simulate
        self._requests: queue.Queue[Request] = queue.Queue()
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._sequence = 0
        self._manual_pwm = 550
        self._serial = None
        self._parser = FrameParser()
        self._last_errors = 0
        self._uart_glitch_visible = False
        self._uart_glitch_logged_at = 0.0
        self._uart_glitch_suppressed = 0
        self._poll_failures = 0
        self._last_reported_measurement_count: int | None = None
        self._status: dict[str, Any] = {
            "connected": simulate,
            "simulated": simulate,
            "port": "SIMULATOR" if simulate else None,
            "state": 1,
            "state_name": "Готов" if simulate else "Нет связи",
            "errors": 0,
            "error_names": [],
            "auto": False,
            "motor_enabled": False,
            "motor_up": False,
            "ground_detect": False,
            "interlock_active": False,
            "sensor_busy": False,
            "encoder": 0,
            "fg_count": 0,
            "pwm": 0,
            "distance": 0,
            "distance_mm": 0.0,
            "motor_distance_mm": 0.0,
            "encoder_speed_cps": 0,
            "descent_distance_mm": 0.0,
            "descent_motor_distance_mm": 0.0,
            "ascent_distance_mm": 0.0,
            "travel_difference_mm": 0.0,
            "saved_descent_mm": 0.0,
            "saved_ascent_mm": 0.0,
            "result_mm": 0.0,
            "measurement_count": 0,
            "buttons": 0,
            "updated": 0.0,
        }
        self._config = {
            "run_pwm": 550,
            "park_pwm": 450,
            "contact_timeout_ms": 450,
            "stall_timeout_ms": 700,
            "mismatch_mm": 100,
            "park_release_ms": 3000,
            "park_test_ms": 1500,
            "park_test_pulses": 4,
            "contact_speed_percent": 80,
            "contact_ignore_ms": 1000,
            "slowdown_remaining_percent": 10,
            "slowdown_divisor_tenths": 20,
            "auto_interval_ms": 60000,
            "length_cal_x10000": 10000,
            "motor_cal_x10000": 10000,
            "length_cal_offset_tenth_mm": 0,
            "brake_comp_mm": 0,
            "load_height_mm": 110,
            "motor_encoder_enabled": 0,
            "descent_correction_mm": 0,
            "error_measurement_limit": 2,
            "motion_timeout_ms": 60000,
            "max_descent_mm": 1000,
            "takeup_pwm": 945,
            "ascent_correction_mm": -7,
        }
        self._bunker_config = {
            "max_fill_x10": 889,
            "target_fill_x10": 678,
            "model_stroke_mm": 570,
            "upper_height_mm": 1500,
            "upper_top_diameter_mm": 300,
            "bunker_diameter_mm": 5300,
            "cylinder_height_mm": 10000,
            "lower_height_mm": 2000,
            "lower_bottom_diameter_mm": 1000,
            "load_offset_mm": 200,
            "density_x1000": 750,
            "product_factor_x1000": 1000,
        }
        self._real_config = dict(self._config)
        self._real_bunker_config = dict(self._bunker_config)
        self._simulator: VirtualController | None = None
        if simulate:
            self._simulator = VirtualController(
                self._config, self._bunker_config
            )
            self._status = self._simulator.status
            self._config = self._simulator.config
            self._bunker_config = self._simulator.bunker_config
            self._last_reported_measurement_count = 0

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=2)
        self._disconnect()

    def status(self) -> dict:
        with self._lock:
            result = dict(self._status)
        result["age_ms"] = max(0, int((time.time() - result["updated"]) * 1000))
        return result

    def config(self) -> dict:
        with self._lock:
            return dict(self._config)

    def bunker_config(self) -> dict:
        with self._lock:
            return dict(self._bunker_config)

    def set_simulation(self, enabled: bool) -> dict:
        with self._lock:
            unchanged = enabled == self._simulate
            controller_connected = enabled and self._serial is not None
        if unchanged:
            return self.status()

        if controller_connected:
            try:
                self.execute(CMD_STOP)
                self.execute(CMD_PARK)
            except (RuntimeError, TimeoutError) as exc:
                self._log.add(
                    "simulation",
                    f"Контроллер не подтвердил парковку: {exc}",
                )

        with self._lock:
            if enabled:
                self._real_config = dict(self._config)
                self._real_bunker_config = dict(self._bunker_config)
                self._simulator = VirtualController(
                    self._real_config, self._real_bunker_config
                )
                self._simulate = True
                self._status = self._simulator.status
                self._config = self._simulator.config
                self._bunker_config = self._simulator.bunker_config
                self._last_reported_measurement_count = 0
                self._log.add("simulation", "Режим симулятора включён")
            else:
                self._simulate = False
                self._simulator = None
                self._config = dict(self._real_config)
                self._bunker_config = dict(self._real_bunker_config)
                self._status = {
                    **self._status,
                    "connected": self._serial is not None,
                    "simulated": False,
                    "port": (
                        self._serial.port
                        if self._serial is not None else self._requested_port
                    ),
                    "state_name": "Возврат к контроллеру",
                    "motor_enabled": False,
                    "pwm": 0,
                    "updated": time.time(),
                }
                self._last_reported_measurement_count = None
                self._log.add("simulation", "Режим симулятора выключен")
            return dict(self._status)

    def execute(self, command: int, payload: bytes = b"", timeout: float = 3.0) -> bytes:
        request = Request(command, payload)
        self._requests.put(request)
        if not request.done.wait(timeout):
            raise TimeoutError("контроллер не ответил")
        if request.error:
            raise RuntimeError(request.error)
        return request.response or b""

    def _find_port(self) -> str | None:
        if self._requested_port:
            return self._requested_port
        if list_ports is None:
            return None
        ports = list(list_ports.comports())
        for item in ports:
            if (item.vid, item.pid) in CH340_IDS:
                return item.device
        return ports[0].device if len(ports) == 1 else None

    def _connect(self) -> bool:
        if serial is None:
            with self._lock:
                self._status["state_name"] = "Установите pyserial"
            return False
        port = self._find_port()
        if not port:
            return False
        try:
            self._serial = serial.Serial(port, 115200, timeout=0.02, write_timeout=0.5)
            self._serial.reset_input_buffer()
            self._parser = FrameParser()
            config_payload = self._transaction(CMD_GET_CONFIG)
            bunker_payload = self._transaction(CMD_GET_BUNKER_CONFIG)
            with self._lock:
                self._status.update(connected=True, port=port)
                self._config = decode_config(config_payload)
                self._bunker_config = decode_bunker_config(bunker_payload)
                self._real_config = dict(self._config)
                self._real_bunker_config = dict(self._bunker_config)
            self._poll_failures = 0
            self._log.add("connection", f"Подключён {port}")
            return True
        except (OSError, serial.SerialException, RuntimeError,
                TimeoutError, ValueError) as exc:
            with self._lock:
                self._status.update(connected=False, port=port, state_name=str(exc))
            return False

    def _disconnect(self) -> None:
        if self._serial is not None:
            try:
                self._serial.close()
            except (OSError, serial.SerialException):
                pass
        self._serial = None
        self._poll_failures = 0
        with self._lock:
            self._status.update(connected=False, motor_enabled=False, pwm=0)

    def _note_poll_failure(self, exc: Exception) -> None:
        self._poll_failures += 1
        if self._poll_failures < 5:
            return
        self._poll_failures = 0
        self._log.add("connection", f"Связь потеряна: {exc}")
        self._disconnect()

    def _transaction(self, command: int, payload: bytes = b"") -> bytes:
        if self._serial is None:
            raise RuntimeError("USB–RS-485 не подключён")
        self._sequence = (self._sequence + 1) & 0xFF
        sequence = self._sequence
        request_frame = encode_frame(command, sequence, payload)
        for _ in range(3):
            self._serial.write(request_frame)
            self._serial.flush()
            deadline = time.monotonic() + 0.7
            while time.monotonic() < deadline:
                incoming = self._serial.read(self._serial.in_waiting or 1)
                for frame in self._parser.feed(incoming):
                    if (frame.command == (command | RESPONSE_FLAG) and
                            frame.sequence == sequence):
                        return frame.payload
        raise TimeoutError("нет корректного ответа после трёх попыток")

    def _simulate_request(self, request: Request) -> None:
        with self._lock:
            if self._simulator is None:
                request.error = "симулятор не запущен"
                request.done.set()
                return
            request.response = self._simulator.handle(
                request.command, request.payload
            )
            if request.response and request.response[0] != 0:
                request.error = (
                    f"виртуальный контроллер вернул код "
                    f"{request.response[0]}"
                )
            self._status = self._simulator.status
            self._config = self._simulator.config
            self._bunker_config = self._simulator.bunker_config
            if request.command == CMD_CLEAR_ERROR:
                self._last_reported_measurement_count = 0
        request.done.set()

    def _record_uart_glitch(self, status: dict) -> None:
        uart_bit = 1 << 5
        visible = bool(status["errors"] & uart_bit)
        if visible and not self._uart_glitch_visible:
            now = time.monotonic()
            if now - self._uart_glitch_logged_at >= 2.0:
                count = self._uart_glitch_suppressed + 1
                self._uart_glitch_suppressed = 0
                self._uart_glitch_logged_at = now
                message = (
                    "Сбой кадра RS-485, измерение продолжено"
                    if count == 1 else
                    f"Сбои кадров RS-485: {count}, измерение продолжено"
                )
                self._log.add("uart", message)
            else:
                self._uart_glitch_suppressed += 1
        elif (not visible and self._uart_glitch_visible and
              self._uart_glitch_suppressed):
            count = self._uart_glitch_suppressed
            self._uart_glitch_suppressed = 0
            self._log.add(
                "uart",
                f"Сбои кадров RS-485: {count}, измерение продолжено"
            )
        self._uart_glitch_visible = visible
        if visible:
            status["errors"] &= ~uart_bit
            status["error_names"] = [
                name for name in status["error_names"]
                if name != "Ошибка UART"
            ]

    def _update_status(self) -> None:
        payload = self._transaction(CMD_GET_STATUS)
        status = decode_status(payload)
        status.update(connected=True, simulated=False,
                      port=self._serial.port, updated=time.time())
        self._record_uart_glitch(status)
        with self._lock:
            if self._simulate:
                return
            self._status = status
        if status["errors"] != self._last_errors:
            self._last_errors = status["errors"]
            if status["errors"]:
                self._log.add("error", ", ".join(status["error_names"]), status)
        self._report_completed_measurement(status)

    def _report_completed_measurement(
        self, status: dict, *, simulated: bool = False
    ) -> None:
        count = int(status["measurement_count"])
        previous = self._last_reported_measurement_count
        if previous is None or count < previous:
            self._last_reported_measurement_count = count
            return
        if count == previous:
            return

        with self._lock:
            bunker_config = dict(self._bunker_config)
        result_mm = float(status["result_mm"])
        tonnes = measurement_mass_tonnes(result_mm, bunker_config)
        now = datetime.now()
        report_dir = (
            simulation_reports_directory()
            if simulated else reports_directory()
        )
        report_path = report_dir / f"{now:%Y-%m-%d}.csv"
        new_file = not report_path.exists() or report_path.stat().st_size == 0

        with report_path.open("a", encoding="utf-8", newline="") as stream:
            if new_file:
                stream.write(report_header())
            stream.write(report_row(now, result_mm, tonnes))

        self._last_reported_measurement_count = count
        self._log.add(
            "report",
            (
                f"Учебный замер {count} записан: "
                f"отчеты/симулятор/{report_path.name}"
                if simulated else
                f"Замер {count} записан: отчеты/{report_path.name}"
            ),
            status,
        )

    def _run(self) -> None:
        next_poll = 0.0
        next_config_poll = 0.0
        reconnect_at = 0.0
        while not self._stop.is_set():
            try:
                request = self._requests.get(timeout=0.02)
            except queue.Empty:
                request = None

            if self._simulate:
                if request:
                    self._simulate_request(request)
                completed_status = None
                with self._lock:
                    if self._simulator is not None:
                        if self._simulator.tick(time.monotonic()):
                            completed_status = dict(self._simulator.status)
                        self._status = self._simulator.status
                        self._config = self._simulator.config
                        self._bunker_config = self._simulator.bunker_config
                if completed_status is not None:
                    self._report_completed_measurement(
                        completed_status, simulated=True
                    )
                continue

            if self._serial is None and time.monotonic() >= reconnect_at:
                if not self._connect():
                    reconnect_at = time.monotonic() + 2.0

            if request:
                try:
                    request.response = self._transaction(request.command, request.payload)
                    if request.response and request.response[0] != 0:
                        request.error = f"контроллер вернул код {request.response[0]}"
                    else:
                        self._log.add("command", f"Команда 0x{request.command:02X}")
                except (OSError, RuntimeError, TimeoutError) as exc:
                    request.error = str(exc)
                    self._note_poll_failure(exc)
                finally:
                    request.done.set()

            if self._serial is not None and time.monotonic() >= next_poll:
                try:
                    self._update_status()
                    if time.monotonic() >= next_config_poll:
                        config_payload = self._transaction(CMD_GET_CONFIG)
                        bunker_payload = self._transaction(
                            CMD_GET_BUNKER_CONFIG
                        )
                        with self._lock:
                            if not self._simulate:
                                self._config = decode_config(config_payload)
                                self._bunker_config = decode_bunker_config(
                                    bunker_payload
                                )
                                self._real_config = dict(self._config)
                                self._real_bunker_config = dict(
                                    self._bunker_config
                                )
                        next_config_poll = time.monotonic() + 10.0
                except (OSError, RuntimeError, TimeoutError, ValueError) as exc:
                    self._note_poll_failure(exc)
                else:
                    self._poll_failures = 0
                next_poll = time.monotonic() + 0.04


class ApiHandler(SimpleHTTPRequestHandler):
    gateway: SerialGateway
    event_log: EventLog
    static_directory: str

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, directory=self.static_directory, **kwargs)

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def _json(self, data: Any, status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _body(self) -> dict:
        length = int(self.headers.get("Content-Length", "0"))
        if length > 65536:
            raise ValueError("слишком большой запрос")
        return json.loads(self.rfile.read(length) or b"{}")

    def do_GET(self) -> None:
        if self.path == "/api/status":
            self._json(self.gateway.status())
        elif self.path == "/api/version":
            self._json({"version": application_version()})
        elif self.path == "/api/config":
            self._json(self.gateway.config())
        elif self.path == "/api/bunker-config":
            self._json(self.gateway.bunker_config())
        elif self.path == "/api/calibration-capture":
            try:
                payload = self.gateway.execute(CMD_GET_CALIBRATION_CAPTURE)
                self._json(decode_calibration_capture(payload))
            except (OSError, RuntimeError, TimeoutError, ValueError) as exc:
                self._json({"ok": False, "error": str(exc)},
                           HTTPStatus.SERVICE_UNAVAILABLE)
        elif self.path.startswith("/api/events"):
            self._json(self.event_log.recent())
        else:
            super().do_GET()

    def do_POST(self) -> None:
        try:
            body = self._body()
            if self.path == "/api/simulation":
                simulation_status = self.gateway.set_simulation(
                    bool(body.get("enabled"))
                )
                self._json({"ok": True, "status": simulation_status})
            elif self.path == "/api/command":
                action = body.get("action")
                if action == "auto":
                    payload = bytes((1 if body.get("enabled") else 0,))
                    response = self.gateway.execute(CMD_SET_AUTO, payload)
                elif action == "manual_pwm":
                    percent = int(body.get("value", -1))
                    if not 0 <= percent <= 100:
                        raise ValueError("PWM должен быть от 0 до 100 %")
                    pwm = MANUAL_PWM_MIN_PERMILLE + (
                        percent *
                        (MANUAL_PWM_MAX_PERMILLE - MANUAL_PWM_MIN_PERMILLE) +
                        50
                    ) // 100
                    response = self.gateway.execute(
                        CMD_SET_MANUAL_PWM, pwm.to_bytes(2, "little")
                    )
                elif action in COMMANDS:
                    response = self.gateway.execute(COMMANDS[action])
                else:
                    raise ValueError("неизвестная команда")
                self._json({"ok": True, "result": response.hex()})
            elif self.path == "/api/config":
                payload = encode_config(body)
                response = self.gateway.execute(CMD_SET_CONFIG, payload)
                if response and response[0] != 0:
                    raise ValueError(f"контроллер отклонил настройки: код {response[0]}")
                with self.gateway._lock:
                    self.gateway._config = decode_config(b"\x00" + payload)
                self._json({"ok": True, "result": response.hex() if response else ""})
            elif self.path == "/api/bunker-config":
                payload = encode_bunker_config(body)
                response = self.gateway.execute(CMD_SET_BUNKER_CONFIG, payload)
                if response and response[0] != 0:
                    raise ValueError(
                        f"контроллер отклонил настройки бункера: код {response[0]}"
                    )
                with self.gateway._lock:
                    self.gateway._bunker_config = decode_bunker_config(
                        b"\x00" + payload
                    )
                self._json({
                    "ok": True,
                    "result": response.hex() if response else "",
                })
            elif self.path == "/api/calibrate":
                mode = body.get("mode")
                with self.gateway._lock:
                    config = dict(self.gateway._config)
                if mode == "manual":
                    reference_mm = float(body.get("reference_mm", 0))
                    reference_for_scale = (
                        reference_mm -
                        float(config.get("load_height_mm", 0)) -
                        float(config.get("length_cal_offset_tenth_mm", 0)) / 10.0
                    )
                    config["length_cal_x10000"] = calc_manual_length_cal_x10000(
                        reference_for_scale,
                        int(body.get("encoder", 0)),
                    )
                elif mode == "auto":
                    config["length_cal_x10000"] = calc_auto_length_cal_x10000(
                        float(body.get("shaft_diameter_mm", 10))
                    )
                elif mode == "motor_fg":
                    reference_mm = float(body.get("reference_mm", 0))
                    if body.get("reference_includes_load_height", True):
                        reference_mm -= float(
                            config.get("load_height_mm", 0)
                        )
                    config["motor_cal_x10000"] = calc_motor_cal_x10000(
                        reference_mm,
                        int(body.get("fg_count", 0)),
                    )
                elif mode == "linear_enc":
                    slope, offset = calc_linear_length_cal(
                        int(body.get("enc1", 0)),
                        float(body.get("mm1", 0)),
                        int(body.get("enc2", body.get("encoder", 0))),
                        float(body.get("mm2", body.get("reference_mm", 0))),
                    )
                    config["length_cal_x10000"] = slope
                    config["length_cal_offset_tenth_mm"] = offset
                else:
                    raise ValueError("неизвестный режим тарировки")
                payload = encode_config(config)
                response = self.gateway.execute(CMD_SET_CONFIG, payload)
                if response and response[0] != 0:
                    raise ValueError(f"контроллер отклонил тарировку: код {response[0]}")
                with self.gateway._lock:
                    self.gateway._config = decode_config(b"\x00" + payload)
                result = {
                    "ok": True,
                    "length_cal_x10000": config["length_cal_x10000"],
                    "length_cal_factor": length_cal_factor(config),
                    "motor_cal_x10000": config["motor_cal_x10000"],
                    "motor_cal_factor": motor_cal_factor(config),
                }
                if "length_cal_offset_tenth_mm" in config:
                    result["length_cal_offset_tenth_mm"] = (
                        config["length_cal_offset_tenth_mm"]
                    )
                    result["length_cal_offset_mm"] = (
                        length_cal_offset_mm(config)
                    )
                self._json(result)
            elif self.path == "/api/encoder-dataset":
                csv_text = body.get("csv", "")
                if not isinstance(csv_text, str) or not csv_text.strip():
                    raise ValueError("нет CSV данных")
                project_root = Path(__file__).resolve().parent.parent
                stamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
                latest_path = project_root / "encoder-test-latest.csv"
                stamped_name = f"encoder-test-{stamp}.csv"
                stamped_path = project_root / stamped_name
                latest_path.write_text(csv_text, encoding="utf-8")
                stamped_path.write_text(csv_text, encoding="utf-8")
                point_count = int(body.get("count", 0))
                self._json({
                    "ok": True,
                    "path": str(latest_path),
                    "relative_path": latest_path.name,
                    "stamped_path": stamped_name,
                    "point_count": point_count,
                    "chat_hint": (
                        f"@encoder-test-latest.csv — датасет теста энкодеров "
                        f"({point_count} точек)"
                    ),
                })
            else:
                self._json({"ok": False, "error": "not found"}, HTTPStatus.NOT_FOUND)
        except (KeyError, TypeError, ValueError) as exc:
            self._json({"ok": False, "error": str(exc)}, HTTPStatus.BAD_REQUEST)
        except (RuntimeError, TimeoutError) as exc:
            self._json({"ok": False, "error": str(exc)}, HTTPStatus.SERVICE_UNAVAILABLE)

    def log_message(self, fmt: str, *args) -> None:
        if self.path.startswith("/api/"):
            return
        _startup_log(f"HTTP {self.command} {self.path}: {fmt % args}")


def _acquire_single_instance_lock() -> tuple[Any, Path] | None:
    lock_dir = data_directory()
    lock_dir.mkdir(parents=True, exist_ok=True)
    lock_path = lock_dir / "gateway.lock"
    handle = open(lock_path, "a+", encoding="utf-8")
    try:
        if os.name == "nt":
            handle.seek(0, os.SEEK_END)
            if handle.tell() == 0:
                handle.write(" ")
                handle.flush()
            handle.seek(0)
            msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except (BlockingIOError, OSError):
        handle.close()
        print(
            "Ошибка: другой экземпляр лотового датчика уже запущен.",
            file=sys.stderr,
        )
        return None
    handle.seek(0)
    handle.truncate()
    handle.write(str(os.getpid()))
    handle.flush()
    return handle, lock_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Шлюз лотового датчика")
    parser.add_argument("--serial", help="COM-порт или /dev/ttyUSB0")
    parser.add_argument("--port", type=int, default=8765, help="HTTP-порт")
    parser.add_argument("--simulate", action="store_true")
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
    url = f"http://127.0.0.1:{args.port}/"

    lock = _acquire_single_instance_lock()
    if lock is None:
        if not args.no_browser:
            _open_browser_when_ready(url)
        return 1
    lock_handle, _lock_path = lock

    static_directory = resource_directory()
    event_log = EventLog(data_directory() / "lot_sensor.db")
    gateway = SerialGateway(event_log, args.serial, args.simulate)
    gateway.start()

    ApiHandler.gateway = gateway
    ApiHandler.event_log = event_log
    ApiHandler.static_directory = str(static_directory)
    server = ThreadingHTTPServer(("127.0.0.1", args.port), ApiHandler)
    print(f"Лотовый датчик: {url}")
    if not args.no_browser:
        threading.Thread(
            target=_open_browser_when_ready,
            args=(url,),
            daemon=True,
            name="browser-launch",
        ).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        gateway.stop()
        lock_handle.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
