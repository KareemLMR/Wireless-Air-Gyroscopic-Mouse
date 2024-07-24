#include "receiver.h"
mtx_t mutex;
cnd_t cond;
bool flag = false;
void callBack()
{
    printf("Matched!\n");
    mtx_lock(&mutex);
    printf("Main thread setting flag to true and waking up waiting thread...\n");
    flag = true;
    cnd_signal(&cond);
    mtx_unlock(&mutex);
}
int main()
{
    mtx_init(&mutex, mtx_plain);
    cnd_init(&cond);
    receiver_t* initial_receiver = malloc(sizeof(receiver_t));
    init_listener_for_broadcast(initial_receiver);
    client_t* client;
    client->msg = (char*)malloc(100 * sizeof(char));
    strcpy(client->msg, "CONNRQ");
    client->callBack = callBack;
    subscribe_for_message(initial_receiver, client);
    while(1)
    {
        mtx_lock(&mutex);
    
        while (!flag)
        {
            cnd_wait(&cond, &mutex);
        }
        flag = false;
        printf("Thread woke up and flag is true!\n");
        
        mtx_unlock(&mutex);
    }
    
    return 0;
}