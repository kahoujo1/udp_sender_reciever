#ifndef __UDP_MODULE_H__
#define __UDP_MODULE_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <cstring>
#include "packet.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <map>
#include <fstream>

#define TIMEOUT_S 0
#define TIMEOUT_US 500000
#define WINDOW_SIZE 5
#define SENDER_PACKET_TIMEOUT_MS 1000
#define FAIL -1

enum
{ // recieve packet enums
    PACKET_OK = 0,
    PACKET_TIMEOUT = 1,
    PACKET_CORRUPTED = 2,
    PACKET_WRONG_ID = 3,
    PACKET_REC_ACK = 4,
    PACKET_REC_NACK = 5
};

class UDP_Module
{
public:
    int sock;
    sockaddr_in local_addr;
    sockaddr_in from;
    sockaddr_in addrDest;
    UDP_Module(int local_port, int target_port, std::string target_ip);
    ~UDP_Module();
    bool send_packet(udp_packet *packet);
    uint8_t recieve_packet(udp_packet *packet);
    bool change_timeout(int timeout_s, int timeout_us);
    std::map <int, udp_packet> packet_buffer_map; // maps packet_id to packet
    //   map <packet_id, packet>
};

class Sender : public UDP_Module
{
public:
    Sender(int local_port, int target_port, std::string target_ip);
    ~Sender();
    uint8_t send_packet_with_ack(udp_packet *packet);
    void send_file(const char *filename);
};

class Reciever : public UDP_Module
{
public:
    Reciever(int local_port, int target_port, std::string target_ip);
    ~Reciever();
    void recieve();
    bool recieve_file(const char *filename, std::string hash, uint32_t start_packet_id);
    int recieve_data_packet(udp_packet *packet, uint32_t packet_id);
    void recieve_file_data(FILE *file);
    udp_packet packet_buffer[WINDOW_SIZE];
    void clean_buffer(std::map<int, udp_packet>& packet_buffer_map, uint32_t written_packet_id);
};

#endif // __UDP_MODULE_H__