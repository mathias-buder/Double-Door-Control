@echo off
REM Check if Python is installed
python --version >nul 2>&1
IF ERRORLEVEL 1 (
    echo Python is not installed. Downloading Python installer...
    
    REM Set up temporary directory to store the installer
    SET TEMP_DIR=%TEMP%\pyinstaller
    IF NOT EXIST "%TEMP_DIR%" (
        mkdir "%TEMP_DIR%"
    )
    
    REM Download Python installer (64-bit version)
    echo Downloading Python...
    powershell -Command "Invoke-WebRequest -Uri https://www.python.org/ftp/python/3.9.9/python-3.9.9-amd64.exe -OutFile %TEMP_DIR%\python_installer.exe"
    
    IF ERRORLEVEL 1 (
        echo Failed to download Python. Please check your internet connection.
        exit /b 1
    )

    REM Install Python silently
    echo Installing Python...
    "%TEMP_DIR%\python_installer.exe" /quiet InstallAllUsers=1 PrependPath=1 Include_test=0
    
    REM Check if Python was installed successfully
    python --version >nul 2>&1
    IF ERRORLEVEL 1 (
        echo Python installation failed. Please install Python manually and try again.
        exit /b 1
    )
    
    REM Clean up
    del /Q "%TEMP_DIR%\python_installer.exe"
    rmdir "%TEMP_DIR%"
)

REM Check if PlatformIO is installed
pip show platformio >nul 2>&1
IF ERRORLEVEL 1 (
    echo PlatformIO not found. Installing PlatformIO...
    pip install platformio
    IF ERRORLEVEL 1 (
        echo Failed to install PlatformIO. Please check your internet connection.
        exit /b 1
    )
)

REM Search for .bin file in the same directory as the script
SET BIN_FILE=
FOR %%F IN ("%~dp0*.bin") DO (
    SET BIN_FILE=%%F
)

REM Check if a .bin file was found
IF "%BIN_FILE%"=="" (
    echo No .bin file found in the current directory. Please add a .bin file and try again.
    exit /b 1
)

REM Display the .bin file found
echo Found .bin file: %BIN_FILE%

REM Detect connected devices (assumes one device is connected)
platformio device list > temp_devices.txt
FOR /F "tokens=2 delims= " %%A IN ('findstr /C:"Port" temp_devices.txt') DO SET SERIAL_PORT=%%A
del temp_devices.txt

IF "%SERIAL_PORT%"=="" (
    echo No devices found. Please connect your board and try again.
    exit /b 1
)

REM Flash the board using the detected port and the found .bin file
REM You can specify the board type if needed using --board BOARD_NAME
platformio run --target upload --upload-port %SERIAL_PORT% --upload-file %BIN_FILE% --environment uno_r4_minima_release

IF ERRORLEVEL 1 (
    echo Failed to flash the board. Please check the connection and try again.
    exit /b 1
)

echo Board flashed successfully!
exit /b 0
