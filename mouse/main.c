#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "transmitter.h"
#include "receiver.h"

//#define PORT 12345
//#define BROADCAST_ADDR "192.168.7.3"  // Adjust this to your network's broadcast address
void* handshake (void* data)
{
    transmitter_t* initial_transmitter = (transmitter_t*)malloc(sizeof(transmitter_t));
    strcpy(initial_transmitter->ip, "192.168.1.255");
    initial_transmitter->port = 12345;
    init_broadcast_sender(initial_transmitter);

    while (1)
    {
        send_broadcast_message(initial_transmitter, "CONNRQ");
        sleep(1);
    }
}
int main()
{
    pthread_t handshaker;
    receiver_t* response_receiver = (receiver_t*)malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    //client_t* client = (client_t*)malloc(sizeof(client_t));

    while (1)
    {
        int failedToCreateThread = pthread_create(&handshaker, NULL, handshake, (void*) NULL);
        if (failedToCreateThread)
        {
            printf("Failed to create a thread... \n");
            return false;
        }

        init_unicast_listener(response_receiver);
        poller->client.msg = "ACCRQ";
//        poller->client = client;
        wait_for_message(response_receiver, poller);

        if (poller->flag)
        {
            printf("Received ACCRQ\n");
            pthread_cancel(handshaker);
            transmitter_t* acceptor = (transmitter_t*)malloc(sizeof(transmitter_t));
            strcpy(acceptor->ip, inet_ntoa(response_receiver->cliaddr.sin_addr));
            acceptor->port = ntohs(response_receiver->cliaddr.sin_port);
            acceptor->port = 12345;
            init_unicast_sender(acceptor);

            printf("Sending ACCRQ to %s:%d\n", acceptor->ip, acceptor->port);
            send_message(acceptor, "ACK");
            while (1) //infinite loop would later be replaced by subscription to stop message.
            {
                atomic_store(&acceptor->is_initialized, false);
                close(acceptor->sockfd);
                init_unicast_sender(acceptor);
                printf("Enter new mouse position\n");
                char cmd[20] = "MVMSE ";
                char pos[10];
                scanf("%s", pos);
                strcat(cmd, pos);
                send_message(acceptor, cmd);
            }
        }
    }
    return 0;
}
