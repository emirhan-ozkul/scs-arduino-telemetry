# PyInstaller spec for the Windows telemetry application.
from pathlib import Path

block_cipher = None
project = Path(SPECPATH)

analysis = Analysis(
    [str(project / 'main.py')],
    pathex=[str(project)],
    binaries=[],
    datas=[],
    hiddenimports=['serial', 'serial.tools.list_ports'],
    hookspath=[],
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
)
pyz = PYZ(analysis.pure, analysis.zipped_data, cipher=block_cipher)
exe = EXE(pyz, analysis.scripts, [], exclude_binaries=True, name='ETS2Telemetry', debug=False,
          bootloader_ignore_signals=False, strip=False, upx=False, console=False)
coll = COLLECT(exe, analysis.binaries, analysis.zipfiles, analysis.zipped_data,
               name='ETS2Telemetry', strip=False, upx=False)
