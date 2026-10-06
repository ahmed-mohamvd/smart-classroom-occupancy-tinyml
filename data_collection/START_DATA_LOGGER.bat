@echo off
setlocal
cd /d "%~dp0.."

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: Project Python environment was not found.
  echo Open the Codex task and ask to prepare the data logger environment.
  pause
  exit /b 1
)

echo STM32 data logger - COM8 at 115200 baud
echo Close STM32CubeIDE Serial Monitor before continuing.
echo.
echo Button 1 selects EMPTY, LOW, or HIGH.
echo Button 2 starts and stops a recording session.
echo Type Q after stopping the session to close this program safely.
echo.

".venv\Scripts\python.exe" "data_collection\serial_data_logger.py" --port COM8 --baud 115200

echo.
echo Logger closed. CSV files are in data_collection\collected_data\raw
pause
