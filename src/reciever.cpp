#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "packet.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "udp_module.h"

// #define LOCAL_PORT 7777
// #define TARGET_PORT 8888
// NetDerper ports
#define TARGET_PORT 14001
#define LOCAL_PORT 15000

#define TARGET_IP "127.0.0.1"

#define FAIL -1

int main()
{
    Reciever reciever(LOCAL_PORT, TARGET_PORT, TARGET_IP);
    reciever.recieve();
};
