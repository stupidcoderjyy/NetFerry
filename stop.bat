@echo off
taskkill /FI "IMAGENAME eq NetFerry.exe" /F 2>nul
echo Stopped.