import sys
import unittest

sys.path.insert(0, r"C:\Users\gapcu\Documents\vscode\SDKApp\ETS2sdkapp\python")

from telemetry_model import Telemetry

class SerialProtocolTests(unittest.TestCase):
    def test_fresh_frame_field_count(self):
        telemetry = Telemetry(connected=True, speed_mps=10, engine_rpm=1000, gear=5,
                              fuel_l=100, remaining_distance_m=1000, odometer_km=500,
                              fuel_consumption_l_km=0.25, water_temperature_c=80,
                              received_monotonic=1)
        fields = telemetry.serial_frame().rstrip("\n").split(",")
        self.assertEqual(len(fields), 10)
        self.assertEqual(fields[0], "T")
        self.assertEqual(fields[1], "1")

    def test_unavailable_fields_use_na(self):
        frame = Telemetry().serial_frame()
        self.assertIn("NA", frame)

if __name__ == "__main__":
    unittest.main()
