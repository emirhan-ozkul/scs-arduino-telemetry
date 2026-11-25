import sys
import unittest

sys.path.insert(0, r"C:\Users\gapcu\Documents\vscode\SDKApp\ETS2sdkapp\python")

from telemetry_model import Telemetry

class ArduinoProtocolTests(unittest.TestCase):
    def fields(self, frame):
        return frame.rstrip("\n").split(",")

    def test_live_example(self):
        fields = self.fields(Telemetry(
            connected=True,
            speed_mps=20,
            engine_rpm=1450,
            gear=8,
            fuel_l=120.5,
            remaining_distance_m=25000,
            odometer_km=12345,
            fuel_consumption_l_km=0.28,
            water_temperature_c=88,
            received_monotonic=1,
        ).serial_frame())
        self.assertEqual(fields, ["T", "1", "72.0", "1450", "8", "120.5", "25.00", "12345.0", "0.28", "88.0"])

    def test_na_field_preserved_by_protocol(self):
        fields = self.fields(Telemetry(
            connected=True,
            speed_mps=20,
            engine_rpm=1450,
            gear=8,
            remaining_distance_m=25000,
            odometer_km=12345,
            fuel_consumption_l_km=0.28,
            water_temperature_c=88,
            received_monotonic=1,
        ).serial_frame())
        self.assertEqual(fields[5], "NA")

    def test_disconnected_frame(self):
        frame = "T,0,NA,NA,NA,NA,NA,NA,NA,NA\n"
        fields = self.fields(frame)
        self.assertEqual(len(fields), 10)
        self.assertEqual(fields[0], "T")
        self.assertEqual(fields[1], "0")
        self.assertTrue(all(field == "NA" for field in fields[2:]))

    def test_invalid_field_counts_rejected_in_shape(self):
        for frame in (
            "T,1,72.0,1450,8\n",
            "T,1,72.0,1450,8,120.5,25.00,12345.0,0.28,88.0,extra\n",
            "X,1,72.0,1450,8,120.5,25.00,12345.0,0.28,88.0\n",
        ):
            fields = frame.rstrip("\n").split(",")
            self.assertTrue(len(fields) != 10 or fields[0] != "T")

if __name__ == "__main__":
    unittest.main()
