from pathlib import Path


project_root = Path(SPECPATH).parents[1]
web_root = project_root / "web"

a = Analysis(
    [str(web_root / "server.py")],
    pathex=[str(web_root)],
    binaries=[],
    datas=[
        (str(web_root / "index.html"), "."),
        (str(project_root / "VERSION"), "."),
    ],
    hiddenimports=["serial", "serial.tools.list_ports_windows"],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name="LotSensor",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch="x86_64",
)
