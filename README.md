# TelemetryDownloader-Group2

## Project Overview

This project is a client-server telemetry downloader system that uses TCP communication and structured packet-based data transfer. The system follows a state-machine design for reliable communication.

---

## Folder Structure

* client/ : client application
* server/ : server application
* common/ : shared packet definitions
* tests/ : test files
* telemetry_data/ : telemetry files
* logs/ : logs for packets

---

## Technologies Used

* C++
* TCP Sockets (Winsock)
* g++ (MinGW)
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

```
g++ tests/test_packets.cpp -o tests/test_packets.exe
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

### Run Tests

```
.\tests\test_packets.exe
```

---

## Features

* TCP client-server communication
* Structured packet-based messaging
* Telemetry data transfer
* Modular architecture
* Basic testing support

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
