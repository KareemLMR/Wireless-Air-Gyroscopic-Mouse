#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "receiver.h"
#include "transmitter.h"

Display *display;
int screen_num;

Window root;
int mouse_x, mouse_y;

void get_mouse_position(Display *display, Window root, int *x, int *y) {
    Window root_return, child_return;
    int root_x, root_y;
    unsigned int mask_return;

    if (XQueryPointer(display, root, &root_return, &child_return, &root_x, &root_y, x, y, &mask_return)) {
        // Coordinates of the mouse pointer relative to the root window
        printf("Mouse position: (%d, %d)\n", *x, *y);
    } else {
        fprintf(stderr, "Failed to query mouse position.\n");
    }
}

void move_mouse(Display *display, int x, int y) {
    XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, x, y);
    XFlush(display);
}

void moveMouse(struct sockaddr_in cliaddr, client_t* received_client, char** args)
{
    int x = atoi(args[0]);
    int y = atoi(args[1]);

    // Move the mouse pointer to (x, y) coordinates
    get_mouse_position(display, root, &mouse_x, &mouse_y);
    printf("Mouse position before move: x = %d, y = %d\n", mouse_x, mouse_y);
    
    move_mouse(display, x, y);

    get_mouse_position(display, root, &mouse_x, &mouse_y);
    printf("Mouse position after move: x = %d, y = %d\n", mouse_x, mouse_y);
}

int main()
{
    // Ensure DISPLAY environment variable is set
    const char *display_env = getenv("DISPLAY");
    if (display_env == NULL) {
        fprintf(stderr, "Error: DISPLAY environment variable is not set.\n");
        return 1;
    }

    display = XOpenDisplay(display_env);
    if (display == NULL) {
        fprintf(stderr, "Error: Could not open display %s.\n", display_env);
        return 1;
    }

    screen_num = DefaultScreen(display);
    root = RootWindow(display, screen_num);

    receiver_t* initial_receiver = malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    init_listener_for_broadcast(initial_receiver);

    printf("Entering while loop\n");
    while (1)
    {
        strcpy(poller->client.msg, "CONNRQ");
        printf("poller = %p\n", (char*)poller);
        printf("poller->client = %p\n", (char*)&poller->client);
        printf("Waiting for a Connection Request\n");
        wait_for_message(initial_receiver, poller);
        if (1)
        {
            printf("Received a Connection Request\n");

            transmitter_t* acceptor = (transmitter_t*)malloc(sizeof(transmitter_t));
            strcpy(acceptor->ip, inet_ntoa(initial_receiver->cliaddr.sin_addr));
            acceptor->port = ntohs(initial_receiver->cliaddr.sin_port);
            acceptor->port = 12345;
            init_unicast_sender(acceptor);

            printf("Sending ACCRQ to %s:%d\n", acceptor->ip, acceptor->port);
            send_message(acceptor, "ACCRQ");
            
            receiver_t* response_receiver = malloc(sizeof(receiver_t));
            init_unicast_listener(response_receiver);
            strcpy(poller->client.msg, "ACK");
            wait_for_message_untill(response_receiver, poller, 5);

            if (poller->flag)
            {
                printf("Received ACK\n");
                strcpy(poller->client.msg, "MVMSE");
                poller->client.callBack = moveMouse;
                subscribe_for_message(response_receiver, &poller->client);
                while (1) //Infinite loop will be replaced by a stop condition
                {
                    printf("Inside loop\n");
                    sleep(1);
                }
            }
            else
            {
                printf("Received NACK\n");
            }

        }
    }
    

    return 0;
}