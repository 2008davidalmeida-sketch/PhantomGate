#include <winsock2.h>
// winsock2.h must come before windows.h
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "network.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Initialises Winsock2 once.  Called automatically by host/client init.
static int winsock_init(void) {
    
    // Define the version of Winsock to use.
    WSADATA wsa;
   
    //Start thewinsock service.
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[net] WSAStartup failed: %d\n", WSAGetLastError());
        return 0;
    }
    return 1;
}

// Sends exactly *len* bytes from *buf* over *sock*, handling short writes.
// Returns 1 on success, 0 on failure.
static int send_all(SOCKET sock, const unsigned char *buf, int len) {

    // Initialise the number of bytes sent.
    int sent = 0;

    // Send the data.
    while (sent < len) {
        // Check if the data was sent successfully.
        int n = send(sock, (const char *)buf + sent, len - sent, 0);
        if (n == SOCKET_ERROR || n == 0) {
            fprintf(stderr, "[net] send failed: %d\n", WSAGetLastError());
            return 0;
        }
        // Update the number of bytes sent.
        sent += n;
    }
    return 1;
}

// Receives exactly *len* bytes into *buf* from *sock*, handling short reads.
// Returns 1 on success, 0 on failure / connection closed.
static int recv_all(SOCKET sock, unsigned char *buf, int len) {

    // Initialise the number of bytes received.
    int received = 0;
    
    // Receive the data.
    while (received < len) {
        int n = recv(sock, (char *)buf + received, len - received, 0);
        if (n == SOCKET_ERROR || n == 0) {
            if (n == SOCKET_ERROR)
                fprintf(stderr, "[net] recv failed: %d\n", WSAGetLastError());
            return 0;
        }
        received += n;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Host (server) side
// ---------------------------------------------------------------------------

SOCKET net_host_init(uint16_t port) {
    
    // Initialise Winsock.
    if (!winsock_init()) return INVALID_SOCKET;

    // Create a socket.
    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Check if the socket was created successfully.
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "[net] socket() failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    // Allow the port to be reused immediately after a restart.
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    // Define the address structure.
    // Define the address structure.
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    // Bind the socket to the address.
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[net] bind() failed: %d\n", WSAGetLastError());
        closesocket(server);
        return INVALID_SOCKET;
    }

    // Listen for incoming connections.
    if (listen(server, 1) == SOCKET_ERROR) {
        fprintf(stderr, "[net] listen() failed: %d\n", WSAGetLastError());
        closesocket(server);
        return INVALID_SOCKET;
    }

    printf("[net] Listening on port %u — waiting for client...\n", port);

    // Accept the connection.
    SOCKET client = accept(server, NULL, NULL);
    
    // Close the server socket.
    closesocket(server); // we only need one connection
    if (client == INVALID_SOCKET) {
        fprintf(stderr, "[net] accept() failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    printf("[net] Client connected.\n");
    return client;
}

// ---------------------------------------------------------------------------
// Client side
// ---------------------------------------------------------------------------

SOCKET net_client_connect(const char *ip, uint16_t port) {
    
    // Check if Winsock was initialized.
    if (!winsock_init()) return INVALID_SOCKET;

    // Create a socket.
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    // Check if the socket was created successfully.
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[net] socket() failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    // Define the address structure.
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    // Connect to the host.
    printf("[net] Connecting to %s:%u...\n", ip, port);

    // Check if the connection was successful.
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[net] connect() failed: %d\n", WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    printf("[net] Connected to host.\n");
    return sock;
}

// ---------------------------------------------------------------------------
// Shared send / receive
// ---------------------------------------------------------------------------

// Frame header layout (all uint32_t, little-endian):
//   [0]  magic
//   [1]  pixel data length in bytes
//   [2]  width
//   [3]  height
#define HEADER_FIELDS 4

int net_send_frame(SOCKET sock, const unsigned char *payload, int payload_size,
                   int width, int height) {
    
    // Define the header.
    uint32_t header[HEADER_FIELDS];
    header[0] = NET_FRAME_MAGIC;
    header[1] = (uint32_t)payload_size;
    header[2] = (uint32_t)width;
    header[3] = (uint32_t)height;

    // Check if the header was sent successfully.
    if (!send_all(sock, (unsigned char *)header, sizeof(header))) return 0;
    
    // Check if the payload data was sent successfully.
    if (!send_all(sock, payload, payload_size)) return 0;
    
    return 1;
}

unsigned char *net_recv_frame(SOCKET sock, int *out_width, int *out_height, int *out_size) {
    
    // Receive the header.
    uint32_t header[HEADER_FIELDS];
    if (!recv_all(sock, (unsigned char *)header, sizeof(header))) return NULL;

    // Check if the header is valid.
    if (header[0] != NET_FRAME_MAGIC) {
        fprintf(stderr, "[net] bad magic: 0x%08X\n", header[0]);
        return NULL;
    }

    // Define the payload length, width, and height.
    uint32_t payload_size = header[1];
    int width             = (int)header[2];
    int height            = (int)header[3];

    // Check if the payload length is valid.
    if (payload_size == 0 || payload_size > 64u * 1024u * 1024u) {
        fprintf(stderr, "[net] implausible frame size: %u bytes\n", payload_size);
        return NULL;
    }

    // Allocate memory for the payload data.
    unsigned char *payload = malloc(payload_size);
    if (!payload) {
        fprintf(stderr, "[net] malloc(%u) failed\n", payload_size);
        return NULL;
    }

    // Check if the payload data was received successfully.
    if (!recv_all(sock, payload, (int)payload_size)) {
        free(payload);
        return NULL;
    }

    // Set the width, height, and size.
    if (out_width)  *out_width  = width;
    if (out_height) *out_height = height;
    if (out_size)   *out_size   = (int)payload_size;

    // Return the payload data.
    return payload;
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

void net_cleanup(void) {
    
    // Clean up Winsock.
    WSACleanup();
}
