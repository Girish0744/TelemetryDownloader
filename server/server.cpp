/**
 * @file server.cpp
 * @brief TelemetryDownloader Server Module
 * 
 * Demonstrates Single Responsibility Principle (SRP) and Defensive Programming.
 * Responsible for listening for client connections, managing the connection state machine,
 * and securely feeding requested telemetry data incrementally over TCP.
 * 
 * @author Authors (Group 2): Dhyey, Girish, Rudra
 */

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <fstream>
#include <string>
#include <ctime>
#include "../common/packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 55000

/**
 * @enum ServerState
 * @brief Discrete states for the Server Connection Lifecycle.
 * Restricts out-of-order commands.
 */
enum class ServerState {
    WAITING,       ///< Waiting for initial Hello
    CONNECTED,     ///< Handshake initialized
    READY,         ///< File verified, ready to transfer
    TRANSFERRING,  ///< Active data transmission
    DONE           ///< Complete transmission/teardown
};

/**
 * @class TelemetryLogger
 * @brief Static utility class enforcing cohesive logging across the system
 */
class TelemetryLogger {
public:
    static void logEvent(std::ofstream& logFile, const std::string& msg) {
        std::time_t now = std::time(nullptr);
        char buf[26];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::string formattedMsg = "[" + std::string(buf) + "] " + msg + "\n";
        
        std::cout << formattedMsg; // Print to terminal
        
        if (logFile.is_open()) {
            logFile << formattedMsg;
            logFile.flush();
        }
    }
};

/**
 * @class TelemetryServer
 * @brief ADT wrapping Server responsibilities keeping Network layer decoupled from state logic
 */
class TelemetryServer {
private:
    std::ofstream logFile;
    SOCKET serverSocket;
    SOCKET clientSocket;
    ServerState currentState;

    /**
     * @brief Defensively sends a packet and traps errors to prevent zombie sockets
     * @param pkt The packet structure to dispatch
     * @return true if successful, false if socket failed
     */
    bool sendPacket(const Packet& pkt) {
        int bytesSent = send(clientSocket, (const char*)&pkt, sizeof(Packet), 0);
        if (bytesSent == SOCKET_ERROR || bytesSent != sizeof(Packet)) {
            TelemetryLogger::logEvent(logFile, "ERROR: Structural integrity failure during send()");
            return false;
        }
        return true;
    }

public:
    TelemetryServer() : serverSocket(INVALID_SOCKET), clientSocket(INVALID_SOCKET), currentState(ServerState::WAITING) {
        logFile.open("logs/server_log.txt", std::ios::app);
        TelemetryLogger::logEvent(logFile, "System Boot: Initializing TelemetryServer instance.");
    }

    ~TelemetryServer() {
        shutdown();
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    /**
     * @brief Initialize Winsock and Bind to defined Port
     * @return true on success
     */
    bool initialize() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: WSAStartup failed.");
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: Socket creation failed.");
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: Bind failed. Port may be in use.");
            return false;
        }

        if (listen(serverSocket, 5) == SOCKET_ERROR) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: Listen failed.");
            return false;
        }

        TelemetryLogger::logEvent(logFile, "Server actively listening on port " + std::to_string(PORT));
        return true;
    }

    /**
     * @brief Idly wait securely for a client connection request
     */
    bool acceptClient() {
        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);
        
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            TelemetryLogger::logEvent(logFile, "ERROR: Client connection failed during accept().");
            return false;
        }

        currentState = ServerState::CONNECTED;
        TelemetryLogger::logEvent(logFile, "LIFECYCLE: Client connected successfully. State: CONNECTED");
        return true;
    }

    /**
     * @brief The core processing loop enforcing State Machine flows defensively
     */
    void processClientLoop() {
        while (currentState != ServerState::DONE) {
            Packet receivedPacket{};
            
            // Defensively read an exact structural boundary
            int bytesReceived = recv(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0);

            if (bytesReceived <= 0) {
                TelemetryLogger::logEvent(logFile, "LIFECYCLE: Client disconnected unexpectedly.");
                break;
            }

            if (bytesReceived != sizeof(Packet)) {
                TelemetryLogger::logEvent(logFile, "DEFENSIVE: Received fragmented or maliciously sized packet. Discarding.");
                continue;
            }

            handlePacket(receivedPacket);
        }
    }

private:
    /**
     * @brief Routing logic mapping Packet Types securely to States
     */
    void handlePacket(const Packet& receivedPacket) {
        switch (receivedPacket.packetType) {
            
            case HELLO:
                if (currentState == ServerState::CONNECTED) {
                    TelemetryLogger::logEvent(logFile, "HELLO received. Negotiating sequence...");
                    
                    Packet verifyPacket{};
                    verifyPacket.packetType = VERIFY;
                    verifyPacket.sequenceNumber = 1;
                    verifyPacket.payloadLength = 0;

                    if (sendPacket(verifyPacket)) {
                        TelemetryLogger::logEvent(logFile, "VERIFY sent.");
                    } else {
                        currentState = ServerState::DONE;
                    }
                }
                break;

            case GET_STATUS:
                if (currentState == ServerState::CONNECTED) {
                    std::ifstream checkFile("server/telemetry.bin", std::ios::binary | std::ios::ate);
                    Packet statusPacket{};
                    statusPacket.packetType = DATA;
                    statusPacket.sequenceNumber = 2;

                    if (checkFile.is_open()) {
                        std::streamsize fileSize = checkFile.tellg();
                        checkFile.close();

                        const char* msg = "Telemetry Ready";
                        strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
                        statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
                        statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

                        TelemetryLogger::logEvent(logFile, "STATUS sent. File size identified: " + std::to_string(fileSize) + " bytes");
                        currentState = ServerState::READY;
                    } else {
                        const char* msg = "Telemetry File Missing";
                        strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
                        statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
                        statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

                        TelemetryLogger::logEvent(logFile, "STATUS failure: server/telemetry.bin missing.");
                    }

                    if (!sendPacket(statusPacket)) currentState = ServerState::DONE;
                }
                break;

            case DOWNLOAD_TELEMETRY:
                if (currentState == ServerState::READY) {
                    currentState = ServerState::TRANSFERRING;
                    TelemetryLogger::logEvent(logFile, "State: TRANSFERRING. Booting explicit loop.");
                    executeTransfer();
                } else {
                    TelemetryLogger::logEvent(logFile, "DEFENSIVE: Ignored DOWNLOAD request from invalid state.");
                }
                break;

            default:
                TelemetryLogger::logEvent(logFile, "DEFENSIVE: Unexpected packet " + std::to_string(receivedPacket.packetType) + " received.");
                break;
        }
    }

    /**
     * @brief Safely transfers file enforcing Client ACK dependencies synchronously
     */
    void executeTransfer() {
        std::ifstream telemetryFile("server/telemetry.bin", std::ios::binary);

        if (!telemetryFile.is_open()) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: Lost server/telemetry.bin handle mid-transfer.");
            Packet err{};
            err.packetType = ERROR_PACKET;
            sendPacket(err);
            currentState = ServerState::DONE;
            return;
        }

        int sequence = 1;
        long long totalBytesSent = 0;

        while (!telemetryFile.eof()) {
            Packet dataPacket{};
            dataPacket.packetType = DATA;
            dataPacket.sequenceNumber = sequence++;

            telemetryFile.read(dataPacket.payload, MAX_PAYLOAD_SIZE);
            std::streamsize bytesRead = telemetryFile.gcount();
            if (bytesRead <= 0) break;

            dataPacket.payloadLength = static_cast<uint32_t>(bytesRead);

            if (!sendPacket(dataPacket)) break;
            
            totalBytesSent += bytesRead;

            if ((sequence - 1) % 100 == 1) {
                TelemetryLogger::logEvent(logFile, "Dispatched Frame " + std::to_string(sequence - 1) + ". Awaiting explicit ACK.");
            }

            // Defensively wait for matching sequence ACK
            Packet ackPacket{};
            int ackBytes = recv(clientSocket, (char*)&ackPacket, sizeof(Packet), 0);
            
            if (ackBytes != sizeof(Packet)) {
                TelemetryLogger::logEvent(logFile, "ERROR: Client disconnected or sent mangled ACK frame.");
                break;
            }

            if (ackPacket.packetType == ACK) {
                if ((sequence - 1) % 100 == 1) {
                    TelemetryLogger::logEvent(logFile, "ACK confirmed for seq " + std::to_string(ackPacket.sequenceNumber));
                }
            } else if (ackPacket.packetType == ERROR_PACKET) {
                TelemetryLogger::logEvent(logFile, "REMOTE ERROR: Client aborted transfer.");
                break;
            }
        }

        telemetryFile.close();

        // Teardown Summary Packet
        Packet finalAckPacket{};
        finalAckPacket.packetType = ACK;
        finalAckPacket.sequenceNumber = sequence;
        sendPacket(finalAckPacket);

        TelemetryLogger::logEvent(logFile, "COMPLETION: Total transferred bytes = " + std::to_string(totalBytesSent));
        currentState = ServerState::DONE;
    }

    /**
     * @brief Safe resource de-allocation cleanup (RAII enforcement paradigm)
     */
    void shutdown() {
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
        WSACleanup();
        TelemetryLogger::logEvent(logFile, "Server instance safely dismantled.");
    }
};

/**
 * @brief Entrypoint wrapper keeping runtime environment isolated from ADT variables
 */
int main() {
    TelemetryServer server;
    if (server.initialize()) {
        if (server.acceptClient()) {
            server.processClientLoop();
        }
    }
    return 0;
}