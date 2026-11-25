import tkinter as tk
from tkinter import ttk, messagebox
from pathlib import Path
import serial.tools.list_ports

from config import AppConfig, SettingsStore
from service import TelemetryService

class TelemetryApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("ETS2 / ATS Telemetry")
        self.root.geometry("620x520")

        config_dir = Path(__import__("sys").executable).resolve().parent
        if not getattr(__import__("sys"), "frozen", False):
            config_dir = Path(__file__).resolve().parent
        self.settings = SettingsStore(config_dir / "settings.json")
        self.config = self.settings.load()
        self.service = TelemetryService(self.config)

        self._build_ui()
        self._refresh_ports()
        self._refresh()

    def _build_ui(self):
        frame = ttk.Frame(self.root, padding=12)
        frame.pack(fill="both", expand=True)

        controls = ttk.LabelFrame(frame, text="Connection", padding=8)
        controls.pack(fill="x")

        ttk.Label(controls, text="UDP host").grid(row=0, column=0, sticky="w")
        self.udp_host = tk.StringVar(value=self.config.udp_host)
        ttk.Entry(controls, textvariable=self.udp_host, width=14).grid(row=0, column=1)

        ttk.Label(controls, text="UDP port").grid(row=0, column=2, sticky="w", padx=(8, 0))
        self.udp_port = tk.StringVar(value=str(self.config.udp_port))
        ttk.Entry(controls, textvariable=self.udp_port, width=8).grid(row=0, column=3)

        ttk.Label(controls, text="COM port").grid(row=1, column=0, sticky="w", pady=(8, 0))
        self.com_port = tk.StringVar(value=self.config.serial_port)
        self.com_combo = ttk.Combobox(controls, textvariable=self.com_port, width=20, state="readonly")
        self.com_combo.grid(row=1, column=1, pady=(8, 0))

        ttk.Label(controls, text="Baud").grid(row=1, column=2, sticky="w", padx=(8, 0), pady=(8, 0))
        self.baud = tk.StringVar(value=str(self.config.serial_baud))
        ttk.Entry(controls, textvariable=self.baud, width=8).grid(row=1, column=3, pady=(8, 0))

        buttons = ttk.Frame(controls)
        buttons.grid(row=2, column=0, columnspan=4, pady=(10, 0), sticky="w")
        ttk.Button(buttons, text="Start", command=self.start).pack(side="left")
        ttk.Button(buttons, text="Stop", command=self.stop).pack(side="left", padx=6)
        ttk.Button(buttons, text="Refresh COM", command=self._refresh_ports).pack(side="left")

        status = ttk.LabelFrame(frame, text="Status", padding=8)
        status.pack(fill="x", pady=(10, 0))
        self.udp_status = tk.StringVar(value="Stopped")
        self.serial_status = tk.StringVar(value="Disconnected")
        ttk.Label(status, text="UDP:").grid(row=0, column=0, sticky="w")
        ttk.Label(status, textvariable=self.udp_status).grid(row=0, column=1, sticky="w")
        ttk.Label(status, text="Arduino:").grid(row=1, column=0, sticky="w")
        ttk.Label(status, textvariable=self.serial_status).grid(row=1, column=1, sticky="w")

        telemetry = ttk.LabelFrame(frame, text="Telemetry", padding=8)
        telemetry.pack(fill="both", expand=True, pady=(10, 0))
        self.game = tk.StringVar(value="Unavailable")
        self.speed = tk.StringVar(value="Unavailable")
        self.rpm = tk.StringVar(value="Unavailable")
        self.gear = tk.StringVar(value="Unavailable")
        self.fuel = tk.StringVar(value="Unavailable")
        self.remaining = tk.StringVar(value="Unavailable")
        self.odometer = tk.StringVar(value="Unavailable")
        self.consumption = tk.StringVar(value="Unavailable")
        self.temperature = tk.StringVar(value="Unavailable")

        labels = [
            ("Game", self.game), ("Speed (km/h)", self.speed), ("Engine RPM", self.rpm),
            ("Gear", self.gear), ("Fuel (L)", self.fuel), ("Remaining (km)", self.remaining),
            ("Odometer (km)", self.odometer), ("Consumption (L/100 km)", self.consumption),
            ("Temperature (°C)", self.temperature),
        ]
        for row, (label, variable) in enumerate(labels):
            ttk.Label(telemetry, text=label + ":", font=("Segoe UI", 10, "bold")).grid(row=row, column=0, sticky="w", pady=2)
            ttk.Label(telemetry, textvariable=variable).grid(row=row, column=1, sticky="w", padx=(12, 0), pady=2)

        self.root.protocol("WM_DELETE_WINDOW", self.stop_and_close)

    def _refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.com_combo["values"] = ports
        if ports and not self.com_port.get():
            self.com_port.set(ports[0])

    def _read_config(self) -> AppConfig:
        return AppConfig(
            udp_host=self.udp_host.get().strip(),
            udp_port=int(self.udp_port.get()),
            serial_port=self.com_port.get().strip(),
            serial_baud=int(self.baud.get()),
            serial_reconnect_seconds=self.config.serial_reconnect_seconds,
            telemetry_stale_seconds=self.config.telemetry_stale_seconds,
        )

    def start(self):
        try:
            config = self._read_config()
            config.validate()
            self.settings.save(config)
            self.config = config
            self.service = TelemetryService(config)
            self.service.start()
        except (ValueError, OSError) as exc:
            messagebox.showerror("Configuration error", str(exc))

    def stop(self):
        self.service.stop()

    def stop_and_close(self):
        self.stop()
        self.root.destroy()

    def _show(self, value):
        return "Unavailable" if value is None else str(value)

    def _refresh(self):
        snapshot = self.service.snapshot()
        telemetry = snapshot.telemetry
        self.udp_status.set(snapshot.udp_status)
        self.serial_status.set(snapshot.serial_status)
        self.game.set(self._show(telemetry.game_id))
        self.speed.set(self._show(round(telemetry.speed_kmh, 1)) if telemetry.speed_kmh is not None else "Unavailable")
        self.rpm.set(self._show(round(telemetry.engine_rpm, 0)) if telemetry.engine_rpm is not None else "Unavailable")
        self.gear.set(self._show(telemetry.gear))
        self.fuel.set(self._show(round(telemetry.fuel_l, 1)) if telemetry.fuel_l is not None else "Unavailable")
        remaining_km = None if telemetry.remaining_distance_m is None else telemetry.remaining_distance_m / 1000.0
        self.remaining.set(self._show(round(remaining_km, 1)) if remaining_km is not None else "Unavailable")
        self.odometer.set(self._show(round(telemetry.odometer_km, 1)) if telemetry.odometer_km is not None else "Unavailable")
        consumption = telemetry.fuel_consumption_l_km
        self.consumption.set(self._show(round(consumption, 2)) if consumption is not None else "Unavailable")
        self.temperature.set(self._show(round(telemetry.water_temperature_c, 1)) if telemetry.water_temperature_c is not None else "Unavailable")
        self.root.after(250, self._refresh)

    def run(self):
        self.root.mainloop()
