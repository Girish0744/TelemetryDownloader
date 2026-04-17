# TelemetryDownloader-Group2

## Project Overview

So, this project is basically a client-server telemetry downloader system. We decided to use TCP communication along with structured packet-based data transfer because it’s much more reliable. I think keeping the connection stable is super important, so that's why we did an explicit Object-Oriented state-machine design to handle the connections securely.

## Folder Structure

Here's how we laid everything out:

* client/ : This is our client application
* server/ : This houses the server application
* common/ : We put shared packet definitions here so both apps can easily use them
* tests/ : All our unit, integration, and system tests
* telemetry_data/ : The actual telemetry files go here
* logs/ : So this will hold all the generated logs for packets during transfers

## Technologies Used

* C++
* TCP Sockets (Winsock)
* g++ (MinGW)
* Catch2 v2 (testing framework)
* VS Code

## Setup Instructions

### 1. Install Required Tools

You'll need a few things first:

* Install Git
* Install Visual Studio Code
* Install MinGW (for the g++ compiler)

### 2. Clone the Repository

To get started, simply clone our code:

```
git clone https://github.com/RudraPatelhere/TelemetryDownloader-Group2.git
cd TelemetryDownloader-Group2
```

### 3. Open Project

Just open the folder up in VS Code and you're good to go!

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

If you want to build all the unit and integration tests at once, just run:

```
cd tests
run_tests.bat
```

Or, if you secretly just want to build them individually:

```
cd tests
g++ -std=c++11 -Wall -o test_packets.exe test_main.cpp test_packets.cpp
g++ -std=c++11 -Wall -o test_socket.exe test_main.cpp test_socket.cpp -lws2_32
```

## Run Instructions

### Run Server

Make sure you run the server first!

```
.\server\server.exe
```

### Run Client

```
.\client\client.exe
```

## Testing

## Final Testing Summary

We made sure to test everything thoroughly! All tests have been verified and are passing smoothly for our final submission.

| Test Level | Test File | Tests | Assertions | Status |
|------------|-----------|-------|------------|--------|
| **Unit Testing** | `test_packets.cpp` | 24 | 104 | ✅ All Passed |
| **Integration Testing** | `test_socket.cpp` | 7 | 30 | ✅ All Passed |
| **System Testing** | `test_system.bat` | 1 (end-to-end) | 1MB file verified | ✅ Passed |

**Total: 32 tests, 134+ assertions — all passing!**

* **Unit Tests** — We checked our packet struct layout, enum values, payload handling, serialization/deserialization round-trips, and edge cases.
* **Integration Tests** — This covers our Winsock socket creation, bind, listen, connect, accept, and our explicit ACK lifecycle over TCP.
* **System Test** — So this will run an automated end-to-end test that starts the server, runs the client, transfers a 1MB telemetry file (precisely 1,048,576 bytes), and verifies byte-exact file integrity.

So yeah, the project essentially has three levels of testing:

### 1. Unit Tests (Packet Serialization)

Tests packet struct layout, enum values, payload handling, and serialization round-trips.

```
cd tests
.\test_packets.exe
```

### 2. Integration Tests (Socket Verification)

Tests Winsock socket creation, bind, listen, connect, and a full packet exchange lifecycle. That's why we did explicit Stop-and-Wait ACK tests here.

```
cd tests
.\test_socket.exe
```

### 3. System Test (End-to-End File Transfer)

Runs `server.exe` and `client.exe` together, transfers the 1MB file, and verifies byte-exact integrity.

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

## Features

I think our implementation covers a lot of solid ground:

* TCP client-server communication
* Structured packet-based messaging
* Telemetry data transfer
* Highly decoupled Object-Oriented modular architecture
* Persistent filesystem logging utility
* Unit testing with Catch2 v2 framework
* Integration testing for Winsock socket operations
* System testing with automated 1MB file transfer verification

## Notes

* Always start the server before the client
* Make sure you pull the latest code before working!
* Shared structures are logically placed in the `common/` folder

## Contributors

* Rudra Patel
* Dhyey
* Girish
