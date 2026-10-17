import time
import unittest

from protocol import (
    CMD_CLEAR_ERROR,
    CMD_GET_CALIBRATION_CAPTURE,
    CMD_GET_DIAGNOSTICS,
    CMD_MEASURE_AUTO_ONCE,
    CMD_MOVE_DOWN,
    CMD_PARK,
    CMD_SET_AUTO,
    CMD_SET_BUNKER_CONFIG,
    CMD_SET_CONFIG,
    decode_calibration_capture,
    decode_diagnostics,
    encode_bunker_config,
    encode_config,
)
from simulator import VirtualController


def test_config() -> dict:
    return {
        "run_pwm": 550,
        "park_pwm": 450,
        "contact_timeout_ms": 200,
        "stall_timeout_ms": 200,
        "mismatch_mm": 100,
        "park_release_ms": 20,
        "park_test_ms": 2000,
        "park_test_pulses": 100,
        "contact_speed_percent": 75,
        "contact_ignore_ms": 100,
        "slowdown_remaining_percent": 10,
        "slowdown_divisor_tenths": 10,
        "auto_interval_ms": 60000,
        "length_cal_x10000": 10000,
        "motor_cal_x10000": 10000,
        "length_cal_offset_tenth_mm": 0,
        "brake_comp_mm": 0,
        "load_height_mm": 0,
        "motor_encoder_enabled": 0,
        "descent_correction_mm": 0,
        "error_measurement_limit": 2,
        "motion_timeout_ms": 120000,
        "max_descent_mm": 10000,
        "takeup_pwm": 900,
        "ascent_correction_mm": 0,
    }


def test_bunker_config() -> dict:
    return {
        "max_fill_x10": 1000,
        "target_fill_x10": 850,
        "model_stroke_mm": 1000,
        "upper_height_mm": 0,
        "upper_top_diameter_mm": 1000,
        "bunker_diameter_mm": 5000,
        "cylinder_height_mm": 1000,
        "lower_height_mm": 1000,
        "lower_bottom_diameter_mm": 1000,
        "load_offset_mm": 500,
        "density_x1000": 750,
        "product_factor_x1000": 1000,
    }


class SimulatorTests(unittest.TestCase):
    def run_measurement(self, simulator: VirtualController) -> None:
        self.assertEqual(
            simulator.handle(CMD_MEASURE_AUTO_ONCE),
            b"\x00",
        )
        now = time.monotonic()
        for step in range(1, 1000):
            if simulator.tick(now + step * 0.1):
                return
        self.fail("simulated measurement did not finish")

    def test_measurement_generates_both_encoders_and_result(self):
        simulator = VirtualController(test_config(), test_bunker_config())

        self.run_measurement(simulator)

        self.assertEqual(simulator.status["state"], 1)
        self.assertEqual(simulator.status["measurement_count"], 1)
        self.assertAlmostEqual(simulator.status["result_mm"], 1700.0, delta=1.0)
        capture = decode_calibration_capture(
            simulator.handle(CMD_GET_CALIBRATION_CAPTURE)
        )
        self.assertTrue(capture["down_valid"])
        self.assertTrue(capture["up_valid"])
        self.assertGreater(capture["raw_down_counts"], 0)
        self.assertGreater(capture["raw_down_fg_count"], 0)
        diagnostics = decode_diagnostics(
            simulator.handle(CMD_GET_DIAGNOSTICS)
        )
        self.assertEqual(diagnostics["sequence"], 1)
        self.assertGreater(diagnostics["measurement_ms"], 0)
        self.assertGreater(diagnostics["descent_ms"], 0)
        self.assertGreater(diagnostics["ascent_ms"], 0)

    def test_encoder_mismatch_control_stops_after_configured_limit(self):
        config = test_config()
        config["mismatch_mm"] = 10
        config["error_measurement_limit"] = 2
        simulator = VirtualController(config, test_bunker_config())
        simulator.config["motor_cal_x10000"] = 15000

        self.run_measurement(simulator)
        self.assertEqual(simulator.status["state"], 1)
        self.run_measurement(simulator)

        self.assertEqual(simulator.status["state"], 8)
        self.assertEqual(simulator.status["errors"], 1 << 4)
        self.assertFalse(simulator.status["auto"])

    def test_encoder_mismatch_control_can_be_disabled(self):
        config = test_config()
        config["mismatch_mm"] = 0
        simulator = VirtualController(config, test_bunker_config())
        simulator.config["motor_cal_x10000"] = 15000

        self.run_measurement(simulator)
        self.run_measurement(simulator)

        self.assertEqual(simulator.status["state"], 1)
        self.assertEqual(simulator.status["errors"], 0)

    def test_parking_runs_release_test_and_return_states(self):
        simulator = VirtualController(test_config(), test_bunker_config())
        simulator.handle(CMD_PARK)
        states = [simulator.status["state"]]
        now = time.monotonic()

        for step in range(1, 200):
            simulator.tick(now + step * 0.1)
            if states[-1] != simulator.status["state"]:
                states.append(simulator.status["state"])
            if simulator.status["state"] == 1:
                break

        self.assertEqual(states, [4, 9, 10, 12, 11, 1])

    def test_periodic_mode_starts_with_parking(self):
        simulator = VirtualController(test_config(), test_bunker_config())
        simulator.handle(CMD_SET_AUTO, b"\x01")

        simulator.tick(time.monotonic() + 0.1)

        self.assertEqual(simulator.status["state"], 4)
        self.assertEqual(simulator.status["state_name"], "Парковка")

    def test_periodic_mode_parks_before_measure_not_after(self):
        config = test_config()
        config["auto_interval_ms"] = 60000
        simulator = VirtualController(config, test_bunker_config())
        simulator.handle(CMD_SET_AUTO, b"\x01")
        states = [simulator.status["state"]]
        now = time.monotonic()

        for step in range(1, 400):
            completed = simulator.tick(now + step * 0.1)
            if states[-1] != simulator.status["state"]:
                states.append(simulator.status["state"])
            if completed:
                break

        self.assertEqual(states[:7], [1, 4, 9, 10, 12, 11, 13])
        self.assertIn(5, states)
        self.assertEqual(states[-1], 1)
        self.assertNotIn(9, states[states.index(5):])

    def test_measurement_skips_release_and_parking_test(self):
        simulator = VirtualController(test_config(), test_bunker_config())
        simulator.handle(CMD_MEASURE_AUTO_ONCE)
        states = [simulator.status["state"]]
        now = time.monotonic()

        for step in range(1, 100):
            simulator.tick(now + step * 0.01)
            if states[-1] != simulator.status["state"]:
                states.append(simulator.status["state"])
            if simulator.status["state"] == 5:
                break

        self.assertEqual(states, [4, 13, 5])

    def test_selected_motor_factor_changes_result_only_in_simulator(self):
        config = test_config()
        config["motor_encoder_enabled"] = 1
        simulator = VirtualController(config, test_bunker_config())
        changed = dict(config)
        changed["motor_cal_x10000"] = 11000
        simulator.handle(CMD_SET_CONFIG, encode_config(changed))

        self.run_measurement(simulator)

        self.assertAlmostEqual(simulator.status["result_mm"], 1620.0, delta=2.0)

    def test_sensor_height_above_base_extends_ground_path(self):
        bunker = test_bunker_config()
        bunker["load_offset_mm"] = 0
        at_base = VirtualController(test_config(), bunker)
        raised_bunker = dict(bunker)
        raised_bunker["load_offset_mm"] = 500
        raised = VirtualController(test_config(), raised_bunker)

        self.assertAlmostEqual(
            raised._configured_ground_path_mm() -
            at_base._configured_ground_path_mm(),
            500.0,
            delta=0.1,
        )

    def test_config_write_stays_in_virtual_controller(self):
        original = test_config()
        simulator = VirtualController(original, test_bunker_config())
        changed = dict(original)
        changed["length_cal_x10000"] = 12345

        simulator.handle(CMD_SET_CONFIG, encode_config(changed))

        self.assertEqual(simulator.config["length_cal_x10000"], 12345)
        self.assertEqual(original["length_cal_x10000"], 10000)

    def test_clear_error_returns_simulator_to_initial_state(self):
        simulator = VirtualController(test_config(), test_bunker_config())
        self.run_measurement(simulator)
        simulator.handle(CMD_SET_AUTO, b"\x01")

        simulator.handle(CMD_CLEAR_ERROR)

        self.assertEqual(simulator.status["state"], 1)
        self.assertFalse(simulator.status["auto"])
        self.assertFalse(simulator.status["ground_detect"])
        self.assertEqual(simulator.status["encoder"], 0)
        self.assertEqual(simulator.status["fg_count"], 0)
        self.assertEqual(simulator.status["measurement_count"], 0)
        self.assertEqual(simulator.status["result_mm"], 0.0)
        self.assertEqual(simulator.status["simulation_progress"], 0.0)

    def test_schematic_bunker_fields_persist_via_controller_protocol(self):
        """Values edited on the bunker diagram map 1:1 to SET_BUNKER_CONFIG."""
        simulator = VirtualController(test_config(), test_bunker_config())
        schematic = dict(test_bunker_config())
        schematic.update({
            "load_offset_mm": 215,
            "bunker_diameter_mm": 5400,
            "upper_height_mm": 1600,
            "cylinder_height_mm": 10100,
            "lower_height_mm": 2100,
            "lower_bottom_diameter_mm": 1200,
        })

        self.assertEqual(
            simulator.handle(
                CMD_SET_BUNKER_CONFIG, encode_bunker_config(schematic)),
            b"\x00",
        )

        for key, value in schematic.items():
            self.assertEqual(simulator.bunker_config[key], value)

    def test_manual_down_stops_on_ground(self):
        simulator = VirtualController(test_config(), test_bunker_config())
        self.assertEqual(simulator.handle(CMD_MOVE_DOWN), b"\x00")
        now = time.monotonic()
        stopped = False
        for step in range(1, 200):
            simulator.tick(now + step * 0.1)
            if simulator.status["ground_detect"]:
                stopped = True
                path_at_stop = simulator.status["simulation_path_mm"]
                self.assertFalse(simulator.status["motor_enabled"])
                self.assertEqual(simulator.status["pwm"], 0)
                simulator.tick(now + (step + 5) * 0.1)
                self.assertFalse(simulator.status["motor_enabled"])
                self.assertEqual(
                    simulator.status["simulation_path_mm"], path_at_stop)
                break
        self.assertTrue(stopped, "manual down did not reach ground")


if __name__ == "__main__":
    unittest.main()
