@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if not defined HIK_YOLO_PYTHON set "HIK_YOLO_PYTHON=python"
set "YOLO_CONFIG_DIR=%SCRIPT_DIR%.ultralytics"

where "%HIK_YOLO_PYTHON%" >nul 2>nul
if errorlevel 1 (
    echo Python was not found via HIK_YOLO_PYTHON="%HIK_YOLO_PYTHON%".
    echo Example:
    echo   set HIK_YOLO_PYTHON=E:\Anaconda\envs\hik_yolov8\python.exe
    pause
    exit /b 1
)

echo Using Python: %HIK_YOLO_PYTHON%
if not exist "%YOLO_CONFIG_DIR%" mkdir "%YOLO_CONFIG_DIR%"
"%HIK_YOLO_PYTHON%" -m pip install --upgrade pip
"%HIK_YOLO_PYTHON%" -m pip install -r "%SCRIPT_DIR%requirements-windows.txt"
if errorlevel 1 (
    echo Failed to install dependencies.
    pause
    exit /b 1
)

"%HIK_YOLO_PYTHON%" "%SCRIPT_DIR%evaluate_vehicle_test.py" --device cpu --save-predictions --save-txt
if errorlevel 1 (
    echo The test evaluation exited with an error.
    pause
    exit /b 1
)

pause
