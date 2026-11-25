from pathlib import Path
import sys
import traceback

if __name__ == "__main__":
    try:
        from app import TelemetryApp
        TelemetryApp().run()
    except Exception:
        executable_dir = Path(sys.executable).resolve().parent
        log_path = executable_dir / "startup_error.log"
        try:
            log_path.write_text(traceback.format_exc(), encoding="utf-8")
        except Exception:
            pass
        raise
