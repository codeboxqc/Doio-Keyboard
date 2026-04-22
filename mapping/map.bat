@echo off
setlocal EnableDelayedExpansion

:: ─────────────────────────────────────────────────────────────────────────────
:: map.bat  -  Flash / Backup / Restore a VIA keymap on a QMK keyboard
::
:: Usage:
::   map.bat kb16VIA.json                         <- flash test.json (auto)
::   map.bat kb16VIA.json my_keymap.json          <- flash explicit keymap
::   map.bat kb16VIA.json --dump                  <- BACKUP current keyboard
::   map.bat kb16VIA.json --restore backup.json   <- RESTORE a backup
::   map.bat --scan                               <- list all HID devices
:: ─────────────────────────────────────────────────────────────────────────────

set "SCRIPT_DIR=%~dp0"
set "PY_SCRIPT=%SCRIPT_DIR%map.py"

:: ── Locate Python ─────────────────────────────────────────────────────────
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found. Please install Python 3.11+ from https://python.org
    pause & exit /b 1
)

:: ── Ensure dependencies are installed ────────────────────────────────────
python -c "import qmk_via_api" >nul 2>&1
if errorlevel 1 (
    echo [*] Installing qmk-via-api ...
    python -m pip install qmk-via-api --quiet
    if errorlevel 1 (
        echo [ERROR] Failed to install qmk-via-api
        pause & exit /b 1
    )
)

python -c "import hid; hid.enumerate()" >nul 2>&1
if errorlevel 1 (
    echo [*] Installing hidapi (native HID library) ...
    python -m pip uninstall hid -y >nul 2>&1
    python -m pip install hidapi --quiet
)

:: ── Run ───────────────────────────────────────────────────────────────────
python "%PY_SCRIPT%" %*
