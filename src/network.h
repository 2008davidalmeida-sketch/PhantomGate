#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Wire frame format (all multi-byte fields are little-endian):
//
//   [4 bytes]  magic   = 0x504E5447  ("PNTG")
//   [4 bytes]  length  = byte count of the pixel data that follows
//   [4 bytes]  width   = frame width  in pixels
//   [4 bytes]  height  = frame height in pixels
//   [N bytes]  pixels  = raw 24-bit BGR pixel data, top-down, row-padded to 4 bytes
// ---------------------------------------------------------------------------

#define NET_FRAME_MAGIC 0x504E5447u

// Host (server) side -----------------------------------------------------------

// Creates a TCP server socket, binds to *port*, and blocks until exactly one
// client connects.  Returns the accepted client SOCKET, or INVALID_SOCKET on
// error.
SOCKET net_host_init(uint16_t port);

// Client side ------------------------------------------------------------------

// Connects to a host at *ip* (dotted-decimal) on *port*.
// Returns a connected SOCKET, or INVALID_SOCKET on error.
SOCKET net_client_connect(const char *ip, uint16_t port);

// Shared send / receive -------------------------------------------------------

// Sends one frame (header + pixel data) over *sock*.
// Returns 1 on success, 0 on failure.
int net_send_frame(SOCKET sock, const unsigned char *pixels,
                   int width, int height);

// Receives one frame from *sock*.  Allocates and returns a pixel buffer that
// the caller must free().  Sets *out_width and *out_height.
// Returns NULL on error or if the connection was closed.
unsigned char *net_recv_frame(SOCKET sock, int *out_width, int *out_height);

// Tears down Winsock.  Call once at program exit.
void net_cleanup(void);

#endif // NETWORK_H
