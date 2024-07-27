#include "transmitter.h"

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

    transmitter->servaddr.sin_family = AF_INET;
    transmitter->servaddr.sin_port = htons(transmitter->port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, transmitter->ip, &transmitter->servaddr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return false;
    }

    while (connect(transmitter->sockfd, (struct sockaddr *)&transmitter->servaddr, sizeof(transmitter->servaddr)) < 0)
    {
        printf("\nConnection Failed \n");
        //return false;
        sleep(1);
    }
    atomic_store(&transmitter->is_initialized, true);
}

void send_message(transmitter_t* transmitter, char* msg)
{
    send(transmitter->sockfd , msg , strlen(msg) , 0);
}


