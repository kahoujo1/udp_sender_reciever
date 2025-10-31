#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include "utils.h"
#include <cassert>
#include <iostream>

#define CRC_CHECK 784084571
#define FILE_HASH_CHECK "89cfd583ddb934baee2ee3cbd077b1be4a4da9b30b5df1531e24760335892551"

void test_file_size()
{
    const char *filename = "testfile.txt";
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("File open failed");
        return;
    }
    fputs("demence z kredence", file);
    fclose(file);

    long size = file_size(filename);
    std::cout << "Expected size: 17, Actual size: " << size << std::endl;
    assert(size == 18);
    std::cout << "test_file_size passed!" << std::endl;
}

void test_file_hash()
{
    const char *filename = "testfile.txt";
    FILE *file = fopen(filename, "w");
    fputs("demence z kredence", file);
    fclose(file);

    std::string hash = file_hash(filename);
    std::cout << "Expected hash: " << hash << " Got hash: " << FILE_HASH_CHECK << std::endl;
    assert(hash == FILE_HASH_CHECK); // Replace with actual hash value
    std::cout << "test_file_hash passed!" << std::endl;
}

void test_gen_crc32()
{
    const uint8_t data[] = "demence z kredence";
    uint32_t crc = gen_crc32(data, 18);
    std::cout << "Expected crc: " << CRC_CHECK << " Got crc: " << crc << std::endl;
    assert(crc == CRC_CHECK); // Replace with actual CRC value
    std::cout << "test_gen_crc32 passed!" << std::endl;
}

void test_packet_crc32()
{
    udp_packet packet;
    packet.data_length = 17;
    memcpy(packet.data, "demence z kredence", 17);
    package_packet_with_crc32(&packet);
    unsigned int crc_gotten = get_packet_crc32(&packet);
    std::cout << " Packaged packet crc: " << packet.crc << " Calculated packet crc: " << crc_gotten << std::endl;
    assert(packet.crc == crc_gotten);
    std::cout << "test_packet_crc32 passed!" << std::endl;
}

int main()
{
    test_file_size();
    test_file_hash();
    test_gen_crc32();
    test_packet_crc32();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
#endif // __TEST_UTILS_H__