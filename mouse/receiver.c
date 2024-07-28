#include "receiver.h"

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
        //close(receiver->sockfd);
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
        //close(receiver->sockfd);
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
            //close(receiver_thread->sockfd);
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

    //close(receiver_thread->sockfd);
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

        int n = recv(receiver_thread->unicast_socket, receiver_thread->buffer, 1024, 0);
        printf("%s\n",receiver_thread->buffer);
        if (n < 0)
        {
            perror("recvfrom failed");
            //close(receiver_thread->unicast_socket);
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

    //close(receiver_thread->unicast_socket);
    //close(receiver_thread->sockfd);
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
    receiver->servaddr.sin_addr.s_addr = INADDR_ANY;
    receiver->servaddr.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(receiver->sockfd, (struct sockaddr *)&receiver->servaddr,
                                 sizeof(receiver->servaddr))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(receiver->sockfd, 10) < 0) {
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
    poller_t* test_poller = (poller_t*)malloc(sizeof(poller_t));
    poller_t* poller;
    uintptr_t padding = 0;
    // if ((char*)test_poller < (char*)test_poller->client)
    // {
    //     printf("Poller < client!\n");
    //     padding = (char*)test_poller->client - (char*)test_poller;
    //     poller = (char*)received_client - (char*)padding;
    // }
    // else
    // {
    //     printf("Poller >= client!\n");
    //     padding = (uintptr_t)test_poller - (uintptr_t)test_poller->client;
    //     poller = (poller_t*)((uintptr_t)received_client + padding);
    // }

    poller = container_of(received_client, poller_t, client);
    
    printf("callback poller = %p\n", (char*)poller);
    printf("callback poller->client = %p\n", (char*)received_client);
    printf("Offset of client: %zu\n", offsetof(poller_t, client));
    printf("padding = %p\n", (char*)padding);
    free(test_poller);
    pthread_mutex_lock(&poller->mutex);
    printf("Main thread setting flag to true and waking up waiting thread...\n");
    poller->flag = true;
    pthread_cond_signal(&poller->cond);
    pthread_mutex_unlock(&poller->mutex);
}

void wait_for_message(receiver_t* receiver, poller_t* poller)
{
    poller->flag = false;
    pthread_mutex_init(&poller->mutex, NULL);
    pthread_cond_init(&poller->cond, NULL);

    poller->client.callBack = callBack;
    subscribe_for_message(receiver, &poller->client);
    pthread_mutex_lock(&poller->mutex);

    while (!poller->flag)
    {
        pthread_cond_wait(&poller->cond, &poller->mutex);
    }
    printf("Thread woke up and flag is true!\n");
    
    pthread_mutex_unlock(&poller->mutex);

    unsubscribe_for_message(receiver, &poller->client);
}

void wait_for_message_untill(receiver_t* receiver, poller_t* poller, int timeout)
{
    struct timespec ts;
    int rc;

    poller->flag = false;
    pthread_mutex_init(&poller->mutex, NULL);
    pthread_cond_init(&poller->cond, NULL);

    poller->client.callBack = callBack;
    subscribe_for_message(receiver, &poller->client);

    pthread_mutex_lock(&poller->mutex);

    // Get the current time
    clock_gettime(CLOCK_REALTIME, &ts);

    // Set the timeout (e.g., 5 seconds from now)
    ts.tv_sec += timeout;

    // Wait on the condition variable with a timeout
    rc = pthread_cond_timedwait(&poller->cond, &poller->mutex, &ts);

    if (rc == ETIMEDOUT)
    {
        printf("Timeout occurred!\n");
    }
    else if (rc != 0)
    {
        fprintf(stderr, "Error waiting on condition variable: %d\n", rc);
    } else 
    {
        printf("Condition variable signaled!\n");
    }

    pthread_mutex_unlock(&poller->mutex);

    unsubscribe_for_message(receiver, &poller->client);
}
