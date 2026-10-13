import struct
import unittest

from protocol import (
    CMD_GET_STATUS,
    RESPONSE_FLAG,
    FrameParser,
    calc_auto_length_cal_x10000,
    calc_manual_length_cal_x10000,
    calc_linear_length_cal,
    calc_motor_cal_x10000,
    apply_linear_length_cal,
    decode_bunker_config,
    decode_calibration_capture,
    decode_config,
    decode_status,
    encode_config,
    encode_bunker_config,
    encode_frame,
    fg_raw_mm,
)


class ProtocolTests(unittest.TestCase):
    def test_fragmented_frame_after_noise(self):
        expected = encode_frame(CMD_GET_STATUS | RESPONSE_FLAG, 7, b"\x00")
        parser = FrameParser()
        self.assertEqual(parser.feed(b"\x99\x55\x01"), [])
        frames = parser.feed(expected[:4])
        frames += parser.feed(expected[4:])
        self.assertEqual(len(frames), 1)
        self.assertEqual(frames[0].sequence, 7)
        self.assertEqual(frames[0].payload, b"\x00")

    def test_bad_crc_is_ignored(self):
        frame = bytearray(encode_frame(CMD_GET_STATUS, 2))
        frame[-1] ^= 0x80
        self.assertEqual(FrameParser().feed(frame), [])

    def test_decode_status(self):
        payload = bytearray(64)
        payload[1] = 7
        struct.pack_into("<H", payload, 2, 0x10)
        payload[4] = 0x37
        struct.pack_into("<i", payload, 5, -123)
        struct.pack_into("<I", payload, 9, 456)
        struct.pack_into("<H", payload, 13, 550)
        struct.pack_into("<i", payload, 15, 900)
        struct.pack_into("<I", payload, 19, 12)
        payload[23] = 3
        struct.pack_into("<i", payload, 24, -1234)
        struct.pack_into("<I", payload, 28, 5678)
        struct.pack_into("<I", payload, 32, 900)
        struct.pack_into("<i", payload, 36, 1000)
        struct.pack_into("<i", payload, 40, 980)
        struct.pack_into("<i", payload, 44, 20)
        struct.pack_into("<i", payload, 48, 995)
        struct.pack_into("<i", payload, 52, 1000)
        struct.pack_into("<i", payload, 56, 1110)
        struct.pack_into("<i", payload, 60, 980)
        status = decode_status(bytes(payload))
        self.assertEqual(status["encoder"], -123)
        self.assertEqual(status["measurement_count"], 12)
        self.assertTrue(status["auto"])
        self.assertTrue(status["interlock_active"])
        self.assertTrue(status["sensor_busy"])
        self.assertIn("Расхождение", status["error_names"][0])
        self.assertEqual(status["distance_mm"], -123.4)
        self.assertEqual(status["motor_distance_mm"], 567.8)
        self.assertEqual(status["encoder_speed_cps"], 900)
        self.assertEqual(status["descent_distance_mm"], 100.0)
        self.assertEqual(status["ascent_distance_mm"], 98.0)
        self.assertEqual(status["travel_difference_mm"], 2.0)
        self.assertEqual(status["descent_motor_distance_mm"], 99.5)
        self.assertEqual(status["saved_descent_mm"], 100.0)
        self.assertEqual(status["result_mm"], 111.0)
        self.assertEqual(status["saved_ascent_mm"], 98.0)

    def test_decode_calibration_capture(self):
        payload = bytearray(18)
        payload[1] = 0x03
        struct.pack_into("<i", payload, 2, 52400)
        struct.pack_into("<i", payload, 6, 52380)
        struct.pack_into("<i", payload, 10, 1120)
        struct.pack_into("<i", payload, 14, 1118)

        capture = decode_calibration_capture(bytes(payload))

        self.assertTrue(capture["down_valid"])
        self.assertTrue(capture["up_valid"])
        self.assertEqual(capture["raw_down_counts"], 52400)
        self.assertEqual(capture["raw_up_counts"], 52380)
        self.assertEqual(capture["raw_up_fg_count"], 1118)

    def test_parking_config_round_trip(self):
        config = {
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
            "length_cal_offset_tenth_mm": -35,
            "brake_comp_mm": 51,
            "load_height_mm": 110,
            "motor_encoder_enabled": 0,
            "descent_correction_mm": -12,
            "error_measurement_limit": 2,
            "motion_timeout_ms": 60000,
            "max_descent_mm": 1000,
            "takeup_pwm": 945,
            "ascent_correction_mm": -7,
        }
        self.assertEqual(decode_config(b"\x00" + encode_config(config)), config)

    def test_bunker_config_round_trip(self):
        config = {
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
        payload = encode_bunker_config(config)
        self.assertEqual(len(payload), 44)
        self.assertEqual(
            decode_bunker_config(b"\x00" + payload),
            config,
        )

    def test_linear_length_calibration(self):
        slope, offset = calc_linear_length_cal(0, 0.0, 43205, 559.2)
        self.assertAlmostEqual(
            apply_linear_length_cal(43205, slope, offset),
            559.2,
            delta=0.5,
        )
        self.assertAlmostEqual(apply_linear_length_cal(0, slope, offset), 0.0, delta=0.1)

    def test_motor_fg_calibration(self):
        fg_count = 617
        reference_mm = 591.5
        raw_mm = fg_raw_mm(fg_count)
        cal = calc_motor_cal_x10000(reference_mm, fg_count)
        self.assertAlmostEqual(raw_mm * cal / 10000.0, reference_mm, delta=0.5)

    def test_manual_length_calibration(self):
        encoder_counts = int(1000 / (31.416 / 2400))
        cal = calc_manual_length_cal_x10000(1000.0, encoder_counts)
        self.assertEqual(cal, 10000)

    def test_auto_length_calibration(self):
        self.assertEqual(calc_auto_length_cal_x10000(10.0), 10000)
        self.assertEqual(calc_auto_length_cal_x10000(10.5), 10500)


if __name__ == "__main__":
    unittest.main()
