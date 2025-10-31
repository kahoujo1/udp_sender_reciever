#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include <chrono>
#define PACKET_DATA_LENGTH 1024

enum
{ // packet types
    PACKET_TYPE_DATA = 0,
    PACKET_TYPE_START,
    PACKET_TYPE_END,
    PACKET_TYPE_ACK,
    PACKET_TYPE_NACK,
    PACKET_TYPE_RESEND,
    PACKET_TYPE_RESTART,
};

struct udp_packet
{
    uint8_t packet_type;
    uint32_t packet_id;
    uint8_t data[PACKET_DATA_LENGTH];
    uint32_t data_length = 0;
    uint32_t crc = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> time_sent = std::chrono::high_resolution_clock::now();
};

#endif // __PACKET_H__