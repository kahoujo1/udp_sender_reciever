#include "udp_module.h"
//#define LOCAL_PORT 8888
//#define TARGET_PORT 7777
// NetDerper ports
#define TARGET_PORT 14000
#define LOCAL_PORT 15001

#define TARGET_IP "127.0.0.1"
// loopback address "127.0.0.1"
#define FAIL -1

int main()
{
    Sender sender(LOCAL_PORT, TARGET_PORT, TARGET_IP);
    sender.send_file("src/cat.jpg");
};
