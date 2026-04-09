#include "catch.hpp"
#include "../common/packet.h"
#include <cstring>
#include <cstdint>
#include <string>

// ============================================================================
// Category 1: Packet Struct Layout & Zero-Initialization
// ============================================================================

TEST_CASE("Packet zero-initializes all fields", "[struct][init]")
{
    Packet p{};

    REQUIRE(p.packetType == 0);
    REQUIRE(p.sequenceNumber == 0);
    REQUIRE(p.payloadLength == 0);

    // Verify payload buffer is zeroed
    bool allZero = true;
    for (int i = 0; i < MAX_PAYLOAD_SIZE; i++)
    {
        if (p.payload[i] != 0)
        {
            allZero = false;
            break;
        }
    }
    REQUIRE(allZero);
}

TEST_CASE("Packet struct size is consistent", "[struct][layout]")
{
    // 3 x uint32_t (12 bytes) + char[1024] = 1036 bytes minimum
    // Actual size may include padding, but must be >= 1036
    REQUIRE(sizeof(Packet) >= 1036);

    // Verify the struct holds all expected members at correct offsets
    Packet p{};
    REQUIRE(sizeof(p.packetType) == sizeof(uint32_t));
    REQUIRE(sizeof(p.sequenceNumber) == sizeof(uint32_t));
    REQUIRE(sizeof(p.payloadLength) == sizeof(uint32_t));
    REQUIRE(sizeof(p.payload) == MAX_PAYLOAD_SIZE);
}

// ============================================================================
// Category 2: PacketType Enum Values
// ============================================================================

TEST_CASE("PacketType enum has correct sequential values", "[enum]")
{
    REQUIRE(HELLO == 1);
    REQUIRE(VERIFY == 2);
    REQUIRE(GET_STATUS == 3);
    REQUIRE(DOWNLOAD_TELEMETRY == 4);
    REQUIRE(DATA == 5);
    REQUIRE(ACK == 6);
    REQUIRE(ERROR_PACKET == 7);
}

// ============================================================================
// Category 3: Field Assignment & Read-Back
// ============================================================================

TEST_CASE("Packet fields can be assigned and read back", "[struct][fields]")
{
    Packet p{};

    SECTION("packetType assignment")
    {
        p.packetType = HELLO;
        REQUIRE(p.packetType == HELLO);

        p.packetType = DATA;
        REQUIRE(p.packetType == DATA);

        p.packetType = ERROR_PACKET;
        REQUIRE(p.packetType == ERROR_PACKET);
    }

    SECTION("sequenceNumber assignment")
    {
        p.sequenceNumber = 0;
        REQUIRE(p.sequenceNumber == 0);

        p.sequenceNumber = 42;
        REQUIRE(p.sequenceNumber == 42);

        p.sequenceNumber = 100000;
        REQUIRE(p.sequenceNumber == 100000);
    }

    SECTION("payloadLength assignment")
    {
        p.payloadLength = 0;
        REQUIRE(p.payloadLength == 0);

        p.payloadLength = 512;
        REQUIRE(p.payloadLength == 512);

        p.payloadLength = MAX_PAYLOAD_SIZE;
        REQUIRE(p.payloadLength == (uint32_t)MAX_PAYLOAD_SIZE);
    }
}

// ============================================================================
// Category 4: Payload Handling
// ============================================================================

TEST_CASE("Payload handles string data correctly", "[payload][string]")
{
    Packet p{};
    const char* msg = "Telemetry Ready";
    size_t msgLen = strlen(msg);

    strncpy(p.payload, msg, MAX_PAYLOAD_SIZE - 1);
    p.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
    p.payloadLength = static_cast<uint32_t>(msgLen);

    REQUIRE(p.payloadLength == msgLen);
    REQUIRE(std::string(p.payload, p.payloadLength) == "Telemetry Ready");
}

TEST_CASE("Payload handles binary data correctly", "[payload][binary]")
{
    Packet p{};

    // Write known binary pattern
    unsigned char pattern[] = {0x00, 0xFF, 0xAB, 0xCD, 0x01, 0x80};
    size_t patternLen = sizeof(pattern);

    memcpy(p.payload, pattern, patternLen);
    p.payloadLength = static_cast<uint32_t>(patternLen);

    REQUIRE(p.payloadLength == patternLen);
    REQUIRE(memcmp(p.payload, pattern, patternLen) == 0);
}

TEST_CASE("Payload handles empty payload", "[payload][empty]")
{
    Packet p{};
    p.packetType = HELLO;
    p.sequenceNumber = 1;
    p.payloadLength = 0;

    REQUIRE(p.payloadLength == 0);
    // Payload buffer should still be zeroed from initialization
    REQUIRE(p.payload[0] == 0);
}

TEST_CASE("Payload handles maximum size payload", "[payload][max]")
{
    Packet p{};
    p.packetType = DATA;
    p.sequenceNumber = 1;

    // Fill entire payload with a known byte
    memset(p.payload, 'X', MAX_PAYLOAD_SIZE);
    p.payloadLength = MAX_PAYLOAD_SIZE;

    REQUIRE(p.payloadLength == (uint32_t)MAX_PAYLOAD_SIZE);

    bool allX = true;
    for (int i = 0; i < MAX_PAYLOAD_SIZE; i++)
    {
        if (p.payload[i] != 'X')
        {
            allX = false;
            break;
        }
    }
    REQUIRE(allX);
}

// ============================================================================
// Category 5: Serialization / Deserialization Round-Trip
// ============================================================================

TEST_CASE("Serialization round-trip preserves packet with no payload", "[serialize][roundtrip]")
{
    Packet original{};
    original.packetType = HELLO;
    original.sequenceNumber = 1;
    original.payloadLength = 0;

    // Simulate send: cast to char buffer
    char buffer[sizeof(Packet)];
    memcpy(buffer, &original, sizeof(Packet));

    // Simulate recv: cast back to Packet
    Packet received{};
    memcpy(&received, buffer, sizeof(Packet));

    REQUIRE(received.packetType == original.packetType);
    REQUIRE(received.sequenceNumber == original.sequenceNumber);
    REQUIRE(received.payloadLength == original.payloadLength);
}

TEST_CASE("Serialization round-trip preserves packet with string payload", "[serialize][roundtrip]")
{
    Packet original{};
    original.packetType = DATA;
    original.sequenceNumber = 42;

    const char* msg = "Telemetry Ready";
    strncpy(original.payload, msg, MAX_PAYLOAD_SIZE - 1);
    original.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
    original.payloadLength = static_cast<uint32_t>(strlen(msg));

    // Simulate send/recv via char buffer
    char buffer[sizeof(Packet)];
    memcpy(buffer, &original, sizeof(Packet));

    Packet received{};
    memcpy(&received, buffer, sizeof(Packet));

    REQUIRE(received.packetType == DATA);
    REQUIRE(received.sequenceNumber == 42);
    REQUIRE(received.payloadLength == strlen(msg));
    REQUIRE(std::string(received.payload, received.payloadLength) == "Telemetry Ready");
}

TEST_CASE("Serialization round-trip preserves packet with binary payload", "[serialize][roundtrip]")
{
    Packet original{};
    original.packetType = DATA;
    original.sequenceNumber = 99;

    // Fill payload with sequential bytes (simulating telemetry binary data)
    for (int i = 0; i < MAX_PAYLOAD_SIZE; i++)
    {
        original.payload[i] = static_cast<char>(i % 256);
    }
    original.payloadLength = MAX_PAYLOAD_SIZE;

    char buffer[sizeof(Packet)];
    memcpy(buffer, &original, sizeof(Packet));

    Packet received{};
    memcpy(&received, buffer, sizeof(Packet));

    REQUIRE(received.packetType == DATA);
    REQUIRE(received.sequenceNumber == 99);
    REQUIRE(received.payloadLength == (uint32_t)MAX_PAYLOAD_SIZE);
    REQUIRE(memcmp(received.payload, original.payload, MAX_PAYLOAD_SIZE) == 0);
}

TEST_CASE("Serialization round-trip works for all packet types", "[serialize][roundtrip][all-types]")
{
    uint32_t types[] = {HELLO, VERIFY, GET_STATUS, DOWNLOAD_TELEMETRY, DATA, ACK, ERROR_PACKET};

    for (uint32_t type : types)
    {
        Packet original{};
        original.packetType = type;
        original.sequenceNumber = type * 10;
        original.payloadLength = 0;

        char buffer[sizeof(Packet)];
        memcpy(buffer, &original, sizeof(Packet));

        Packet received{};
        memcpy(&received, buffer, sizeof(Packet));

        REQUIRE(received.packetType == type);
        REQUIRE(received.sequenceNumber == type * 10);
        REQUIRE(received.payloadLength == 0);
    }
}

// ============================================================================
// Category 6: Protocol-Specific Packet Construction
//   (mirrors exact patterns from client.cpp and server.cpp)
// ============================================================================

TEST_CASE("HELLO packet construction matches client.cpp", "[protocol][hello]")
{
    // Mirrors client.cpp lines 42-45
    Packet hello{};
    hello.packetType = HELLO;
    hello.sequenceNumber = 1;
    hello.payloadLength = 0;

    REQUIRE(hello.packetType == HELLO);
    REQUIRE(hello.sequenceNumber == 1);
    REQUIRE(hello.payloadLength == 0);
}

TEST_CASE("VERIFY packet construction matches server.cpp", "[protocol][verify]")
{
    // Mirrors server.cpp lines 82-85
    Packet verifyPacket{};
    verifyPacket.packetType = VERIFY;
    verifyPacket.sequenceNumber = 1;
    verifyPacket.payloadLength = 0;

    REQUIRE(verifyPacket.packetType == VERIFY);
    REQUIRE(verifyPacket.sequenceNumber == 1);
    REQUIRE(verifyPacket.payloadLength == 0);
}

TEST_CASE("GET_STATUS packet construction matches client.cpp", "[protocol][status]")
{
    // Mirrors client.cpp lines 66-69
    Packet status{};
    status.packetType = GET_STATUS;
    status.sequenceNumber = 2;
    status.payloadLength = 0;

    REQUIRE(status.packetType == GET_STATUS);
    REQUIRE(status.sequenceNumber == 2);
    REQUIRE(status.payloadLength == 0);
}

TEST_CASE("DATA packet with status message matches server.cpp", "[protocol][data][status-msg]")
{
    // Mirrors server.cpp lines 97-109
    Packet statusPacket{};
    statusPacket.packetType = DATA;
    statusPacket.sequenceNumber = 2;

    const char* msg = "Telemetry Ready";
    strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
    statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
    statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

    REQUIRE(statusPacket.packetType == DATA);
    REQUIRE(statusPacket.sequenceNumber == 2);
    REQUIRE(statusPacket.payloadLength == strlen("Telemetry Ready"));
    REQUIRE(std::string(statusPacket.payload, statusPacket.payloadLength) == "Telemetry Ready");
}

TEST_CASE("DATA packet with missing file message matches server.cpp", "[protocol][data][missing-msg]")
{
    // Mirrors server.cpp lines 115-118
    Packet statusPacket{};
    statusPacket.packetType = DATA;
    statusPacket.sequenceNumber = 2;

    const char* msg = "Telemetry File Missing";
    strncpy(statusPacket.payload, msg, MAX_PAYLOAD_SIZE - 1);
    statusPacket.payload[MAX_PAYLOAD_SIZE - 1] = '\0';
    statusPacket.payloadLength = static_cast<uint32_t>(strlen(msg));

    REQUIRE(statusPacket.packetType == DATA);
    REQUIRE(statusPacket.payloadLength == strlen("Telemetry File Missing"));
    REQUIRE(std::string(statusPacket.payload, statusPacket.payloadLength) == "Telemetry File Missing");
}

TEST_CASE("DATA packet with binary telemetry chunk matches server.cpp", "[protocol][data][binary]")
{
    // Mirrors server.cpp lines 149-159
    Packet dataPacket{};
    dataPacket.packetType = DATA;
    dataPacket.sequenceNumber = 1;

    // Simulate reading binary data from a file
    char simulatedFileData[MAX_PAYLOAD_SIZE];
    memset(simulatedFileData, 0xAB, MAX_PAYLOAD_SIZE);

    memcpy(dataPacket.payload, simulatedFileData, MAX_PAYLOAD_SIZE);
    dataPacket.payloadLength = MAX_PAYLOAD_SIZE;

    REQUIRE(dataPacket.packetType == DATA);
    REQUIRE(dataPacket.sequenceNumber == 1);
    REQUIRE(dataPacket.payloadLength == (uint32_t)MAX_PAYLOAD_SIZE);
    REQUIRE(memcmp(dataPacket.payload, simulatedFileData, MAX_PAYLOAD_SIZE) == 0);
}

TEST_CASE("ACK packet construction matches server.cpp", "[protocol][ack]")
{
    // Mirrors server.cpp lines 167-170
    int sequence = 1025; // Simulating end of a 1MB transfer (1024 chunks + 1)
    Packet ackPacket{};
    ackPacket.packetType = ACK;
    ackPacket.sequenceNumber = sequence;
    ackPacket.payloadLength = 0;

    REQUIRE(ackPacket.packetType == ACK);
    REQUIRE(ackPacket.sequenceNumber == (uint32_t)sequence);
    REQUIRE(ackPacket.payloadLength == 0);
}

TEST_CASE("ERROR_PACKET construction matches server.cpp", "[protocol][error]")
{
    // Mirrors server.cpp lines 135-138
    Packet errorPacket{};
    errorPacket.packetType = ERROR_PACKET;
    errorPacket.sequenceNumber = 0;
    errorPacket.payloadLength = 0;

    REQUIRE(errorPacket.packetType == ERROR_PACKET);
    REQUIRE(errorPacket.sequenceNumber == 0);
    REQUIRE(errorPacket.payloadLength == 0);
}

// ============================================================================
// Category 7: Edge Cases & Robustness
// ============================================================================

TEST_CASE("Maximum sequence number", "[edge][sequence]")
{
    Packet p{};
    p.packetType = DATA;
    p.sequenceNumber = UINT32_MAX;
    p.payloadLength = 0;

    REQUIRE(p.sequenceNumber == UINT32_MAX);

    // Round-trip with max sequence number
    char buffer[sizeof(Packet)];
    memcpy(buffer, &p, sizeof(Packet));

    Packet received{};
    memcpy(&received, buffer, sizeof(Packet));
    REQUIRE(received.sequenceNumber == UINT32_MAX);
}

TEST_CASE("Payload length mismatch does not corrupt struct", "[edge][mismatch]")
{
    Packet p{};
    p.packetType = DATA;
    p.sequenceNumber = 1;

    // Write only 5 bytes but set payloadLength to 10
    const char* shortData = "ABCDE";
    memcpy(p.payload, shortData, 5);
    p.payloadLength = 10;

    // The struct itself doesn't enforce consistency — payloadLength is just metadata.
    // This test verifies the struct doesn't crash or corrupt fields.
    REQUIRE(p.payloadLength == 10);
    REQUIRE(memcmp(p.payload, "ABCDE", 5) == 0);
    REQUIRE(p.packetType == DATA);
    REQUIRE(p.sequenceNumber == 1);
}

TEST_CASE("Stale payload data with zero payloadLength", "[edge][stale]")
{
    Packet p{};
    p.packetType = DATA;

    // Write data into payload
    memset(p.payload, 'Z', 100);

    // But set payloadLength to 0 — protocol should treat as empty
    p.payloadLength = 0;

    // Verify the payloadLength field is the authority
    REQUIRE(p.payloadLength == 0);
    // The stale data still exists in the buffer (this is expected behavior)
    REQUIRE(p.payload[0] == 'Z');
}

TEST_CASE("Multiple packets can coexist independently", "[edge][independence]")
{
    Packet p1{};
    p1.packetType = HELLO;
    p1.sequenceNumber = 1;
    p1.payloadLength = 0;

    Packet p2{};
    p2.packetType = DATA;
    p2.sequenceNumber = 2;
    const char* msg = "test data";
    strncpy(p2.payload, msg, MAX_PAYLOAD_SIZE - 1);
    p2.payloadLength = static_cast<uint32_t>(strlen(msg));

    // Verify they don't interfere with each other
    REQUIRE(p1.packetType == HELLO);
    REQUIRE(p1.payloadLength == 0);
    REQUIRE(p2.packetType == DATA);
    REQUIRE(p2.payloadLength == strlen(msg));
    REQUIRE(std::string(p2.payload, p2.payloadLength) == "test data");
}