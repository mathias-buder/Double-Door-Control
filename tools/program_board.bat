@echo off
REM Set the path to DFU programmer application
SET DFU_PROGRAMMER_PATH="dfu-util.exe"

REM Search for .bin file in the same directory as the script
SET BIN_FILE=
FOR %%F IN ("%~dp0*.bin") DO (
    SET BIN_FILE=%%F
)

REM Check if a .bin file was found
IF "%BIN_FILE%"=="" (
    echo No .bin file found in the current directory. Please add a .bin file and try again.
    pause
    exit /b 1
)

REM Display the .bin file found
echo Found .bin file: %BIN_FILE%

@REM REM Detect connected devices (assumes one device is connected)
@REM platformio device list > temp_devices.txt
@REM FOR /F "tokens=2 delims= " %%A IN ('findstr /C:"Port" temp_devices.txt') DO SET SERIAL_PORT=%%A
@REM del temp_devices.txt

@REM IF "%SERIAL_PORT%"=="" (
@REM     echo No devices found. Please connect your board and try again.
@REM     exit /b 1
@REM )

REM This batch script is used to program a board using the dfu-programmer tool.
REM 
REM The script performs the following actions:
REM 1. Uses the dfu-programmer tool to program the board.
REM 2. The -v flag enables verbose output.
REM 3. The -a 0 option specifies the address to start programming from.
REM 4. The -D option specifies the binary file to be programmed.
REM 5. The --reset option resets the device after programming.
REM 
REM Environment Variables:
REM - DFU_PROGRAMMER_PATH: Path to the dfu-programmer executable.
REM - BIN_FILE: Path to the binary file to be programmed.
echo Flashing the board...
%DFU_PROGRAMMER_PATH% -v -a 0 -D %BIN_FILE% --reset


IF ERRORLEVEL 1 (
    echo Failed to flash the board. Please check the connection and try again.
    pause
    exit /b 1
)

echo Board flashed successfully!

rem Make the user press a key to close the window
pause
exit /b 0