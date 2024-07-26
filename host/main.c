#include "receiver.h"
#include "transmitter.h"

int main()
{
    receiver_t* initial_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    client_t* client = (client_t*)malloc(sizeof(client_t));
    init_listener_for_broadcast(initial_receiver);
    client->msg = "CONNRQ";
    poller->client = client;
    printf("Expected Client = %p\n", (char*)client);
    printf("Expected Poller = %p\n", (char*)poller);

    printf("Size of client_t: %zu\n", sizeof(client_t));
    printf("Size of poller_t: %zu\n", sizeof(poller_t));
    printf("Offset of client in poller_t: %zu\n", offsetof(poller_t, client));
    printf("Size of poller_t: %lu\n", sizeof(poller_t));
    printf("Size of client*: %lu\n", sizeof(client_t*));
    printf("Size of mtx_t: %lu\n", sizeof(mtx_t));
    printf("Size of cnd_t: %lu\n", sizeof(cnd_t));
    printf("Size of bool: %lu\n", sizeof(bool));
    wait_for_message(initial_receiver, poller);
    if (poller->flag)
    {
        printf("Received a Connection Request\n");
    }
    return 0;
}