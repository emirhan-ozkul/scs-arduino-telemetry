import sys
import unittest

sys.path.insert(0, r"C:\Users\gapcu\Documents\vscode\SDKApp\ETS2sdkapp\python")

from telemetry_model import Telemetry, decode_packet

class TelemetryModelTests(unittest.TestCase):
    def test_decode_and_convert(self):
        packet = b'{"protocol":"scs-telemetry-v1","game_id":"eut2","connected":true,"speed_mps":20,"engine_rpm":1450,"gear":8,"fuel_l":120.5,"fuel_consumption_l_km":0.28,"odometer_km":12345,"remaining_distance_m":2500,"water_temperature_c":88}'
        telemetry = decode_packet(packet)
        self.assertEqual(telemetry.game_id, "eut2")
        self.assertAlmostEqual(telemetry.speed_kmh, 72.0)
        self.assertEqual(telemetry.gear, 8)

    def test_missing_values_remain_none(self):
        packet = b'{"protocol":"scs-telemetry-v1","game_id":"ats","connected":true,"speed_mps":null,"engine_rpm":null,"gear":null,"fuel_l":null,"fuel_consumption_l_km":null,"odometer_km":null,"remaining_distance_m":null,"water_temperature_c":null}'
        telemetry = decode_packet(packet)
        self.assertIsNone(telemetry.speed_mps)
        self.assertIsNone(telemetry.gear)
        self.assertTrue(telemetry.serial_frame().startswith("T,1,"))

    def test_invalid_protocol_rejected(self):
        with self.assertRaises(ValueError):
            decode_packet(b'{"protocol":"wrong","connected":true}')

    def test_invalid_numeric_rejected(self):
        packet = b'{"protocol":"scs-telemetry-v1","game_id":"eut2","connected":true,"speed_mps":"fast"}'
        with self.assertRaises(ValueError):
            decode_packet(packet)

    def test_stale_telemetry(self):
        telemetry = Telemetry(received_monotonic=10.0)
        self.assertFalse(telemetry.is_fresh(2.0, now=12.5))
        self.assertTrue(telemetry.is_fresh(2.0, now=11.5))

    def test_serial_frame_fixed_fields(self):
        telemetry = Telemetry(
            connected=True,
            speed_mps=20,
            engine_rpm=1450,
            gear=8,
            fuel_l=120.5,
            remaining_distance_m=25000,
            odometer_km=12345,
            fuel_consumption_l_km=0.28,
            water_temperature_c=88,
            received_monotonic=100,
        )
        fields = telemetry.serial_frame().rstrip("\n").split(",")
        self.assertEqual(fields, ["T", "1", "72.0", "1450", "8", "120.5", "25.00", "12345.0", "0.28", "88.0"])

if __name__ == "__main__":
    unittest.main()
