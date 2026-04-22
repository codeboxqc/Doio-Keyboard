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
    echo  Usage:
    echo    map.exe design.json                    ^<- auto flash me.json
    echo    map.exe design.json me.json            ^<- flash explicit file
    echo    map.exe design.json --dump             ^<- backup keyboard
    echo    map.exe design.json --export           ^<- export as me.json ^(for Editor^)
    echo    map.exe design.json --restore bak.json ^<- restore backup
    echo    map.exe --scan                         ^<- list HID devices
    echo.
    echo  Supported keyboards:
    echo    KB16-01     kb16-01.json
    echo    KB16B-02    kb16b-02.json
    echo    KB16B-02V2  kb16b-02_v2.json
    echo    KB64G-01    kb64g-01_v2.json
) else (
    echo [ERROR] Build failed - map.exe not found in dist\
)

pause
