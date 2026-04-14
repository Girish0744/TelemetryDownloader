/**
 * @file client.cpp
 * @brief TelemetryDownloader Client Module
 * 
 * Demonstrates Single Responsibility Principle (SRP) and Defensive Programming.
 * Responsible for securely connecting to the TelemetryServer, issuing command handshakes,
 * and actively confirming payloads via Stop-and-Wait ACKs.
 * 
 * @author Authors (Group 2): Dhyey, Girish, Rudra
 */

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <ctime>
#include "../common/packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 55000

/**
 * @class TelemetryLogger
 * @brief Static utility class enforcing cohesive logging. Avoids duplicate code between client and server.
 */
class TelemetryLogger {
public:
    static void logEvent(std::ofstream& logFile, const std::string& msg) {
        std::time_t now = std::time(nullptr);
        char buf[26];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::string formattedMsg = "[" + std::string(buf) + "] " + msg + "\n";
        
        std::cout << formattedMsg; 
        
        if (logFile.is_open()) {
            logFile << formattedMsg;
            logFile.flush();
        }
    }
};

/**
 * @class TelemetryClient
 * @brief Encapsulates socket generation, procedural handshakes, and strict error checking
 */
class TelemetryClient {
private:
    std::ofstream logFile;
    SOCKET clientSocket;

    /**
     * @brief Safe transmission wrapper asserting structural parity
     * @param pkt Packet instance
     * @return truth flag over strict byte matching
     */
    bool sendPacket(const Packet& pkt) {
        int bytesSent = send(clientSocket, (const char*)&pkt, sizeof(Packet), 0);
        if (bytesSent == SOCKET_ERROR || bytesSent != sizeof(Packet)) {
            TelemetryLogger::logEvent(logFile, "DEFENSIVE: Socket boundary failed during dispatch. Expected " + std::to_string(sizeof(Packet)) + " bytes.");
            return false;
        }
        return true;
    }

public:
    TelemetryClient() : clientSocket(INVALID_SOCKET) {
        logFile.open("logs/client_log.txt", std::ios::app);
        TelemetryLogger::logEvent(logFile, "Booting Client Instance...");
    }

    ~TelemetryClient() {
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
        }
        WSACleanup();
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    /**
     * @brief Instantiates Winsock and resolves loopback addressing
     * @return Connection validity flag
     */
    bool connectToServer() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: WSAStartup failure.");
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: Socket acquisition missing.");
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            TelemetryLogger::logEvent(logFile, "CRITICAL: TCP Target (127.0.0.1:55000) actively refused connection.");
            return false;
        }

        TelemetryLogger::logEvent(logFile, "Handshake successful. Connected to core TCP layer.");
        return true;
    }

    /**
     * @brief Procedural HELLO -> VERIFY -> STATUS extraction flow
     * @return true if Server allows the client's identity
     */
    bool performHandshake() {
        Packet hello{};
        hello.packetType = HELLO;
        hello.sequenceNumber = 1;
        hello.payloadLength = 0;

        if (!sendPacket(hello)) return false;
        TelemetryLogger::logEvent(logFile, "Dispatched formal HELLO identity.");

        Packet response{};
        int bytes = recv(clientSocket, (char*)&response, sizeof(Packet), 0);
        if (bytes != sizeof(Packet)) {
            TelemetryLogger::logEvent(logFile, "DEFENSIVE: Remote disconnected or invalid packet boundary size.");
            return false;
        }

        if (response.packetType == VERIFY) {
            TelemetryLogger::logEvent(logFile, "Server granted VERIFY authentication. Proceeding to STATUS.");
        } else {
            return false;
        }

        Packet status{};
        status.packetType = GET_STATUS;
        status.sequenceNumber = 2;
        if (!sendPacket(status)) return false;

        Packet statusResponse{};
        bytes = recv(clientSocket, (char*)&statusResponse, sizeof(Packet), 0);
        if (bytes == sizeof(Packet)) {
            std::string msg(statusResponse.payload, statusResponse.payloadLength);
            TelemetryLogger::logEvent(logFile, "Extracted Metadata JSON/String: " + msg);
            return true;
        }
        return false;
    }

    /**
     * @brief Request telemetry and block-download into local binary 
     * Applies defensive strict bounds per frame mapped back to continuous ACKs.
     */
    void downloadTelemetry() {
        Packet download{};
        download.packetType = DOWNLOAD_TELEMETRY;
        download.sequenceNumber = 3;

        if (!sendPacket(download)) return;
        TelemetryLogger::logEvent(logFile, "Sent DOWNLOAD assertion. Invoking block writer.");

        std::ofstream out("download_telemetry.bin", std::ios::binary);
        if (!out.is_open()) {
            TelemetryLogger::logEvent(logFile, "CRITICAL ERROR: Failed generating localized disk handle (download_telemetry.bin). Safe exiting.");
            return; // Destructor will handle socket kills safely
        }

        int totalBytesReceived = 0;
        TelemetryLogger::logEvent(logFile, "Data frame extraction initialized.");

        while (true) {
            Packet data{};
            int bytes = recv(clientSocket, (char*)&data, sizeof(Packet), 0);

            if (bytes <= 0) {
                TelemetryLogger::logEvent(logFile, "Exception: Host terminated feed mid-stream.");
                break;
            }

            if (bytes != sizeof(Packet)) {
                TelemetryLogger::logEvent(logFile, "DEFENSIVE: Read mismatch (Bytes: " + std::to_string(bytes) + "). Discarding anomaly chunk.");
                break; 
            }

            if (data.packetType == DATA) {
                out.write(data.payload, data.payloadLength);
                totalBytesReceived += data.payloadLength;

                if (data.sequenceNumber % 100 == 1) {
                    TelemetryLogger::logEvent(logFile, "Accepted Packet: " + std::to_string(data.sequenceNumber) + " | Dispatching Sync-ACK.");
                }

                Packet ack{};
                ack.packetType = ACK;
                ack.sequenceNumber = data.sequenceNumber;
                if (!sendPacket(ack)) break;

            } else if (data.packetType == ACK) {
                TelemetryLogger::logEvent(logFile, "Final Summary ACK extracted. Safe extraction achieved.");
                break;
            } else if (data.packetType == ERROR_PACKET) {
                TelemetryLogger::logEvent(logFile, "Received dynamic ERROR abort from remote. Purging context.");
                break;
            }
        }

        out.close();
        if (totalBytesReceived == 1048576) {
             TelemetryLogger::logEvent(logFile, "SUCCESS: Full metric boundary validated -> 1,048,576 bytes constructed securely.");
        } else if (totalBytesReceived > 0) {
             TelemetryLogger::logEvent(logFile, "WARNING: Completed with non-standard file size: " + std::to_string(totalBytesReceived));
        } else {
             TelemetryLogger::logEvent(logFile, "FAILURE: Zero byte collection.");
        }
    }
};

/**
 * @brief Isolates raw environment logic out of the network ADT
 */
int main() {
    TelemetryClient client;
    if (client.connectToServer()) {
        if (client.performHandshake()) {
            client.downloadTelemetry();
        }
    }
    return 0;
}