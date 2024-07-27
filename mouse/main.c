#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "transmitter.h"
#include "receiver.h"

#define PORT 12345
#define BROADCAST_ADDR "192.168.7.3"  // Adjust this to your network's broadcast address

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enable broadcast option
    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Setup server address to send broadcast message
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(BROADCAST_ADDR);
    servaddr.sin_port = htons(PORT);

    // Message to send
    const char *message = "CONNRQ";

    // Send broadcast message
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Broadcast message sent.\n");

    close(sockfd);

    receiver_t* response_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    client_t* client = (client_t*)malloc(sizeof(client_t));

    printf("poller address = %p\n", poller);
    printf("client address = %p\n", client);
    init_unicast_listener(response_receiver);
    client->msg = "ACCRQ";
    poller->client = client;
    wait_for_message(response_receiver, poller);

    if (poller->flag)
    {
        printf("Received ACCRQ\n");
        transmitter_t* acceptor = (transmitter_t*)malloc(sizeof(transmitter_t));
        strcpy(acceptor->ip, inet_ntoa(response_receiver->cliaddr.sin_addr));
        acceptor->port = ntohs(response_receiver->cliaddr.sin_port);
        acceptor->port = 12345;
        init_unicast_sender(acceptor);

        //sleep(5);
        printf("Sending ACCRQ to %s:%d\n", acceptor->ip, acceptor->port);
        send_message(acceptor, "ACK");
    }
    return 0;
}
