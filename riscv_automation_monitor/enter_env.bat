@echo off
SETLOCAL
:: Change directory to the script's location to handle OneDrive paths with spaces
cd /d "%~dp0"

echo [INFO] Entering RISC-V testing virtual environment...
echo [INFO] Execution Policy set to Bypass (this session only)

:: Launch PowerShell, activate the virtual environment, and keep the window open (-NoExit)
powershell -NoExit -ExecutionPolicy Bypass -Command "& {.\riscv_test_env\Scripts\Activate.ps1}"