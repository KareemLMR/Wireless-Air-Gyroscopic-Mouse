#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


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

int main() {
    Display *display;
    int screen_num;
    int x = 500;  // Example x coordinate
    int y = 300;  // Example y coordinate

    Window root;
    int mouse_x, mouse_y;

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

    // Move the mouse pointer to (x, y) coordinates
    get_mouse_position(display, root, &mouse_x, &mouse_y);
    printf("Mouse position before move: x = %d, y = %d\n", mouse_x, mouse_y);
    
    move_mouse(display, x, y);

    get_mouse_position(display, root, &mouse_x, &mouse_y);
    printf("Mouse position after move: x = %d, y = %d\n", mouse_x, mouse_y);

    // Close the display connection
    XCloseDisplay(display);

    return 0;
}



// #include <stdio.h>
// #include <stdlib.h>
// #include <X11/Xlib.h>

// int main() {
//     Display *display;
//     char *display_name = "192.168.1.9:0"; // Using localhost
//     Window root_window;
//     int screen_num;
//     int x, y;

//     // Open the display
//     display = XOpenDisplay(display_name);
//     if (display == NULL) {
//         fprintf(stderr, "Error: Could not open display %s.\n", display_name);
//         return 1;
//     }

//     // Get the root window and screen number
//     screen_num = DefaultScreen(display);
//     root_window = RootWindow(display, screen_num);

//     // Specify the new coordinates (in this example, move 100 pixels right and 50 pixels down)
//     x = 100;
//     y = 50;

//     // Move the pointer using XWarpPointer
//     XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);

//     // Flush all pending requests to the X server
//     XFlush(display);

//     // Close the display
//     XCloseDisplay(display);

//     return 0;
// }
