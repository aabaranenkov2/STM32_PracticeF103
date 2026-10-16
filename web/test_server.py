import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import call, patch
from urllib.error import URLError

from server import (
    ApiHandler,
    _open_browser_when_ready,
    factory_profile_path,
    load_factory_profile,
)


class FakeResponse:
    status = 200

    def __init__(self, body: bytes):
        self._body = body

    def __enter__(self):
        return self

    def __exit__(self, _exc_type, _exc, _traceback):
        return False

    def read(self) -> bytes:
        return self._body


class BrowserStartupTests(unittest.TestCase):
    @patch("server._startup_log")
    @patch("server.time.sleep")
    @patch("server.webbrowser.open", return_value=True)
    @patch(
        "server.urlopen",
        side_effect=[
            URLError("not ready"),
            FakeResponse(b'{"version":"1.0.4"}'),
            FakeResponse(b"<!DOCTYPE html><html></html>"),
        ],
    )
    def test_browser_opens_only_after_server_answers(
        self, urlopen_mock, browser_mock, sleep_mock, log_mock
    ):
        self.assertTrue(
            _open_browser_when_ready(
                "http://127.0.0.1:8765/", attempts=3, delay_seconds=0.01
            )
        )

        self.assertEqual(urlopen_mock.call_count, 3)
        sleep_mock.assert_called_once_with(0.01)
        browser_mock.assert_called_once_with("http://127.0.0.1:8765/")
        log_mock.assert_called_once_with("Страница открыта после попытки 2")

    @patch("server._startup_log")
    @patch("server.time.sleep")
    @patch("server.webbrowser.open")
    @patch("server.urlopen", side_effect=URLError("not ready"))
    def test_failure_is_logged_after_all_attempts(
        self, urlopen_mock, browser_mock, sleep_mock, log_mock
    ):
        self.assertFalse(
            _open_browser_when_ready(
                "http://127.0.0.1:8765/", attempts=3, delay_seconds=0.01
            )
        )

        self.assertEqual(urlopen_mock.call_count, 3)
        self.assertEqual(sleep_mock.call_args_list, [call(0.01), call(0.01)])
        browser_mock.assert_not_called()
        self.assertIn(
            "Не удалось открыть страницу за 3 попыток",
            log_mock.call_args.args[0],
        )

    @patch("server._startup_log")
    def test_static_request_logging_does_not_use_console(self, log_mock):
        handler = SimpleNamespace(path="/", command="GET")

        ApiHandler.log_message(handler, '"%s" %s', "GET / HTTP/1.1", "200")

        log_mock.assert_called_once_with(
            'HTTP GET /: "GET / HTTP/1.1" 200'
        )


class FactoryProfileTests(unittest.TestCase):
    def test_profile_contains_tuned_controller_and_bunker_values(self):
        profile = load_factory_profile()

        self.assertEqual(profile["controller_config"]["run_pwm"], 670)
        self.assertEqual(
            profile["controller_config"]["length_cal_x10000"], 10747
        )
        self.assertEqual(
            profile["controller_config"]["motor_cal_x10000"], 12221
        )
        self.assertEqual(
            profile["controller_config"]["motor_encoder_enabled"], 1
        )
        self.assertEqual(
            profile["bunker_config"]["bunker_diameter_mm"], 5600
        )
        self.assertEqual(
            profile["bunker_config"]["cylinder_height_mm"], 10000
        )

    def test_profile_is_copied_once_to_writable_data_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            resources = root / "resources"
            data = root / "data"
            resources.mkdir()
            data.mkdir()
            source = resources / "factory_defaults.json"
            source.write_text(
                json.dumps({"profile_name": "Первоначальный"}),
                encoding="utf-8",
            )

            with (
                patch("server.resource_directory", return_value=resources),
                patch("server.data_directory", return_value=data),
            ):
                destination = factory_profile_path()
                source.write_text(
                    json.dumps({"profile_name": "Новый"}),
                    encoding="utf-8",
                )
                second_destination = factory_profile_path()

            self.assertEqual(destination, second_destination)
            saved = json.loads(destination.read_text(encoding="utf-8"))
            self.assertEqual(saved["profile_name"], "Первоначальный")


if __name__ == "__main__":
    unittest.main()
