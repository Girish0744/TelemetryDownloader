# TelemetryDownloader-Group2

## Project Overview

This project is a client-server telemetry downloader system that uses TCP communication and structured packet-based data transfer. The system follows a state-machine design for reliable communication.


---

## Folder Structure

* client/ : client application
* server/ : server application
* common/ : shared packet definitions
* tests/ : unit, integration, and system tests
* telemetry_data/ : telemetry files
* logs/ : logs for packets

---

## Technologies Used

* C++
* TCP Sockets (Winsock)
* g++ (MinGW)
* Catch2 v2 (testing framework)
* VS Code

---

## Setup Instructions

### 1. Install Required Tools

* Install Git
* Install Visual Studio Code
* Install MinGW (g++)

### 2. Clone the Repository

```
git clone https://github.com/RudraPatelhere/TelemetryDownloader-Group2.git
cd TelemetryDownloader-Group2
```

### 3. Open Project

* Open the folder in VS Code

---

## Build Instructions

### Build Server

```
g++ server/server.cpp -o server/server.exe -lws2_32
```

### Build Client

```
g++ client/client.cpp -o client/client.exe -lws2_32
```

### Build Tests

Build all unit and integration tests at once:

```
cd tests
run_tests.bat
```

Or build individually:

```
cd tests
g++ -std=c++11 -Wall -o test_packets.exe test_main.cpp test_packets.cpp
g++ -std=c++11 -Wall -o test_socket.exe test_main.cpp test_socket.cpp -lws2_32
```

---

## Run Instructions

### Run Server

```
.\server\server.exe
```

### Run Client

```
.\client\client.exe
```

---

## Testing

---

## Final Testing Summary

All tests have been verified and are passing as of the final submission.

| Test Level | Test File | Tests | Assertions | Status |
|------------|-----------|-------|------------|--------|
| **Unit Testing** | `test_packets.cpp` | 24 | 104 | ✅ All Passed |
| **Integration Testing** | `test_socket.cpp` | 6 | 26 | ✅ All Passed |
| **System Testing** | `test_system.bat` | 1 (end-to-end) | 1MB file verified | ✅ Passed |

**Total: 31 tests, 130+ assertions — all passing.**

* **Unit Tests** — Packet struct layout, enum values, payload handling, serialization/deserialization round-trips, protocol-specific construction patterns, and edge cases.
* **Integration Tests** — Winsock socket creation, bind, listen, connect, accept, and a full packet exchange lifecycle over TCP.
* **System Test** — Automated end-to-end test that starts the server, runs the client, transfers a 1MB telemetry file (1,048,576 bytes), and verifies byte-exact file integrity.

--- 

The project has three levels of testing:

### 1. Unit Tests (Packet Serialization)

Tests packet struct layout, enum values, payload handling, and serialization round-trips.

```
cd tests
.\test_packets.exe
```

### 2. Integration Tests (Socket Verification)

Tests Winsock socket creation, bind, listen, connect, and a full packet exchange lifecycle.

```
cd tests
.\test_socket.exe
```

### 3. System Test (End-to-End File Transfer)

Runs server.exe and client.exe together, transfers a 1MB file, and verifies byte-exact integrity.

```
tests\test_system.bat
```

### Run All Unit + Integration Tests

```
cd tests
run_tests.bat
```

### Useful Test Options

```
.\test_packets.exe -s              :: Verbose output with all assertions
.\test_packets.exe --list-tests    :: List all registered test cases
.\test_socket.exe "[socket]"       :: Run only socket-tagged tests
```

---

## Features

* TCP client-server communication
* Structured packet-based messaging
* Telemetry data transfer
* Modular architecture
* Unit testing with Catch2 v2 framework (24 packet tests, 6 socket tests)
* Integration testing for Winsock socket operations
* System testing with automated 1MB file transfer verification

---

## Notes

* Start server before client
* Always pull latest code before working
* Shared structures are in common/

---

## Contributors

* Rudra Patel
* Dhyey
* Girish
