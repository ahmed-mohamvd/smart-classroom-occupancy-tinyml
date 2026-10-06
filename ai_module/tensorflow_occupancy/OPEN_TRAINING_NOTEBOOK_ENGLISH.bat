@echo off
setlocal
cd /d "%~dp0..\.."

if not exist ".venv-tf\Scripts\python.exe" (
  echo ERROR: TensorFlow environment was not found.
  pause
  exit /b 1
)

set "IPYTHONDIR=%CD%\ai_module\tensorflow_occupancy\.jupyter\ipython"
set "JUPYTER_CONFIG_DIR=%CD%\ai_module\tensorflow_occupancy\.jupyter\config"
set "JUPYTER_DATA_DIR=%CD%\ai_module\tensorflow_occupancy\.jupyter\data"
set "JUPYTER_RUNTIME_DIR=%CD%\ai_module\tensorflow_occupancy\.jupyter\runtime"

echo Opening the English Occupancy AI training notebook...
echo Select Run -^> Run All Cells to repeat the training.
echo.

".venv-tf\Scripts\python.exe" -m jupyter lab "ai_module\tensorflow_occupancy\training_walkthrough_english.ipynb"

endlocal
