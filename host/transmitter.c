#include "transmitter.h"

bool init_broadcast_sender(transmitter_t* transmitter)
{
    if (atomic_load(&transmitter->is_initialized))
    {
        printf("Already initialized! \n");
        return false;
    }
    // Create a UDP socket
    if ((transmitter->sockfd  = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        return false;
    }

    // Enable broadcast option
    int broadcastEnable = 1;
    if (setsockopt(transmitter->sockfd , SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("setsockopt failed");
        close(transmitter->sockfd );
        return false;
    }

    // Setup server address to send broadcast message
    memset(&transmitter->servaddr , 0, sizeof(transmitter->servaddr));
    transmitter->servaddr.sin_family = AF_INET;
    transmitter->servaddr.sin_addr.s_addr = inet_addr(transmitter->ip);
    transmitter->servaddr.sin_port = htons(transmitter->port);

    atomic_store(&transmitter->is_initialized, true);
}

bool init_unicast_sender(transmitter_t* transmitter)
{
    if (atomic_load(&transmitter->is_initialized))
    {
        printf("Already initialized! \n");
        return false;
    }

    if ((transmitter->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return false;
    }

    memset(&transmitter->servaddr, 0, sizeof(transmitter->servaddr));
    transmitter->servaddr.sin_family = AF_INET;
    transmitter->servaddr.sin_port = htons(transmitter->port);
    //transmitter->servaddr.sin_addr.s_addr = inet_addr("192.168.7.2");
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, transmitter->ip, &transmitter->servaddr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return false;
    }

    int optval = 1;
    if (setsockopt(transmitter->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
    }

    while (connect(transmitter->sockfd, (struct sockaddr *)&transmitter->servaddr, sizeof(transmitter->servaddr)) < 0)
    {
        printf("\nConnection Failed \n");
        sleep(1);
        //return false;
    }
    atomic_store(&transmitter->is_initialized, true);
}

void send_message(transmitter_t* transmitter, char* msg)
{
    send(transmitter->sockfd , msg , strlen(msg) , 0);
}

void send_broadcast_message(transmitter_t* transmitter, char* msg)
{
    sendto(transmitter->sockfd, msg, strlen(msg), 0, (struct sockaddr *)&transmitter->servaddr, sizeof(transmitter->servaddr));
}
