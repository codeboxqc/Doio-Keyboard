@echo off
:: ============================================================
::  build.bat  -  Build map.exe from map.py
::  Works with Python 3.8 - 3.14
:: ============================================================

echo.
echo === Python version ===
python --version

echo.
echo === Installing dependencies ===
pip install hidapi qmk-via-api pyinstaller --quiet

echo.
echo === Building map.exe ===
python -m PyInstaller --onefile --console --name map map.py

echo.
if exist dist\map.exe (
    echo [+] Done!  dist\map.exe is ready.
    echo.
    echo     Copy these files next to map.exe:
    echo       kb16VIA.json
    echo       test.json
    echo.
    echo     Commands:
    echo       map.exe kb16VIA.json --dump                      <- backup FIRST!
    echo       map.exe kb16VIA.json test.json                   <- flash new keymap
    echo       map.exe kb16VIA.json --restore backup_XYZ.json  <- restore backup
    echo       map.exe --scan                                   <- list HID devices
) else (
    echo [ERROR] Build failed - map.exe not found in dist\
)

pause
