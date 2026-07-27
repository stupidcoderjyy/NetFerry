@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "EXE=%SCRIPT_DIR%NetFerry.exe"
set "DATA_DIR=%SCRIPT_DIR%data"

if not exist "%EXE%" (
    echo [ERROR] NetFerry.exe not found in %SCRIPT_DIR%
    exit /b 1
)

if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"

echo === NetFerry ===
echo Data dir: %DATA_DIR%

start "inside"  "%EXE%" --config="%SCRIPT_DIR%example_in.json"
start "outside" "%EXE%" --config="%SCRIPT_DIR%example_out.json"

echo.
echo [inside]  port 8080  (started)
echo [outside] ^> target_server  (started)
echo.
echo Test: curl -X POST http://localhost:8080/api/test -d "hello"
echo Stop:  stop.bat
echo.
echo Press any key to stop all gateways...
pause > nul

taskkill /FI "IMAGENAME eq NetFerry.exe" /F 2>nul
echo Stopped.