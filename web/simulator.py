"""Software-only lot sensor used for operator training."""

from __future__ import annotations

import struct
import time

from protocol import (
    CMD_CLEAR_ERROR,
    CMD_GET_CALIBRATION_CAPTURE,
    CMD_MEASURE_AUTO_ONCE,
    CMD_MOVE_DOWN,
    CMD_MOVE_UP,
    CMD_PARK,
    CMD_RESET_ENCODERS,
    CMD_SET_AUTO,
    CMD_SET_BUNKER_CONFIG,
    CMD_SET_CONFIG,
    CMD_SET_MANUAL_PWM,
    CMD_STOP,
    ERROR_NAMES,
    MEASURE_MM_PER_PULSE,
    MOTOR_CIRCUMFERENCE_UM,
    MOTOR_GEAR_RATIO_X10,
    MOTOR_PULSES_PER_REV,
    STATE_NAMES,
    apply_linear_length_cal,
    decode_bunker_config,
    decode_config,
    fg_raw_mm,
    motor_cal_factor,
)


RESULT_OK = 0
RESULT_BAD_STATE = 4
SIMULATION_SPEED_MM_S = 300.0
MEASUREMENT_TRAVEL_SECONDS = 3.0
PARK_MIN_SECONDS = 0.4
MEASURE_REVERSAL_SECONDS = 0.02
CONTACT_REVERSAL_SECONDS = 0.15
TAKEUP_SECONDS = 0.15


class VirtualController:
    """Stateful replacement for STM32 and mechanics."""

    def __init__(self, config: dict, bunker_config: dict) -> None:
        self.config = dict(config)
        self.bunker_config = dict(bunker_config)
        now = time.monotonic()
        self.status = {
            "connected": True,
            "simulated": True,
            "port": "SIMULATOR",
            "state": 1,
            "state_name": STATE_NAMES[1],
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
            "measurement_count": 0,
            "buttons": 0,
            "distance_mm": 0.0,
            "motor_distance_mm": 0.0,
            "encoder_speed_cps": 0,
            "descent_distance_mm": 0.0,
            "ascent_distance_mm": 0.0,
            "travel_difference_mm": 0.0,
            "descent_motor_distance_mm": 0.0,
            "saved_descent_mm": 0.0,
            "result_mm": 0.0,
            "saved_ascent_mm": 0.0,
            "simulation_path_mm": 0.0,
            "simulation_target_mm": 0.0,
            "simulation_progress": 0.0,
            "updated": time.time(),
        }
        self.capture = {
            "down_valid": False,
            "up_valid": False,
            "raw_down_counts": 0,
            "raw_up_counts": 0,
            "raw_down_fg_count": 0,
            "raw_up_fg_count": 0,
        }
        self._phase = "ready"
        self._phase_started = now
        self._last_tick = now
        self._next_auto = now
        self._path_mm = 0.0
        self._target_path_mm = 0.0
        self._park_test_target_mm = 0.0
        self._parking_speed_mm_s = SIMULATION_SPEED_MM_S
        self._measurement_after_park = False
        self._park_test_before_measure = False
        self._encoder_mismatch_count = 0
        self._manual_pwm = int(self.config.get("run_pwm", 550))
        self._physical_enc_factor = max(
            0.0001, int(self.config.get("length_cal_x10000", 10000)) / 10000.0
        )
        self._physical_enc_offset_mm = (
            int(self.config.get("length_cal_offset_tenth_mm", 0)) / 10.0
        )
        self._physical_motor_factor = max(
            0.0001, int(self.config.get("motor_cal_x10000", 10000)) / 10000.0
        )

    def _set_state(
        self,
        state: int,
        *,
        motor_enabled: bool = False,
        motor_up: bool = False,
        pwm: int = 0,
    ) -> None:
        self.status.update(
            state=state,
            state_name=STATE_NAMES.get(state, f"Неизвестно ({state})"),
            motor_enabled=motor_enabled,
            motor_up=motor_up,
            pwm=int(pwm),
            sensor_busy=state != 1,
        )
        self._phase_started = time.monotonic()

    def _enc_counts(self, path_mm: float) -> int:
        raw_path = max(
            0.0, float(path_mm) - self._physical_enc_offset_mm
        )
        return int(round(
            raw_path / (MEASURE_MM_PER_PULSE * self._physical_enc_factor)
        ))

    def _fg_counts(self, path_mm: float) -> int:
        raw_mm_per_pulse = (
            MOTOR_CIRCUMFERENCE_UM
            / (MOTOR_PULSES_PER_REV * MOTOR_GEAR_RATIO_X10 * 100.0)
        )
        return int(round(
            max(0.0, float(path_mm)) /
            (raw_mm_per_pulse * self._physical_motor_factor)
        ))

    def _enc_mm(self, counts: int) -> float:
        return apply_linear_length_cal(
            counts,
            int(self.config.get("length_cal_x10000", 10000)),
            int(self.config.get("length_cal_offset_tenth_mm", 0)),
        )

    def _fg_mm(self, counts: int) -> float:
        return abs(fg_raw_mm(counts)) * motor_cal_factor(self.config)

    def _refresh_live_values(self, speed_mm_s: float = 0.0) -> None:
        enc = self._enc_counts(self._path_mm)
        fg = self._fg_counts(self._path_mm)
        enc_mm = self._enc_mm(enc)
        fg_mm = self._fg_mm(fg)
        self.status.update(
            encoder=enc,
            fg_count=fg,
            distance=enc,
            distance_mm=round(enc_mm, 1),
            motor_distance_mm=round(fg_mm, 1),
            encoder_speed_cps=int(
                abs(speed_mm_s) /
                max(0.0001, MEASURE_MM_PER_PULSE)
            ),
            simulation_path_mm=round(self._path_mm, 1),
            simulation_target_mm=round(self._target_path_mm, 1),
            simulation_progress=(
                max(0.0, min(1.0, self._path_mm / self._target_path_mm))
                if self._target_path_mm > 0.0 else 0.0
            ),
        )

    def _reset_runtime(self) -> None:
        self._phase = "ready"
        self._path_mm = 0.0
        self._target_path_mm = 0.0
        self._park_test_target_mm = 0.0
        self._measurement_after_park = False
        self._park_test_before_measure = False
        self._encoder_mismatch_count = 0
        self.capture.update(
            down_valid=False,
            up_valid=False,
            raw_down_counts=0,
            raw_up_counts=0,
            raw_down_fg_count=0,
            raw_up_fg_count=0,
        )
        self.status.update(
            errors=0,
            error_names=[],
            auto=False,
            ground_detect=False,
            encoder=0,
            fg_count=0,
            distance=0,
            measurement_count=0,
            distance_mm=0.0,
            motor_distance_mm=0.0,
            encoder_speed_cps=0,
            descent_distance_mm=0.0,
            descent_motor_distance_mm=0.0,
            ascent_distance_mm=0.0,
            travel_difference_mm=0.0,
            saved_descent_mm=0.0,
            saved_ascent_mm=0.0,
            result_mm=0.0,
            simulation_path_mm=0.0,
            simulation_target_mm=0.0,
            simulation_progress=0.0,
        )
        self._set_state(1)

    def _configured_ground_path_mm(self) -> float:
        lower = float(self.bunker_config.get("lower_height_mm", 0))
        cylinder = float(self.bunker_config.get("cylinder_height_mm", 0))
        upper = float(self.bunker_config.get("upper_height_mm", 0))
        total_height = lower + cylinder + upper
        fill_fraction = max(
            0.0,
            min(1.0, float(self.bunker_config.get("target_fill_x10", 0)) /
                1000.0),
        )
        fill_height = total_height * fill_fraction
        sensor_height = (
            lower +
            cylinder +
            float(self.bunker_config.get("load_offset_mm", 0))
        )
        load_height = float(self.config.get("load_height_mm", 0))
        return max(0.0, sensor_height - fill_height - load_height)

    def _begin_parking(
        self, measurement_after: bool, park_test: bool = False
    ) -> None:
        self._measurement_after_park = measurement_after
        self._park_test_before_measure = park_test and measurement_after
        self._target_path_mm = self._configured_ground_path_mm()
        configured_test_mm = (
            float(self.config.get("park_test_pulses", 0)) *
            MEASURE_MM_PER_PULSE *
            self._physical_enc_factor
        )
        visible_limit_mm = max(20.0, self._target_path_mm * 0.2)
        self._park_test_target_mm = min(
            max(20.0, configured_test_mm),
            200.0,
            visible_limit_mm,
        )
        self._parking_speed_mm_s = max(
            SIMULATION_SPEED_MM_S,
            self._path_mm / 2.0,
        )
        self._phase = "parking_up"
        self.status["ground_detect"] = False
        self._set_state(
            4,
            motor_enabled=True,
            motor_up=True,
            pwm=int(self.config.get("park_pwm", 450)),
        )

    def _begin_measurement_descent(self) -> None:
        self._target_path_mm = self._configured_ground_path_mm()
        self.capture.update(
            down_valid=False,
            up_valid=False,
            raw_down_counts=0,
            raw_up_counts=0,
            raw_down_fg_count=0,
            raw_up_fg_count=0,
        )
        self.status.update(
            descent_distance_mm=0.0,
            descent_motor_distance_mm=0.0,
            ascent_distance_mm=0.0,
            travel_difference_mm=0.0,
            ground_detect=False,
        )
        self._path_mm = 0.0
        self._phase = "descending"
        self._set_state(
            5,
            motor_enabled=True,
            motor_up=False,
            pwm=int(self.config.get("run_pwm", 550)),
        )
        self._refresh_live_values()

    def _complete_measurement(self, now: float) -> None:
        path = self._target_path_mm
        enc_counts = self._enc_counts(path)
        fg_counts = self._fg_counts(path)
        enc_path = self._enc_mm(enc_counts)
        fg_path = self._fg_mm(fg_counts)
        self.capture.update(
            down_valid=True,
            up_valid=True,
            raw_down_counts=enc_counts,
            raw_up_counts=enc_counts,
            raw_down_fg_count=fg_counts,
            raw_up_fg_count=fg_counts,
        )
        lower = float(self.bunker_config.get("lower_height_mm", 0))
        cylinder = float(self.bunker_config.get("cylinder_height_mm", 0))
        sensor_height = (
            lower +
            cylinder +
            float(self.bunker_config.get("load_offset_mm", 0))
        )
        selected_path = (
            fg_path
            if int(self.config.get("motor_encoder_enabled", 0)) != 0
            else enc_path
        )
        measured_distance = (
            selected_path + float(self.config.get("load_height_mm", 0))
        )
        result_mm = max(0.0, sensor_height - measured_distance)
        self._path_mm = 0.0
        self._refresh_live_values()
        self.status.update(
            descent_distance_mm=round(enc_path, 1),
            descent_motor_distance_mm=round(fg_path, 1),
            ascent_distance_mm=round(enc_path, 1),
            saved_descent_mm=round(enc_path, 1),
            saved_ascent_mm=round(enc_path, 1),
            travel_difference_mm=0.0,
            result_mm=round(result_mm, 1),
            measurement_count=int(self.status["measurement_count"]) + 1,
            ground_detect=False,
        )
        mismatch_mm = int(self.config.get("mismatch_mm", 0))
        if mismatch_mm > 0 and abs(enc_path - fg_path) > mismatch_mm:
            self._encoder_mismatch_count += 1
        else:
            self._encoder_mismatch_count = 0
        mismatch_fault = (
            mismatch_mm > 0 and
            self._encoder_mismatch_count >=
            int(self.config.get("error_measurement_limit", 2))
        )
        if mismatch_fault:
            self._phase = "fault"
            self.status.update(
                auto=False,
                errors=1 << 4,
                error_names=[ERROR_NAMES[4]],
            )
            self._set_state(8)
        else:
            self._phase = "ready"
            self._set_state(1)
        self._next_auto = now + (
            max(1000, int(self.config.get("auto_interval_ms", 60000))) / 1000.0
        )

    def _capture_payload(self) -> bytes:
        flags = (
            (0x01 if self.capture["down_valid"] else 0) |
            (0x02 if self.capture["up_valid"] else 0)
        )
        return struct.pack(
            "<BBiiii",
            RESULT_OK,
            flags,
            int(self.capture["raw_down_counts"]),
            int(self.capture["raw_up_counts"]),
            int(self.capture["raw_down_fg_count"]),
            int(self.capture["raw_up_fg_count"]),
        )

    def handle(self, command: int, payload: bytes = b"") -> bytes:
        now = time.monotonic()
        if command == CMD_MOVE_UP:
            self._phase = "manual_up"
            self._set_state(
                2, motor_enabled=self._manual_pwm != 0,
                motor_up=True, pwm=self._manual_pwm,
            )
        elif command == CMD_MOVE_DOWN:
            self._phase = "manual_down"
            on_ground = (
                self._path_mm >= self._configured_ground_path_mm()
            )
            self.status["ground_detect"] = on_ground
            self._set_state(
                3,
                motor_enabled=self._manual_pwm != 0 and not on_ground,
                motor_up=False,
                pwm=0 if on_ground else self._manual_pwm,
            )
        elif command == CMD_STOP:
            self.status["auto"] = False
            self._encoder_mismatch_count = 0
            self._phase = "ready"
            self._set_state(1)
        elif command == CMD_PARK:
            self._begin_parking(False)
        elif command == CMD_MEASURE_AUTO_ONCE:
            if int(self.status["state"]) != 1:
                return bytes((RESULT_BAD_STATE,))
            self._begin_parking(True)
        elif command == CMD_SET_AUTO:
            self.status["auto"] = bool(payload[0])
            if not self.status["auto"]:
                self._encoder_mismatch_count = 0
            self._next_auto = now
        elif command == CMD_SET_MANUAL_PWM:
            self._manual_pwm = int.from_bytes(payload, "little")
            if int(self.status["state"]) in (2, 3):
                if (int(self.status["state"]) == 3 and
                        self.status.get("ground_detect")):
                    self.status["pwm"] = 0
                    self.status["motor_enabled"] = False
                else:
                    self.status["pwm"] = self._manual_pwm
                    self.status["motor_enabled"] = self._manual_pwm != 0
        elif command == CMD_CLEAR_ERROR:
            self._reset_runtime()
        elif command == CMD_RESET_ENCODERS:
            self._path_mm = 0.0
            self._refresh_live_values()
        elif command == CMD_SET_CONFIG:
            self.config = decode_config(b"\x00" + payload)
            if int(self.config.get("mismatch_mm", 0)) == 0:
                self._encoder_mismatch_count = 0
        elif command == CMD_SET_BUNKER_CONFIG:
            self.bunker_config = decode_bunker_config(b"\x00" + payload)
        elif command == CMD_GET_CALIBRATION_CAPTURE:
            return self._capture_payload()
        self.status["updated"] = time.time()
        return bytes((RESULT_OK,))

    def tick(self, now: float) -> bool:
        dt = max(0.0, min(0.1, now - self._last_tick))
        self._last_tick = now
        completed = False
        run_speed = SIMULATION_SPEED_MM_S * max(
            0.2, float(self.config.get("run_pwm", 550)) / 550.0
        )
        measurement_speed = max(
            10.0,
            self._target_path_mm / MEASUREMENT_TRAVEL_SECONDS,
        )
        park_release_seconds = max(
            0.1,
            min(1.0, float(self.config.get("park_release_ms", 500)) / 1000.0),
        )

        if (self._phase == "ready" and self.status["auto"] and
                now >= self._next_auto):
            self._begin_parking(True, park_test=True)
        elif self._phase == "manual_down":
            ground_mm = self._configured_ground_path_mm()
            if self._path_mm >= ground_mm:
                self._path_mm = ground_mm
                self.status["ground_detect"] = True
                self.status["motor_enabled"] = False
                self.status["pwm"] = 0
                self._refresh_live_values(0.0)
            else:
                self._path_mm += run_speed * dt
                if self._path_mm >= ground_mm:
                    self._path_mm = ground_mm
                    self.status["ground_detect"] = True
                    self.status["motor_enabled"] = False
                    self.status["pwm"] = 0
                    self._refresh_live_values(0.0)
                else:
                    self._refresh_live_values(run_speed)
                    self.status["ground_detect"] = False
        elif self._phase == "manual_up":
            self._path_mm = max(0.0, self._path_mm - run_speed * dt)
            self._refresh_live_values(run_speed)
            self.status["ground_detect"] = False
        elif self._phase == "parking_up":
            self._path_mm = max(
                0.0, self._path_mm - self._parking_speed_mm_s * dt
            )
            self._refresh_live_values(self._parking_speed_mm_s)
            if (self._path_mm <= 0.0 and
                    now - self._phase_started >= PARK_MIN_SECONDS):
                if (self._measurement_after_park and
                        not self._park_test_before_measure):
                    self._measurement_after_park = False
                    self._phase = "measure_wait"
                    self._set_state(13)
                else:
                    self._phase = "park_release"
                    self._set_state(9)
        elif self._phase == "park_release":
            if now - self._phase_started >= park_release_seconds:
                self._phase = "park_test_down"
                self._set_state(
                    10,
                    motor_enabled=True,
                    motor_up=False,
                    pwm=int(self.config.get("park_pwm", 450)),
                )
        elif self._phase == "park_test_down":
            self._path_mm = min(
                self._park_test_target_mm,
                self._path_mm + self._parking_speed_mm_s * dt,
            )
            self._refresh_live_values(self._parking_speed_mm_s)
            if self._path_mm >= self._park_test_target_mm:
                self._phase = "park_test_stop"
                self._set_state(12)
        elif self._phase == "park_test_stop":
            if now - self._phase_started >= TAKEUP_SECONDS:
                self._phase = "park_return"
                self._set_state(
                    11,
                    motor_enabled=True,
                    motor_up=True,
                    pwm=int(self.config.get("park_pwm", 450)),
                )
        elif self._phase == "park_return":
            self._path_mm = max(
                0.0, self._path_mm - self._parking_speed_mm_s * dt
            )
            self._refresh_live_values(self._parking_speed_mm_s)
            if self._path_mm <= 0.0:
                if self._measurement_after_park:
                    self._measurement_after_park = False
                    self._phase = "measure_wait"
                    self._set_state(13)
                else:
                    self._phase = "ready"
                    self._set_state(1)
        elif self._phase == "measure_wait":
            if now - self._phase_started >= MEASURE_REVERSAL_SECONDS:
                self._begin_measurement_descent()
        elif self._phase == "descending":
            self._path_mm = min(
                self._target_path_mm,
                self._path_mm + measurement_speed * dt,
            )
            self._refresh_live_values(measurement_speed)
            self.status.update(
                descent_distance_mm=round(
                    self._enc_mm(self._enc_counts(self._path_mm)), 1
                ),
                descent_motor_distance_mm=round(
                    self._fg_mm(self._fg_counts(self._path_mm)), 1
                ),
            )
            if self._path_mm >= self._target_path_mm:
                self.capture.update(
                    down_valid=True,
                    raw_down_counts=self._enc_counts(self._target_path_mm),
                    raw_down_fg_count=self._fg_counts(self._target_path_mm),
                )
                self.status["ground_detect"] = True
                self._phase = "contact"
                self._set_state(6)
        elif self._phase == "contact":
            if now - self._phase_started >= CONTACT_REVERSAL_SECONDS:
                self.status["ground_detect"] = False
                self._phase = "takeup"
                self._set_state(
                    14, motor_enabled=True, motor_up=True,
                    pwm=int(self.config.get("takeup_pwm", 945)),
                )
        elif self._phase == "takeup":
            if now - self._phase_started >= TAKEUP_SECONDS:
                self._phase = "ramp"
                self._set_state(
                    15, motor_enabled=True, motor_up=True,
                    pwm=int(self.config.get("takeup_pwm", 945)),
                )
        elif self._phase == "ramp":
            if now - self._phase_started >= TAKEUP_SECONDS:
                self._phase = "ascending"
                self._set_state(
                    7, motor_enabled=True, motor_up=True,
                    pwm=int(self.config.get("run_pwm", 550)),
                )
        elif self._phase == "ascending":
            self._path_mm = max(
                0.0, self._path_mm - measurement_speed * dt
            )
            self._refresh_live_values(measurement_speed)
            ascent_path = self._target_path_mm - self._path_mm
            self.status["ascent_distance_mm"] = round(
                self._enc_mm(self._enc_counts(ascent_path)), 1
            )
            if self._path_mm <= 0.0:
                self._complete_measurement(now)
                completed = True

        self.status["updated"] = time.time()
        return completed
