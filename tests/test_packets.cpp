#include <iostream>
#include "../common/packet.h"

int main()
{
    Packet p{};
    p.packetType = HELLO;
    p.sequenceNumber = 1;
    p.payloadLength = 0;

    if (p.packetType == HELLO &&
        p.sequenceNumber == 1 &&
        p.payloadLength == 0)
    {
        std::cout << "Packet test passed\n";
    }
    else
    {
        std::cout << "Packet test failed\n";
    }

    return 0;
}