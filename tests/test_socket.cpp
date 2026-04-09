#include "catch.hpp"
#include "../common/packet.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

#define TEST_PORT 55000

// ============================================================================
// Integration Tests: Winsock Socket Creation, Bind, and Listen
// ============================================================================

TEST_CASE("WSAStartup initializes Winsock successfully", "[socket][init]")
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    REQUIRE(result == 0);
    REQUIRE(LOBYTE(wsaData.wVersion) == 2);
    REQUIRE(HIBYTE(wsaData.wVersion) == 2);

    WSACleanup();
}

TEST_CASE("TCP socket creation succeeds", "[socket][create]")
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(sock != INVALID_SOCKET);

    closesocket(sock);
    WSACleanup();
}

TEST_CASE("Bind to local port succeeds", "[socket][bind]")
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(sock != INVALID_SOCKET);

    // Allow port reuse so tests don't fail if socket is in TIME_WAIT
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int bindResult = bind(sock, (sockaddr*)&addr, sizeof(addr));
    REQUIRE(bindResult != SOCKET_ERROR);

    closesocket(sock);
    WSACleanup();
}

TEST_CASE("Listen on bound socket succeeds", "[socket][listen]")
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(sock != INVALID_SOCKET);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int bindResult = bind(sock, (sockaddr*)&addr, sizeof(addr));
    REQUIRE(bindResult != SOCKET_ERROR);

    int listenResult = listen(sock, 5);
    REQUIRE(listenResult != SOCKET_ERROR);

    closesocket(sock);
    WSACleanup();
}

TEST_CASE("Client can connect to a listening server socket", "[socket][connect]")
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // --- Set up server socket ---
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(serverSock != INVALID_SOCKET);

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TEST_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    REQUIRE(bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR);
    REQUIRE(listen(serverSock, 5) != SOCKET_ERROR);

    // --- Set up client socket and connect ---
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(clientSock != INVALID_SOCKET);

    sockaddr_in connectAddr{};
    connectAddr.sin_family = AF_INET;
    connectAddr.sin_port = htons(TEST_PORT);
    connectAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int connectResult = connect(clientSock, (sockaddr*)&connectAddr, sizeof(connectAddr));
    REQUIRE(connectResult != SOCKET_ERROR);

    // --- Server accepts the connection ---
    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);
    SOCKET acceptedSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);
    REQUIRE(acceptedSock != INVALID_SOCKET);

    // --- Cleanup ---
    closesocket(acceptedSock);
    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
}

TEST_CASE("Full socket lifecycle with packet exchange", "[socket][lifecycle]")
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // --- Server setup ---
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(serverSock != INVALID_SOCKET);

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TEST_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    REQUIRE(bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR);
    REQUIRE(listen(serverSock, 5) != SOCKET_ERROR);

    // --- Client connects ---
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(clientSock != INVALID_SOCKET);

    sockaddr_in connectAddr{};
    connectAddr.sin_family = AF_INET;
    connectAddr.sin_port = htons(TEST_PORT);
    connectAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    REQUIRE(connect(clientSock, (sockaddr*)&connectAddr, sizeof(connectAddr)) != SOCKET_ERROR);

    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);
    SOCKET acceptedSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);
    REQUIRE(acceptedSock != INVALID_SOCKET);

    // --- Client sends a HELLO packet ---
    Packet hello{};
    hello.packetType = HELLO;
    hello.sequenceNumber = 1;
    hello.payloadLength = 0;

    int sent = send(clientSock, (char*)&hello, sizeof(Packet), 0);
    REQUIRE(sent == sizeof(Packet));

    // --- Server receives the HELLO packet ---
    Packet received{};
    int recvd = recv(acceptedSock, (char*)&received, sizeof(Packet), 0);
    REQUIRE(recvd == sizeof(Packet));
    REQUIRE(received.packetType == HELLO);
    REQUIRE(received.sequenceNumber == 1);
    REQUIRE(received.payloadLength == 0);

    // --- Cleanup ---
    closesocket(acceptedSock);
    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
}
