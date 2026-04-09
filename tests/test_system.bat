@echo off
echo ============================================================
echo   System Test: End-to-End 1MB Telemetry File Transfer
echo ============================================================
echo.

:: Navigate to project root
cd /d "%~dp0\.."

:: ----- Step 1: Verify source telemetry file exists -----
echo [Step 1] Checking server\telemetry.bin...

if not exist "server\telemetry.bin" (
    echo [FAIL] server\telemetry.bin not found!
    echo        Generate it with: fsutil file createnew server\telemetry.bin 1048576
    exit /b 1
)

for %%A in ("server\telemetry.bin") do set SOURCE_SIZE=%%~zA
echo   Source file size: %SOURCE_SIZE% bytes

if not "%SOURCE_SIZE%"=="1048576" (
    echo [FAIL] server\telemetry.bin is not 1,048,576 bytes!
    exit /b 1
)
echo   [OK] Source file is exactly 1,048,576 bytes.
echo.

:: ----- Step 2: Delete old download if it exists -----
echo [Step 2] Cleaning up old download...
if exist "client\download_telemetry.bin" del "client\download_telemetry.bin"
echo   [OK] Ready for fresh download.
echo.

:: ----- Step 3: Start server in background -----
echo [Step 3] Starting server.exe...
cd server
start /B "" server.exe > nul 2>&1
cd ..
echo   [OK] Server started in background.
echo.

:: ----- Step 4: Wait for server to bind -----
echo [Step 4] Waiting 2 seconds for server to initialize...
timeout /t 2 /nobreak > nul
echo   [OK] Proceeding.
echo.

:: ----- Step 5: Run client -----
echo [Step 5] Running client.exe to download telemetry...
cd client
client.exe
set CLIENT_EXIT=%errorlevel%
cd ..
echo.

if not "%CLIENT_EXIT%"=="0" (
    echo [FAIL] client.exe exited with error code %CLIENT_EXIT%
    taskkill /F /IM server.exe > nul 2>&1
    exit /b 1
)
echo   [OK] Client finished successfully.
echo.

:: ----- Step 6: Stop server -----
echo [Step 6] Stopping server...
taskkill /F /IM server.exe > nul 2>&1
echo   [OK] Server stopped.
echo.

:: ----- Step 7: Verify downloaded file -----
echo [Step 7] Verifying downloaded file...

if not exist "client\download_telemetry.bin" (
    echo [FAIL] client\download_telemetry.bin was not created!
    exit /b 1
)

for %%A in ("client\download_telemetry.bin") do set DOWNLOAD_SIZE=%%~zA

echo   Downloaded file size: %DOWNLOAD_SIZE% bytes
echo   Expected file size:   1048576 bytes

if "%DOWNLOAD_SIZE%"=="1048576" (
    echo.
    echo ============================================================
    echo   [PASS] System test PASSED!
    echo   Both files are exactly 1,048,576 bytes.
    echo ============================================================
) else (
    echo.
    echo ============================================================
    echo   [FAIL] System test FAILED!
    echo   Expected 1,048,576 bytes but got %DOWNLOAD_SIZE% bytes.
    echo ============================================================
    exit /b 1
)
