#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../common/packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 55000

int main()
{
    WSADATA wsaData;
    SOCKET clientSocket;

    sockaddr_in serverAddr{};

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server\n";

    Packet hello{};
    hello.packetType = HELLO;
    hello.sequenceNumber = 1;
    hello.payloadLength = 0;

    send(clientSocket, (char*)&hello, sizeof(Packet), 0);

    Packet response{};
    int bytesReceived = recv(clientSocket, (char*)&response, sizeof(Packet), 0);

    if (bytesReceived <= 0)
    {
        std::cout << "Server disconnected\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (response.packetType == VERIFY)
    {
        std::cout << "Connection verified\n";
    }

    // GET_STATUS request
    Packet status{};
    status.packetType = GET_STATUS;
    status.sequenceNumber = 2;
    status.payloadLength = 0;

    send(clientSocket, (char*)&status, sizeof(Packet), 0);

    // Receive response
    Packet statusResponse{};
    bytesReceived = recv(clientSocket, (char*)&statusResponse, sizeof(Packet), 0);

    if (bytesReceived > 0)
    {
      std::string statusMsg(statusResponse.payload, statusResponse.payloadLength);
      std::cout << "Server Status: " << statusMsg << std::endl;
    }

    // DOWNLOAD_TELEMETRY request
    Packet download{};
    download.packetType = DOWNLOAD_TELEMETRY;
    download.sequenceNumber = 3;
    download.payloadLength = 0;

    send(clientSocket, (char*)&download, sizeof(Packet), 0);

    // Open file
    std::ofstream out("download_telemetry.bin", std::ios::binary);
    
    if (!out.is_open())
    {
      std::cout << "Failed to open file\n";
      return 1;
    }

    int totalBytesReceived = 0;

    while (true)
    {
       Packet data{};
       int bytes = recv(clientSocket, (char*)&data, sizeof(Packet), 0);

        if (bytes <= 0)
        {
          break;
        }

        std::cout << "Received packet seq: " << data.sequenceNumber << std::endl;
        
         if (data.packetType == DATA)
        {
          out.write(data.payload, data.payloadLength);
          totalBytesReceived += data.payloadLength;
        }
        
        else if (data.packetType == ACK)
        {
           break; // transfer complete
        }
    }

    std::cout << "Total bytes received: " << totalBytesReceived << std::endl;

    out.close();
    if (totalBytesReceived > 0)
    {
        std::cout << "Telemetry downloaded successfully\n";
    }
    else
    {
        std::cout << "Telemetry download failed or file was empty\n";
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}