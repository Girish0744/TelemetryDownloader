# Testing Guide — TelemetryDownloader-Group2

This document is a comprehensive guide for running and verifying all tests in the project.

---

## Overview

The project uses a **three-tier testing strategy** to ensure correctness at every level:

| Tier | Purpose | Tool | File |
|------|---------|------|------|
| **Unit Testing** | Verifies packet struct layout, enum values, payload handling, and serialization/deserialization round-trips | Catch2 v2 | `tests/test_packets.cpp` |
| **Integration Testing** | Verifies Winsock socket creation, bind, listen, connect, accept, and a full packet exchange over TCP | Catch2 v2 | `tests/test_socket.cpp` |
| **System Testing** | Runs server and client end-to-end, transfers a 1MB telemetry file, and verifies byte-exact integrity | Batch script | `tests/test_system.bat` |

---

## Prerequisites

Before running the tests, make sure you have the following:

### 1. g++ (MinGW)

The project is compiled with **g++ from MinGW**. Verify it is installed and accessible from the command line:

```
g++ --version
```

You should see output similar to:

```
g++.exe (MinGW.org GCC-6.3.0-1) 6.3.0
```

If you do not see this, refer to the [Troubleshooting](#troubleshooting) section below.

### 2. Catch2 v2 Header

The single-header testing framework `catch.hpp` (v2.13.10) should already be present in the `tests/` folder. Verify it exists:

```
dir tests\catch.hpp
```

If it is missing, download it from the official release:

```
curl -L -o tests/catch.hpp https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp
```

### 3. Telemetry File (for System Test only)

The system test requires `server/telemetry.bin` to be exactly **1,048,576 bytes** (1 MB). Verify it exists:

```
dir server\telemetry.bin
```

If it is missing, generate it with:

```
fsutil file createnew server\telemetry.bin 1048576
```

---

## Step-by-Step Instructions

### Running Unit and Integration Tests

The `run_tests.bat` script builds and runs both the unit tests and integration tests in sequence.

**Step 1:** Open a terminal (Command Prompt or PowerShell) and navigate to the `tests/` folder:

```
cd tests
```

**Step 2:** Run the batch script:

```
run_tests.bat
```

**Expected output:**

```
============================================
  Building Packet Unit Tests (Catch2 v2)
============================================

[OK] Packet tests build succeeded.

============================================
  Running Packet Unit Tests
============================================

Passed all 24 test cases with 104 assertions.

============================================
  Building Socket Integration Tests
============================================

[OK] Socket tests build succeeded.

============================================
  Running Socket Integration Tests
============================================

Passed all 6 test cases with 26 assertions.

============================================
  All Tests Passed!
============================================
```

You can also build and run each test executable individually:

```
:: Unit tests only
g++ -std=c++11 -Wall -o test_packets.exe test_main.cpp test_packets.cpp
.\test_packets.exe

:: Integration tests only
g++ -std=c++11 -Wall -o test_socket.exe test_main.cpp test_socket.cpp -lws2_32
.\test_socket.exe
```

**Useful Catch2 options:**

| Command | Description |
|---------|-------------|
| `.\test_packets.exe -s` | Verbose output showing every assertion |
| `.\test_packets.exe --list-tests` | List all registered test case names |
| `.\test_socket.exe "[socket]"` | Run only tests tagged with `[socket]` |
| `.\test_packets.exe "[serialize]"` | Run only serialization round-trip tests |
| `.\test_packets.exe "[protocol]"` | Run only protocol pattern tests |
| `.\test_packets.exe "[edge]"` | Run only edge case tests |

---

### Running the System Test (1MB File Transfer)

The system test automates the full end-to-end workflow: starting the server, running the client, transferring 1 MB of telemetry data, and verifying the result.

**Step 1:** Make sure `server/server.exe` and `client/client.exe` are already built. If not, build them first:

```
g++ server/server.cpp -o server/server.exe -lws2_32
g++ client/client.cpp -o client/client.exe -lws2_32
```

**Step 2:** From the **project root directory**, run:

```
tests\test_system.bat
```

**Expected output:**

```
============================================================
  System Test: End-to-End 1MB Telemetry File Transfer
============================================================

[Step 1] Checking server\telemetry.bin...
  Source file size: 1048576 bytes
  [OK] Source file is exactly 1,048,576 bytes.

[Step 2] Cleaning up old download...
  [OK] Ready for fresh download.

[Step 3] Starting server.exe...
  [OK] Server started in background.

[Step 4] Waiting 2 seconds for server to initialize...
  [OK] Proceeding.

[Step 5] Running client.exe to download telemetry...
  Connected to server
  Connection verified
  Server Status: Telemetry Ready
  Received packet seq: 1
  ...
  Received packet seq: 1024
  Total bytes received: 1048576
  Telemetry downloaded successfully

  [OK] Client finished successfully.

[Step 6] Stopping server...
  [OK] Server stopped.

[Step 7] Verifying downloaded file...
  Downloaded file size: 1048576 bytes
  Expected file size:   1048576 bytes

============================================================
  [PASS] System test PASSED!
  Both files are exactly 1,048,576 bytes.
============================================================
```

---

## Verification

### How the 1MB File Transfer is Verified

The system test verifies the transfer in two ways:

**1. Byte count comparison:**

The script checks that both files are exactly **1,048,576 bytes**:

- `server\telemetry.bin` — the source file (verified before transfer)
- `client\download_telemetry.bin` — the downloaded file (verified after transfer)

**2. Manual verification (optional):**

You can also verify the file sizes manually at any time using PowerShell:

```powershell
(Get-Item server\telemetry.bin).Length
(Get-Item client\download_telemetry.bin).Length
```

Both commands should output:

```
1048576
```

Or using Command Prompt:

```cmd
for %A in ("server\telemetry.bin") do echo %~zA
for %A in ("client\download_telemetry.bin") do echo %~zA
```

---

## Troubleshooting

### g++ is not recognized / not in PATH

If you see `'g++' is not recognized as an internal or external command`, MinGW's `bin` directory is not in your system PATH.

**Fix — Option 1: Add MinGW to PATH permanently**

1. Open **Start Menu** → search for **"Environment Variables"** → click **"Edit the system environment variables"**
2. Click **"Environment Variables..."**
3. Under **System variables**, find **Path** and click **Edit**
4. Click **New** and add the path to your MinGW bin folder, for example:
   ```
   C:\MinGW\bin
   ```
5. Click **OK** on all dialogs
6. **Close and reopen** your terminal for changes to take effect
7. Verify with `g++ --version`

**Fix — Option 2: Use the full path temporarily**

If you don't want to modify PATH, use the full path to g++ directly:

```
C:\MinGW\bin\g++ --version
C:\MinGW\bin\g++ -std=c++11 -Wall -o tests\test_packets.exe tests\test_main.cpp tests\test_packets.cpp
```

### Port 55000 is already in use

If the integration tests or system test fail with a bind error, another process may be using port 55000.

Check what is using the port:

```
netstat -ano | findstr :55000
```

Kill the process if needed (replace `<PID>` with the process ID from the output):

```
taskkill /F /PID <PID>
```

### System test fails — server.exe not found

Make sure the server and client executables are built before running the system test:

```
g++ server/server.cpp -o server/server.exe -lws2_32
g++ client/client.cpp -o client/client.exe -lws2_32
```

### System test fails — telemetry.bin not found

Generate the 1MB telemetry file:

```
fsutil file createnew server\telemetry.bin 1048576
```

### catch.hpp compilation is slow

The first build takes ~15–30 seconds because Catch2's single header is large (~657KB). Subsequent incremental builds are faster because `test_main.cpp` (which includes Catch2's `main()`) is compiled separately from the test files.
