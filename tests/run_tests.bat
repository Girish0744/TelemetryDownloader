@echo off
echo ============================================
echo   Building Packet Unit Tests (Catch2 v2)
echo ============================================
echo.

g++ -std=c++11 -Wall -o test_packets.exe test_main.cpp test_packets.cpp

if %errorlevel% neq 0 (
    echo.
    echo [FAIL] Packet test build failed!
    exit /b 1
)

echo [OK] Packet tests build succeeded.
echo.
echo ============================================
echo   Running Packet Unit Tests
echo ============================================
echo.

test_packets.exe --reporter compact

if %errorlevel% neq 0 (
    echo.
    echo [FAIL] Packet tests failed!
    exit /b 1
)

echo.
echo ============================================
echo   Building Socket Integration Tests
echo ============================================
echo.

g++ -std=c++11 -Wall -o test_socket.exe test_main.cpp test_socket.cpp -lws2_32

if %errorlevel% neq 0 (
    echo.
    echo [FAIL] Socket test build failed!
    exit /b 1
)

echo [OK] Socket tests build succeeded.
echo.
echo ============================================
echo   Running Socket Integration Tests
echo ============================================
echo.

test_socket.exe --reporter compact

if %errorlevel% neq 0 (
    echo.
    echo [FAIL] Socket tests failed!
    exit /b 1
)

echo.
echo ============================================
echo   All Tests Passed!
echo ============================================
