#include "udp_module.h"
#include "utils.h"

/*
----------------------------UDP_MODULE------------------------------------
*/

UDP_Module::UDP_Module(int local_port, int target_port, std::string target_ip)
{
    // create recieve socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket creation failed");
        exit(FAIL);
    }
    // set timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_S;
    tv.tv_usec = TIMEOUT_US;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error");
        exit(FAIL);
    }
    // configure address
    int fromlen = sizeof(this->from);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    // configure address
    this->addrDest.sin_family = AF_INET;
    this->addrDest.sin_port = htons(target_port);
    this->addrDest.sin_addr.s_addr = inet_addr(target_ip.c_str());

    // bind socket
    if (bind(sock, (sockaddr *)&local_addr, sizeof(local_addr)) != 0)
    {
        printf("Binding error!\n");
        exit(FAIL);
    }
}

UDP_Module::~UDP_Module()
{
    close(sock);
}

bool UDP_Module::send_packet(udp_packet *packet)
{
    //printf("Calculating CRC\n");
    package_packet_with_crc32(packet);
    //printf("Sending packet\n");
    sendto(sock, packet, sizeof(udp_packet), 0, (sockaddr *)&addrDest, sizeof(addrDest));
    //printf("Packet sent.\n");
    return true;
}

uint8_t UDP_Module::recieve_packet(udp_packet *packet)
{
    int fromlen = sizeof(this->from);
    int bytesRead = recvfrom(sock, packet, sizeof(udp_packet), 0, (sockaddr *)&this->from, (socklen_t *)&fromlen);
    if (bytesRead < 0)
    {
        printf("Recieving packet: Packet timeout\n");
        return PACKET_TIMEOUT;
    }
    // TODO: calculate CRC and check if it is correct
    if (packet->crc != get_packet_crc32(packet))
    {
        printf("Recieving packet: Packet corrupted\n");
        return PACKET_CORRUPTED;
    }
    //printf("Packet recieved.\n");
    return PACKET_OK;
}

bool UDP_Module::change_timeout(int timeout_s, int timeout_us)
{
    struct timeval tv;
    tv.tv_sec = timeout_s;
    tv.tv_usec = timeout_us;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error changing timeout");
        return false;
    }
    return true;
}
/*
----------------------------SENDER------------------------------------
*/

Sender::Sender(int local_port, int target_port, std::string target_ip) : UDP_Module(local_port, target_port, target_ip)
{
}

Sender::~Sender()
{
}

uint8_t Sender::send_packet_with_ack(udp_packet *packet)
{
    send_packet(packet);
    udp_packet ack_packet;
    uint8_t packet_recieved = recieve_packet(&ack_packet);
    // TODO: replace with switch
    if (packet_recieved == PACKET_TIMEOUT)
    { // Responce did not come
        std::cout << "SENDER: Packet timeout\n";
        return PACKET_TIMEOUT;
    }
    else if (packet_recieved == PACKET_CORRUPTED)
    { // Responce came corrupted
        std::cout << "SENDER: Packet corrupted\n";
        return PACKET_CORRUPTED;
    }
    else if (ack_packet.packet_id != packet->packet_id)
    { // Responce came from another packet
        std::cout << "SENDER: Packet ID mismatch\n";
        std::cout << "Expected ID: " << packet->packet_id << " Recieved ID: " << ack_packet.packet_id << std::endl;
        return PACKET_WRONG_ID;
    }
    else if (ack_packet.packet_type == PACKET_TYPE_ACK)
    { // Responce is ACK
        std::cout << "SENDER: Packet recieved ACK\n";
        return PACKET_REC_ACK;
    }
    else
    { // Responce is NACK (ack_packet.packet_type == PACKET_TYPE_NACK)
        std::cout << "SENDER: Packet recieved NACK\n";
        return PACKET_REC_NACK;
    }
}

void Sender::send_file(const char *filename)
{
    // calculate hash
    std::string hash = file_hash(filename);
    printf("hash: %s\n", hash.c_str());
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("file open failed");
        exit(FAIL);
    }
    // TODO: set timeout for recieving port
    // variables:
    uint8_t ret;
    uint32_t packet_id_ = 0;
    udp_packet start_packet;
    start_packet.packet_id = packet_id_;
    start_packet.packet_type = PACKET_TYPE_START;
    std::memcpy(start_packet.data, hash.c_str(), hash.length());
    start_packet.data_length = hash.length();
    std::cout << "Sending start packet\n";
    do
    {
        //print_packet(&start_packet);
        ret = send_packet_with_ack(&start_packet);
    } while (ret == PACKET_TIMEOUT || ret == PACKET_CORRUPTED || ret == PACKET_WRONG_ID || ret == PACKET_REC_NACK);
    printf("Start packet sent and recieved ACK, starting sending data packets\n");
    udp_packet data_packet;
    size_t bytesRead;
    udp_packet ret_packet;
    uint32_t resend_packet_id = 0; // packet id of the packet that needs to be resent, 0 means no packet needs to be resent
    while (true) {
        // TODO: check all the packets in the buffer and resend the ones whose timer has expired
        // if there is a packet that needs to be resent, resend it
        if (resend_packet_id != 0 && packet_buffer_map.find(resend_packet_id) != packet_buffer_map.end()) {
            udp_packet resend_packet = packet_buffer_map[resend_packet_id];
            send_packet(&resend_packet);
            packet_buffer_map[resend_packet_id].time_sent = std::chrono::high_resolution_clock::now();
            //packet_buffer_map[resend_packet_id].packet_id = resend_packet_id;
            //print_packet(&resend_packet);
            resend_packet_id = 0;
        } else {
            while (packet_buffer_map.size() < WINDOW_SIZE && 
            (bytesRead = fread(data_packet.data, 1, PACKET_DATA_LENGTH, file)) > 0) { 
            data_packet.packet_id = ++packet_id_;
            data_packet.packet_type = PACKET_TYPE_DATA; // You can define different packet types if needed
            data_packet.data_length = bytesRead;
            data_packet.time_sent = std::chrono::high_resolution_clock::now();
            // send the packet
            packet_buffer_map[data_packet.packet_id] = data_packet;
            std::cout << "Read the file, sending data packet with ID " << data_packet.packet_id << std::endl;
            send_packet(&data_packet);
            // save it to the buffer
            } // while end
        } 
        // if I can save the packet, read file, send it and save it
        
        // recieve answer
        ret = recieve_packet(&ret_packet);
        if (ret == PACKET_OK) { // packet recieved, check if it is ACK or NACK
            if (ret_packet.packet_id == 0) {
                std::cout << "Sender: Recieved ACK for start packet, ignore." << std::endl;
            } else if (ret_packet.packet_type == PACKET_TYPE_ACK) {
                packet_buffer_map.erase(ret_packet.packet_id); // remove the packet from the map
                std::cout << "Sender: ACK recieved for packet " << ret_packet.packet_id << ", deleting from buffer." << std::endl;
            } else if (ret_packet.packet_type == PACKET_TYPE_NACK) { 
                std::cout << "Sender: NACK recieved for packet " << ret_packet.packet_id << ", resending." << std::endl;
                resend_packet_id = ret_packet.packet_id;
            } else {
                std::cout << "Sender: Unknown packet type recieved." << std::endl;
            }
        } else {// either CRC error or timeout
            std::cout << "Sender: Timeout or CRC error during recieving answer." << std::endl;
        }
        // check if packet timer has expired
        for (auto it = packet_buffer_map.begin(); it != packet_buffer_map.end(); it++) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - it->second.time_sent).count()
                > SENDER_PACKET_TIMEOUT_MS) {
                std::cout << "Sender: Packet "<< it->second.packet_id << " did not recieve answer in time, resending." << std::endl;
                auto value_copy = it->first;
                resend_packet_id = value_copy;
                break;
            }
        }
        // check if buffer is free and i am at the end of the file
        if (packet_buffer_map.empty() && bytesRead == 0) {
            break;
        }
    } // while end
    std::cout << "Sender: Recieved ACK for all data packages, sending end packet." << std::endl;
    // Send end packet
    udp_packet end_packet;
    end_packet.packet_type = PACKET_TYPE_END;
    end_packet.packet_id = ++packet_id_;
    printf("Sending end packet\n");
    for (int i = 0; i < 10; i++) {
        ret = send_packet_with_ack(&end_packet);
        if (ret == PACKET_REC_ACK) {
            break;
        }
    }
    if (ret != PACKET_REC_ACK) {
        std::cout << "Sender: Did not recieve ACK for end packet." << std::endl;
    } else {
        std::cout << "Sender: Recieved ACK for end packet, ending." << std::endl;
    }
    fclose(file);
    return;
}


/*
----------------------------RECIEVER------------------------------------
*/

/*
What can go wrong:
    Packet from sender is corupted -> reciever sends NACK
    Packet from sender is not recieved -> reciever sends NACK
    Sender does not recieve ACK or NACK (timeout) -> sender resends the packet
    Sender recieves NACK -> resends the packet
    Sender recieves ACK -> sends next packet
    Reciever finishes the file, but the hash is not correct -> reciever sends NACK and the sender starts sending the whole file again
*/
// TODO: wait for start packet for a number of seconds, if start packet not recieved return false
// TODO: Send acknowledge for the start packet
// TODO: In a while: recieve packet, if recieve_packet returns true, write data to file, if recieve_packet returns false, wait for the packet again

Reciever::Reciever(int local_port, int target_port, std::string target_ip) : UDP_Module(local_port, target_port, target_ip)
{
}

Reciever::~Reciever()
{
}

void Reciever::recieve()
{
    bool cont = true;

    while (cont)
    {
        // initialise variables
        udp_packet packet;
        udp_packet response_packet;
        uint32_t packet_id = 0;
        std::string hash_recieved;
        

        if (recieve_packet(&packet) == PACKET_OK) // take valid packet
        {
            switch (packet.packet_type)
            {
            case PACKET_TYPE_ACK:
                printf("Recieved ACK packet, throwing away packet\n");
                continue;
            case PACKET_TYPE_NACK:
                printf("Recieved NACK packet, throwing away packet\n");
                send_packet(&packet);
                continue;
            case PACKET_TYPE_START:
                hash_recieved = std::string((char *)packet.data, packet.data_length);
                printf("Recieved start packet, hash: %s\n", hash_recieved.c_str());
                response_packet.packet_type = PACKET_TYPE_ACK;
                response_packet.packet_id = packet.packet_id;
                send_packet(&response_packet);
                if (recieve_file("recieved_file.jpg", hash_recieved, packet.packet_id))
                {
                    printf("File recieved succesfully, exiting\n");
                    cont = false;
                }
                else
                {
                    printf("File NOT recieved succesfully, exiting\n");
                    response_packet.packet_type = PACKET_TYPE_ACK;
                    send_packet(&response_packet);
                    cont = false;
                };
                continue;
            case PACKET_TYPE_DATA:
                printf("Recieved DATA packet, without start, sending RESTART\n");
            case PACKET_TYPE_END:
                // TODO: This should rly send a packet restart or something tbh
                printf("Recieved END packet, without start, sending RESTART\n");
                ;
                response_packet.packet_type = PACKET_TYPE_RESTART;
                send_packet(&response_packet);
                continue;
            default:
                printf("Recieved a UNKNOWN packet type, throwing away\n");
                continue;
            }
        }
        else
        {
            printf("Recieved packet is corrupted or timed out\n");
        }
    }
}

bool Reciever::recieve_file(const char *filename, std::string hash_recieved, uint32_t start_packet_id)
{
    // open file for writing
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("file open failed");
        exit(FAIL);
    }

    bool ret = false;
    udp_packet packet;
    std::string hash_calculated;
    uint32_t packet_id = start_packet_id + 1;

    do // Start receiving data packets, in a loop
{
    if (recieve_data_packet(&packet, packet_id) == PACKET_OK) // take valid packet
    {
        // if it is a data packet
        if (packet.packet_type == PACKET_TYPE_DATA) 
        {
            // if buffer is full, try to clean it
            if (packet_buffer_map.size() >= WINDOW_SIZE) 
            {
                clean_buffer(packet_buffer_map, packet_id);
            }
            
            // if the packet is not in the buffer, save it
            if (packet_buffer_map.find(packet.packet_id) == packet_buffer_map.end() && packet_buffer_map.size() < WINDOW_SIZE)
            {
                packet_buffer_map[packet.packet_id] = packet;
            }
            // else the packet is in the buffer already send ACK
            /*
            if (packet_buffer_map.find(packet.packet_id) != packet_buffer_map.end())
            {
                std::cout << "Data packet " << packet.packet_id << " already in buffer, sending ACK" << std::endl;
                udp_packet ack_packet;
                ack_packet.packet_id = packet.packet_id;
                ack_packet.packet_type = PACKET_TYPE_ACK;
                send_packet(&ack_packet);
            }
            */

            // Write packets in order from the buffer
            while (packet_buffer_map.find(packet_id) != packet_buffer_map.end())
            {
                udp_packet &pkt = packet_buffer_map[packet_id];
                fwrite(pkt.data, sizeof(uint8_t), pkt.data_length, file);
                std::cout << "Data packet " << pkt.packet_id << " popped from buffer and written to file" << std::endl;
                packet_buffer_map.erase(packet_id);
                packet_id++;
            }
            continue;
        }

        // Check if the packet is the last one
        else if (packet.packet_type == PACKET_TYPE_END) // if it is an end packet
        {
            // Check hash at the end of the file
            fclose(file);
            printf("Received end packet, calculating hash.\n");
            hash_calculated = file_hash(filename);
            if (hash_calculated == hash_recieved)
            {
                udp_packet ack_packet;
                ack_packet.packet_type = PACKET_TYPE_ACK;
                ack_packet.packet_id = packet_id;
                send_packet(&ack_packet);
                printf("Hash matches, ACK sent. Ending...\n");
                ret = true;
                break;
            }
            else
            {
                udp_packet nack_packet;
                nack_packet.packet_type = PACKET_TYPE_NACK;
                send_packet(&nack_packet);
                ret = false;
                printf("Hash does not match, NACK sent. Ending...\n");
                break;
            }
        }
    }
} while (true);
    return ret;
}

int Reciever::recieve_data_packet(udp_packet *packet, uint32_t packet_id)
{
    bool ret = recieve_packet(packet);
    udp_packet ret_packet;
    ret_packet.packet_id = packet->packet_id;
    ret_packet.data_length = 0;
    
    if (ret == PACKET_OK && packet->packet_id < packet_id) { // packet recieved, but was already written to file:
        ret_packet.packet_type = PACKET_TYPE_ACK;
        std::cout << "Packet with lower id recieved, sending ACK for packet id: " << ret_packet.packet_id << std::endl;
        send_packet(&ret_packet);
    }
    else if (ret == PACKET_OK && packet->packet_id < packet_id + WINDOW_SIZE)
    {
        ret_packet.packet_type = PACKET_TYPE_ACK;
        std::cout << "Packet recieved sending ACK for packet id:" << ret_packet.packet_id << std::endl;
        send_packet(&ret_packet);
    }
    else 
    {
        ret_packet.packet_type = PACKET_TYPE_NACK;
        std::cout << "Packet not recieved, sending NACK for packet id:" << ret_packet.packet_id << std::endl;
        send_packet(&ret_packet);
    }
    return ret;
}

void Reciever::clean_buffer(std::map<int, udp_packet>& packet_buffer_map, uint32_t written_packet_id) {
    if (packet_buffer_map.empty()) {
        std::cerr << "Error: Packet buffer map is empty." << std::endl;
        return;
    }
    // Remove packet with highest ID
    auto highest_id_it = packet_buffer_map.begin();
    for (auto it = packet_buffer_map.begin(); it != packet_buffer_map.end(); ++it) {
        if (it->first > highest_id_it->first) {
            highest_id_it = it;
        }
    }

    std::cout << "Removing packet with highest ID: " << highest_id_it->first << std::endl;
    packet_buffer_map.erase(highest_id_it);
}