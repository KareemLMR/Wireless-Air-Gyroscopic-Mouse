#include "receiver.h"
#include "transmitter.h"

pthread_t ack_handler, nack_handler;

void* handle_ack(void*)
{
    receiver_t* response_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    client_t* client = (client_t*)malloc(sizeof(client_t));
    init_unicast_listener(response_receiver);
    client->msg = "ACK";
    poller->client = client;
    wait_for_message(response_receiver, poller);

    if (poller->flag)
    {
        printf("Received ACK\n");
    }

    pthread_cancel(nack_handler);
}
void* handle_nack(void*)
{
    receiver_t* response_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    client_t* client = (client_t*)malloc(sizeof(client_t));
    init_unicast_listener(response_receiver);
    client->msg = "NACK";
    poller->client = client;
    wait_for_message(response_receiver, poller);

    if (poller->flag)
    {
        printf("Received NACK\n");
    }
    pthread_cancel(ack_handler);
}
int main()
{
    receiver_t* initial_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    client_t* client = (client_t*)malloc(sizeof(client_t));
    init_listener_for_broadcast(initial_receiver);
    client->msg = "CONNRQ";
    poller->client = client;

    wait_for_message(initial_receiver, poller);
    //sleep(5);
    if (1) //poller->flag)
    {
        //close(initial_receiver->sockfd);
        //close(initial_receiver->unicast_socket);
        printf("Received a Connection Request\n");

        transmitter_t* acceptor = (transmitter_t*)malloc(sizeof(transmitter_t));
        strcpy(acceptor->ip, inet_ntoa(initial_receiver->cliaddr.sin_addr));
        acceptor->port = ntohs(initial_receiver->cliaddr.sin_port);
        acceptor->port = 12345;
        init_unicast_sender(acceptor);

        //sleep(5);
        printf("Sending ACCRQ to %s:%d\n", acceptor->ip, acceptor->port);
        send_message(acceptor, "ACCRQ");
        
        printf("Forking ack_handler thread\n");
        int failedToCreateThread = pthread_create(&ack_handler, NULL, handle_ack, (void*) NULL);
        if (failedToCreateThread)
        {
            printf("Failed to create a thread... \n");
            exit(EXIT_FAILURE);
        }
        printf("Forking nack_handler thread\n");
        failedToCreateThread = pthread_create(&nack_handler, NULL, handle_nack, (void*) NULL);
        if (failedToCreateThread)
        {
            printf("Failed to create a thread... \n");
            exit(EXIT_FAILURE);
        }

        void *status;
        printf("Waiting for ack_handler thread\n");
        if (pthread_join(ack_handler,  &status) != 0)
        {
            perror("Failed to join cancelable thread");
            exit(EXIT_FAILURE);
        }

        if (status == PTHREAD_CANCELED)
        {
            printf("The cancelable thread was canceled\n");
        } 
        else
        {
            printf("The cancelable thread finished normally with status %ld\n", (long)status);
        }
        printf("Waiting for nack_handler thread\n");
        if (pthread_join(nack_handler,  &status) != 0)
        {
            perror("Failed to join cancelable thread");
            exit(EXIT_FAILURE);
        }
        if (status == PTHREAD_CANCELED)
        {
            printf("The cancelable thread was canceled\n");
        } 
        else
        {
            printf("The cancelable thread finished normally with status %ld\n", (long)status);
        }

    }
    sleep(5);
    //while(1);
    return 0;
}