#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <fstream>
#include "../common/packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 55000

// to create telemetry.bin for testing, run this code once and it will generate a 1MB file filled with random data : TelemetryDownloader-Group2\server>fsutil file createnew telemetry.bin 1048576 

int main()
{
    WSADATA wsaData;
    SOCKET serverSocket;
    SOCKET clientSocket;

    sockaddr_in serverAddr{};
    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR)
    {
        std::cout << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Client connection failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected successfully\n";

    while (true)
    {
        Packet receivedPacket{};
        int bytesReceived = recv(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0);

        if (bytesReceived <= 0)
        {
            std::cout << "Client disconnected\n";
            break;
        }

        if (receivedPacket.packetType == HELLO)
        {
            std::cout << "HELLO received from client\n";

            Packet verifyPacket{};
            verifyPacket.packetType = VERIFY;
            verifyPacket.sequenceNumber = 1;
            verifyPacket.payloadLength = 0;

            send(clientSocket, (char*)&verifyPacket, sizeof(Packet), 0);

            std::cout << "VERIFY sent to client\n";
        }
        else if (receivedPacket.packetType == GET_STATUS)
        {
            std::cout << "GET_STATUS received\n";

            std::ifstream checkFile("telemetry.bin", std::ios::binary | std::ios::ate);

            Packet statusPacket{};
            statusPacket.packetType = DATA;
            statusPacket.sequenceNumber = 2;

            if (checkFile.is_open())
            {
                std::streamsize fileSize = checkFile.tellg();
                checkFile.close();

                const char* msg = "Telemetry Ready";
                strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
                statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
                statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

                std::cout << "Status sent to client. File size: " << fileSize << " bytes\n";
            }
            else
            {
                const char* msg = "Telemetry File Missing";
                strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
                statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
                statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

                std::cout << "Status sent to client. telemetry.bin not found\n";
            }

            send(clientSocket, (char*)&statusPacket, sizeof(Packet), 0);
        }
        else if (receivedPacket.packetType == DOWNLOAD_TELEMETRY)
        {
            std::cout << "DOWNLOAD_TELEMETRY received\n";

            std::ifstream telemetryFile("telemetry.bin", std::ios::binary);

            if (!telemetryFile.is_open())
            {
                std::cout << "Failed to open telemetry.bin\n";

                Packet errorPacket{};
                errorPacket.packetType = ERROR_PACKET;
                errorPacket.sequenceNumber = 0;
                errorPacket.payloadLength = 0;

                send(clientSocket, (char*)&errorPacket, sizeof(Packet), 0);
                continue;
            }

            int sequence = 1;
            long long totalBytesSent = 0;

            while (!telemetryFile.eof())
            {
                Packet dataPacket{};
                dataPacket.packetType = DATA;
                dataPacket.sequenceNumber = sequence++;

                telemetryFile.read(dataPacket.payload, MAX_PAYLOAD_SIZE);
                std::streamsize bytesRead = telemetryFile.gcount();

                if (bytesRead <= 0)
                    break;

                dataPacket.payloadLength = static_cast<uint32_t>(bytesRead);

                send(clientSocket, (char*)&dataPacket, sizeof(Packet), 0);
                totalBytesSent += bytesRead;
            }

            telemetryFile.close();

            Packet ackPacket{};
            ackPacket.packetType = ACK;
            ackPacket.sequenceNumber = sequence;
            ackPacket.payloadLength = 0;

            send(clientSocket, (char*)&ackPacket, sizeof(Packet), 0);

            std::cout << "Telemetry transfer complete. Total bytes sent: "
                      << totalBytesSent << std::endl;
        }
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}