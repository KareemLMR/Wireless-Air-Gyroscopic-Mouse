#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct transmitter
{
    int sockfd;
    struct sockaddr_in servaddr;
    atomic_bool is_initialized;
    uint16_t port;
    char ip[20];
}transmitter_t;

bool init_broadcast_sender(transmitter_t* transmitter);
bool init_unicast_sender(transmitter_t* transmitter);
void send_message(transmitter_t* transmitter, char* msg);
void send_broadcast_message(transmitter_t* transmitter, char* msg);
