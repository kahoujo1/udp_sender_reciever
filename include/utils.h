#ifndef __UTILS_H__
#define __UTILS_H__

#include "packet.h"
#include <openssl/evp.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#define BUFFER_SIZE 32768
#define CRC_POLYNOMIAL 0xEDB88320
// Create hash

long file_size(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("file open failed");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);

    return fileSize;
}

std::string file_hash(const char *filename)
{
    // Initialize OpenSSL
    OpenSSL_add_all_digests();

    // Open file
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("File open failed during hash computation");
        return "";
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL)
    {
        fclose(file);
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return "";
    }

    // Read file in chunks and update hash
    unsigned char *buffer = new unsigned char[BUFFER_SIZE];
    int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)))
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1)
        {
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            delete[] buffer;
            return "";
        }
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        delete[] buffer;
        return "";
    }

    // Close file and free buffer
    EVP_MD_CTX_free(mdctx);
    fclose(file);
    delete[] buffer;

    // Convert hash to string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    // Cleanup OpenSSL
    EVP_cleanup();

    return ss.str();
}

uint32_t gen_crc32(const uint8_t *data, size_t length)
{
    // initalise crc
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= data[i]; // XOR byte into least significant byte of crc

        for (int j = 0; j < 8; ++j)
        { // scroll over each bit
            if (crc & 1)
            {                                      // if this bit is set
                crc = (crc >> 1) ^ CRC_POLYNOMIAL; // multiply by polynomial
            }
            else
            {
                crc >>= 1; // else move to next bit
            }
        }
    }

    return ~crc; // flip bits
}

void package_packet_with_crc32(udp_packet *packet)
{
    // Create buffer for packet data
    size_t buffer_size = sizeof(packet->packet_type) + sizeof(packet->packet_id) + packet->data_length;
    uint8_t *buffer = new uint8_t[buffer_size];

    // Copy packet data to buffer
    memcpy(buffer, &packet->packet_type, sizeof(packet->packet_type));
    memcpy(buffer + sizeof(packet->packet_type), &packet->packet_id, sizeof(packet->packet_id));
    memcpy(buffer + sizeof(packet->packet_type) + sizeof(packet->packet_id), packet->data, packet->data_length);

    // Generate CRC and assign to packet
    packet->crc = gen_crc32(buffer, buffer_size);

    delete[] buffer;
}

unsigned int get_packet_crc32(udp_packet *packet)
{
    unsigned int crc;
    // Create buffer for packet data
    size_t buffer_size = sizeof(packet->packet_type) + sizeof(packet->packet_id) + packet->data_length;
    uint8_t *buffer = new uint8_t[buffer_size];

    // Copy packet data to buffer
    memcpy(buffer, &packet->packet_type, sizeof(packet->packet_type));
    memcpy(buffer + sizeof(packet->packet_type), &packet->packet_id, sizeof(packet->packet_id));
    memcpy(buffer + sizeof(packet->packet_type) + sizeof(packet->packet_id), packet->data, packet->data_length);

    // Generate CRC and assign to packet
    crc = gen_crc32(buffer, buffer_size);
    delete[] buffer;
    return crc;
}

void print_packet(udp_packet *packet)
{
    std::cout << "------PRINTING PACKET------" << std::endl;
    std::cout << "Packet type: " << (int)packet->packet_type << std::endl;
    std::cout << "Packet ID: " << packet->packet_id << std::endl;
    std::cout << "Data length: " << packet->data_length << std::endl;
    //std::cout << "CRC: " << packet->crc << std::endl;
    std::cout << "----------------------------" << std::endl;
}

#endif // __UTILS_H__