#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "packet.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <iostream>

#define LOCAL_PORT 5555
#define TARGET_PORT 8888
#define TARGET_IP "127.0.0.1"

#define FAIL -1



int main()
{
    // create UDP socket
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket creation failed");
        return FAIL;
    }
    // configure address
    sockaddr_in local;
    sockaddr_in from;
    int fromlen = sizeof(from);
    local.sin_family = AF_INET;
    local.sin_port = htons(LOCAL_PORT);
    local.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(sock, (sockaddr*)&local, sizeof(local)) != 0){
		printf("Binding error!\n");
	    getchar(); //wait for press Enter
		return FAIL;
	}

    // read file
    struct udp_packet packet;
    // copy file data to packet data
    uint8_t data[PACKET_DATA_LENGTH];

	printf("Waiting for datagram ...\n");
	if(recvfrom(sock, &packet, sizeof(data), 0, (sockaddr*)&from, (socklen_t*) &fromlen) == FAIL){
		printf("Socket error!\n");
		getchar();
		return 1;
	}
	else
        std::cout << "Data received: "; 
        for (int i = 0; i < PACKET_DATA_LENGTH; i++)
        {
            if (packet.data[i] == '\0' && i > 0)
            {
                break;
            }
            std::cout << packet.data[i];
        }
        std::cout << std::endl;

    shutdown(sock, 2);
    close(sock);
};


