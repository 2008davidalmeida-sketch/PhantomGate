#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// network.h includes winsock2.h — must come before windows.h
#include "network.h"
#include <windows.h>
#include "capture.h"
#include "viewer.h"

// Target frame interval in milliseconds (~30 fps).
#define FRAME_MS 33

// ---------------------------------------------------------------------------
// Host mode:  capture frames and stream them to a connected client.
// Usage:  rdp.exe --host <port>
// ---------------------------------------------------------------------------
static int run_host(uint16_t port) {
    printf("[HOST] Waiting for client on port %u...\n", port);

    // Initialise network.
    SOCKET client = net_host_init(port);

    // Check if the network was initialised successfully.
    if (client == INVALID_SOCKET) {
        fprintf(stderr, "[HOST] Failed to initialise network.\n");
        return 1;
    }

    printf("[HOST] Streaming — press Ctrl+C to stop.\n");

    // Initialise variables.
    DWORD frame_count = 0;
    DWORD t_start = GetTickCount();

    // Loop to capture and send frames.
    while (1) {
        DWORD frame_start = GetTickCount();

        // Capture a frame.
        int width = 0, height = 0;
        unsigned char *pixels = capture_frame(&width, &height);
        
        // Check if the capture was successful.
        if (!pixels) {
            fprintf(stderr, "[HOST] capture_frame failed — skipping.\n");
            Sleep(FRAME_MS);
            continue;
        }

        // Send the frame to the client.
        if (!net_send_frame(client, pixels, width, height)) {
            fprintf(stderr, "[HOST] Client disconnected.\n");
            free(pixels);
            break;
        }
        free(pixels);

        //Update the frame count.
        frame_count++;

        // Calculate elapsed time and print FPS every second.
        DWORD elapsed = GetTickCount() - t_start;
        if (elapsed >= 1000) {
            printf("[HOST] %.1f fps\n", frame_count * 1000.0 / elapsed);
            frame_count = 0;
            t_start = GetTickCount();
        }

        // Pace the loop to the target frame rate.
        DWORD frame_ms = GetTickCount() - frame_start;
        if (frame_ms < FRAME_MS) Sleep(FRAME_MS - frame_ms);
    }

    // Close the socket and cleanup the network.
    closesocket(client);
    net_cleanup();
    return 0;
}

// ---------------------------------------------------------------------------
// Client mode:  receive frames from the host and display them in a window.
// Usage:  rdp.exe --client <ip> <port>
// ---------------------------------------------------------------------------
static int run_client(const char *ip, uint16_t port) {
    printf("[CLIENT] Connecting to %s:%u...\n", ip, port);

    //Initialise network.
    SOCKET sock = net_client_connect(ip, port);
    
    //Check if the connection was successful.
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[CLIENT] Failed to connect.\n");
        return 1;
    }

    // Receive one frame first so we know the host resolution.
    int width = 0, height = 0;
    unsigned char *first = net_recv_frame(sock, &width, &height);
   
    //Check if the first frame was received.
    if (!first) {
        fprintf(stderr, "[CLIENT] Failed to receive first frame.\n");
        closesocket(sock);
        net_cleanup();
        return 1;
    }

    //Initialise the viewer window with the received frame.
    viewer_init(width / 2, height / 2, "PhantomGate — connecting...");
    viewer_render_frame(first, width, height);
    free(first);

    //Initialise frame count and start time.
    DWORD frame_count = 0;
    DWORD t_start = GetTickCount();
    char title[128];

    //Loop to receive and display frames.
    while (viewer_poll_events()) {
        
        //Receive the next frame from the host.
        unsigned char *pixels = net_recv_frame(sock, &width, &height);
        
        //Check if the frame was received successfully.
        if (!pixels) {
            printf("[CLIENT] Stream ended or connection lost.\n");
            break;
        }

        //Render the received frame in the viewer window.
        viewer_render_frame(pixels, width, height);
        free(pixels);

        //Increment the frame count.
        frame_count++;

        //Calculate elapsed time and print FPS every second.
        DWORD elapsed = GetTickCount() - t_start;
        if (elapsed >= 1000) {
            double fps = frame_count * 1000.0 / elapsed;
            snprintf(title, sizeof(title), "PhantomGate — %.1f fps", fps);
            viewer_set_title(title);
            frame_count = 0;
            t_start = GetTickCount();
        }
    }

    //Close the socket and cleanup the network.
    closesocket(sock);
    net_cleanup();
    return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    // Tells Windows to report real physical pixel dimensions, not DPI-scaled ones.
    SetProcessDPIAware();

    //Check if the correct number of arguments was provided.
    if (argc < 2) {
        printf("Usage:\n");
        printf("  rdp.exe --host <port>\n");
        printf("  rdp.exe --client <ip> <port>\n");
        return 1;
    }

    //Check if the mode is host.
    if (strcmp(argv[1], "--host") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: rdp.exe --host <port>\n");
            return 1;
        }
        uint16_t port = (uint16_t)atoi(argv[2]);
        return run_host(port);

    //Check if the mode is client.
    } else if (strcmp(argv[1], "--client") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: rdp.exe --client <ip> <port>\n");
            return 1;
        }
        const char *ip = argv[2];
        uint16_t port = (uint16_t)atoi(argv[3]);
        return run_client(ip, port);

    //Check if the mode is unknown.
    } else {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        printf("Usage:\n");
        printf("  rdp.exe --host <port>\n");
        printf("  rdp.exe --client <ip> <port>\n");
        return 1;
    }
}