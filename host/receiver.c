#include "receiver.h"

//SLIST_HEAD(headClient, client) clients = SLIST_HEAD_INITIALIZER(clients);

bool init_listener_for_broadcast(receiver_t* receiver)
{
    if (atomic_load(&receiver->is_initialized))
    {
        printf("Already initialized! \n");
        return false;
    }

    receiver->len = sizeof(receiver->cliaddr);
    // Create a UDP socket
    if ((receiver->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        return false;
    }

    // Enable broadcast option
    int broadcastEnable = 1;
    if (setsockopt(receiver->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("setsockopt failed");
        close(receiver->sockfd);
        return false;
    }

    // Setup server address to receive from any address on PORT
    memset(&receiver->servaddr, 0, sizeof(receiver->servaddr));
    receiver->servaddr.sin_family = AF_INET;
    receiver->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on any interface
    receiver->servaddr.sin_port = htons(PORT);

    // Bind socket to the server address
    if (bind(receiver->sockfd, (const struct sockaddr *)&receiver->servaddr, sizeof(receiver->servaddr)) < 0)
    {
        perror("bind failed");
        close(receiver->sockfd);
        return false;
    }

    printf("Listening for UDP packets on %s:%d...\n", BROADCAST_ADDR, PORT);

    SLIST_INIT(&receiver->clients);

    int failedToCreateThread = pthread_create(&receiver->thread, NULL, listen_for_broadcast, (void*) receiver);
    if (failedToCreateThread)
    {
        printf("Failed to create a thread... \n");
        return false;
    }
    atomic_store(&receiver->is_initialized, true);
    return true;
}
void* listen_for_broadcast(void* receiver)
{
    receiver_t* receiver_thread = (receiver_t *) receiver;
    while (1)
    {
        // Receive packet
        pthread_mutex_lock(&receiver_thread->server_mutex);
        int n = recvfrom(receiver_thread->sockfd, (char *)receiver_thread->buffer, sizeof(receiver_thread->buffer), 0, (struct sockaddr *)&receiver_thread->cliaddr, &receiver_thread->len);
        if (n < 0) {
            perror("recvfrom failed");
            close(receiver_thread->sockfd);
            //exit(EXIT_FAILURE);
        }

        receiver_thread->buffer[n] = '\0';  // Null-terminate the received data
        pthread_mutex_unlock(&receiver_thread->server_mutex);

        client_t* current_client;
        pthread_mutex_lock(&receiver_thread->subscribers_mutex);
        SLIST_FOREACH(current_client, &receiver_thread->clients, entries)
        {
            if (!strcmp(receiver_thread->buffer, current_client->msg))
            {
                (current_client->callBack)();
            }
        }
        pthread_mutex_unlock(&receiver_thread->subscribers_mutex);

        // Print received packet information
        printf("Received packet from %s:%d\n", inet_ntoa(receiver_thread->cliaddr.sin_addr), ntohs(receiver_thread->cliaddr.sin_port));
        printf("Message: %s\n", receiver_thread->buffer);
        printf("--------------------------------------\n");
    }

    close(receiver_thread->sockfd);
}
void subscribe_for_message(receiver_t* receiver, client_t* client)
{
    pthread_mutex_lock(&receiver->subscribers_mutex);
    SLIST_INSERT_HEAD(&receiver->clients, client, entries);
    pthread_mutex_unlock(&receiver->subscribers_mutex);
}

void unsubscribe_for_message(receiver_t* receiver, client_t* client)
{
    pthread_mutex_lock(&receiver->subscribers_mutex);
    client_t* current_client;
    SLIST_FOREACH(current_client, &receiver->clients, entries)
    {
        if (!strcmp(client->msg, current_client->msg))
        {
            SLIST_REMOVE(&receiver->clients, current_client, client, entries);
        }
    }
    pthread_mutex_unlock(&receiver->subscribers_mutex);
}