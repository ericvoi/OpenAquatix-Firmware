@echo off
echo ===================================
echo STM32H723 DFU Programming Tool
echo ===================================
echo.
echo Looking for DFU device...
dfu-util -l | findstr "Internal Flash"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: No DFU device found!
    echo Please ensure your device is connected and in DFU mode.
    pause
    exit /b 1
)

echo.
echo Device found! Programming firmware...
echo.

dfu-util -a 0 -s 0x08000000:leave -D Debug/UAM.bin 2>&1 | findstr /V "Error during download get_status"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Programming failed!
) else (
    echo.
    echo Programming successful! Device should restart automatically.
)

pause