#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <string>
#include "../common/packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 54000

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

            Packet statusPacket{};
            statusPacket.packetType = DATA;
            statusPacket.sequenceNumber = 2;

            const char* msg = "Telemetry Ready";
            strcpy_s(statusPacket.payload, msg);
            statusPacket.payloadLength = strlen(msg);

            send(clientSocket, (char*)&statusPacket, sizeof(Packet), 0);

            std::cout << "Status sent to client\n";
        }

        else if (receivedPacket.packetType == DOWNLOAD_TELEMETRY)
        {
            std::cout << "DOWNLOAD_TELEMETRY received\n";

            // Send multiple DATA packets
            for (int i = 0; i < 5; i++)
            {
                Packet dataPacket{};
                dataPacket.packetType = DATA;
                dataPacket.sequenceNumber = i + 1;

                std::string msg = "DATA_PACKET_" + std::to_string(i + 1);
                memcpy(dataPacket.payload, msg.c_str(), msg.size());
                dataPacket.payloadLength = msg.size();

                send(clientSocket, (char*)&dataPacket, sizeof(Packet), 0);
            }

            // Send ACK (end signal)
            Packet ackPacket{};
            ackPacket.packetType = ACK;
            ackPacket.sequenceNumber = 999;
            ackPacket.payloadLength = 0;

            send(clientSocket, (char*)&ackPacket, sizeof(Packet), 0);

            std::cout << "Telemetry transfer complete\n";
        }
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}