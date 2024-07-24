#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <threads.h>
#include <sys/queue.h>
#include <pthread.h>

#define PORT 12345
#define BROADCAST_ADDR "192.168.1.255"  // Adjust this to your network's broadcast address

typedef struct client
{
    char* msg;
    void (*callBack)(void);
    SLIST_ENTRY(client) entries;

}client_t;

// static int sockfd;
// static struct sockaddr_in servaddr, cliaddr;
// static char buffer[1024];
// static socklen_t len = sizeof(cliaddr);
// static pthread_t thread;
// static atomic_bool is_initialized = ATOMIC_VAR_INIT(false);
// static pthread_mutex_t subscribers_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct receiver
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    char buffer[1024];
    socklen_t len;
    pthread_t thread;
    atomic_bool is_initialized;
    pthread_mutex_t subscribers_mutex;
    pthread_mutex_t server_mutex;
    SLIST_HEAD(headClient, client) clients;

}receiver_t;

bool init_listener_for_broadcast(receiver_t* receiver);
void* listen_for_broadcast(void* receiver);
void subscribe_for_message(receiver_t* receiver, client_t* client);
void unsubscribe_for_message(receiver_t* receiver, client_t* client);

#endif