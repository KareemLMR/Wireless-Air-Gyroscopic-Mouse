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
    const char delimiters[] = " ";
    char *token;
    char *token_args;
    char** args = (char**)malloc(sizeof(char*));

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
        printf("string before tokenization = %s\n", receiver_thread->buffer);
        pthread_mutex_unlock(&receiver_thread->server_mutex);

        client_t* current_client;
        token = strtok(receiver_thread->buffer, delimiters);
        //token[strlen(token)] = '\0';
        printf("Token: %s\n", token);
        pthread_mutex_lock(&receiver_thread->subscribers_mutex);
        SLIST_FOREACH(current_client, &receiver_thread->clients, entries)
        {
            if (!strcmp(token, current_client->msg))
            {
                //token[strlen(token)] = ' ';
                token = strtok(NULL, delimiters);
                token_args = strtok(token, ",");  // Passing NULL maintains context between calls
                int i = 0;
                while (token_args != NULL)
                {
                    printf("Token: %s\n", token_args);
                    args[i] = (char*)malloc(strlen(token_args) * sizeof(char));
                    strcpy(args[i], token_args);
                    i++;
                    token_args = strtok(NULL, ",");
                }
                (current_client->callBack)(receiver_thread->cliaddr, current_client, args);
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
void* listen_for_unicast(void* receiver)
{
    receiver_t* receiver_thread = (receiver_t *) receiver;
    const char delimiters[] = " ";
    char *token;
    char** args = (char**)malloc(sizeof(char*));
    char *token_args;

    while (1)
    {
        // Receive packet
        pthread_mutex_lock(&receiver_thread->server_mutex);
        if ((receiver_thread->unicast_socket = accept(receiver_thread->sockfd, (struct sockaddr *)&receiver_thread->cliaddr,
                       (socklen_t*)&receiver_thread->len))<0)
        {
            perror("accept");
        }
        //int n = read(receiver_thread->unicast_socket , receiver_thread->buffer, 1024);
        //receiver_thread->unicast_socket = accept(receiver_thread->sockfd, (struct sockaddr *)NULL, NULL);
        int n = recv(receiver_thread->unicast_socket, receiver_thread->buffer, 1024, 0);
        printf("%s\n",receiver_thread->buffer);
        if (n < 0)
        {
            perror("recvfrom failed");
            close(receiver_thread->unicast_socket);
            //exit(EXIT_FAILURE);
        }

        receiver_thread->buffer[n] = '\0';  // Null-terminate the received data
        printf("string before tokenization = %s\n", receiver_thread->buffer);
        pthread_mutex_unlock(&receiver_thread->server_mutex);

        client_t* current_client;
        token = strtok(receiver_thread->buffer, delimiters);
        //token[strlen(token)] = '\0';
        printf("Token: %s\n", token);
        pthread_mutex_lock(&receiver_thread->subscribers_mutex);
        SLIST_FOREACH(current_client, &receiver_thread->clients, entries)
        {
            if (!strcmp(token, current_client->msg))
            {
                //token[strlen(token)] = ' ';
                token = strtok(NULL, delimiters);
                token_args = strtok(token, ",");  // Passing NULL maintains context between calls
                int i = 0;
                while (token_args != NULL)
                {
                    printf("Token: %s\n", token_args);
                    args[i] = (char*)malloc(strlen(token_args) * sizeof(char));
                    strcpy(args[i], token_args);
                    i++;
                    token_args = strtok(NULL, ",");
                }
                (current_client->callBack)(receiver_thread->cliaddr, current_client, args);
            }
        }
        pthread_mutex_unlock(&receiver_thread->subscribers_mutex);

        // Print received packet information
        printf("Received packet from %s:%d\n", inet_ntoa(receiver_thread->cliaddr.sin_addr), ntohs(receiver_thread->cliaddr.sin_port));
        printf("Message: %s\n", receiver_thread->buffer);
        printf("--------------------------------------\n");
    }

    close(receiver_thread->unicast_socket);
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

bool init_unicast_listener(receiver_t* receiver)
{
    if (atomic_load(&receiver->is_initialized))
    {
        printf("Already initialized! \n");
        return false;
    }
    receiver->len = sizeof(receiver->cliaddr);
     // Creating socket file descriptor
    if ((receiver->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    int opt = 1;
    if (setsockopt(receiver->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    receiver->servaddr.sin_family = AF_INET;
    receiver->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiver->servaddr.sin_port = htons( 12345 );

    // Forcefully attaching socket to the port 8080
    if (bind(receiver->sockfd, (struct sockaddr *)&receiver->servaddr,
                                 sizeof(receiver->servaddr))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(receiver->sockfd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    SLIST_INIT(&receiver->clients);

    int failedToCreateThread = pthread_create(&receiver->thread, NULL, listen_for_unicast, (void*) receiver);
    if (failedToCreateThread)
    {
        printf("Failed to create a thread... \n");
        return false;
    }

    atomic_store(&receiver->is_initialized, true);
    return true;

}

void callBack(struct sockaddr_in cliaddr, client_t* received_client, char** args)
{
    // printf("Matched!\n");
    // printf("Args = %s\n", args[0]);
    // printf("Args = %s\n", args[1]);
    // printf("Args = %s\n", args[2]);
    //poller_t* poller = container_of(received_client, poller_t, client);
    poller_t* poller = (char*)received_client - (char*)0x60;
    // printf("CallBack Poller = %p\n", (char*)poller);
    // printf("CallBack Client = %p\n", (char*)received_client);
    // printf("OFFSET_OF = %zu\n", OFFSET_OF(poller_t, mutex));
    pthread_mutex_lock(&poller->mutex);
    printf("Main thread setting flag to true and waking up waiting thread...\n");
    poller->flag = true;
    pthread_cond_signal(&poller->cond);
    pthread_mutex_unlock(&poller->mutex);
}

void wait_for_message(receiver_t* receiver, poller_t* poller)
{
    // printf("Poller = %p\n", poller);
    // printf("ENtered!\n");
    poller->flag = false;
    pthread_mutex_init(&poller->mutex, NULL);
    pthread_cond_init(&poller->cond, NULL);
    //printf("Setting CallBack!\n");

    poller->client->callBack = callBack;
    //printf("CallBack set successfully!\n");
    subscribe_for_message(receiver, poller->client);
    //printf("Subscribed successfully!\n");
    pthread_mutex_lock(&poller->mutex);

    while (!poller->flag)
    {
        pthread_cond_wait(&poller->cond, &poller->mutex);
    }
    printf("Thread woke up and flag is true!\n");
    
    pthread_mutex_unlock(&poller->mutex);

    unsubscribe_for_message(receiver, poller->client);
}

