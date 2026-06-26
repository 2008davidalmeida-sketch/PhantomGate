# PhantomGate Architecture & Implementation Details

PhantomGate is an open-source remote desktop application written in C. It captures a host machine's screen, streams it over a TCP connection using Winsock2, and displays it in real-time on a client machine using the Win32 API.

This document breaks down how the entire process works, detailing the flow, each file, and every major function.

---

## High-Level Data Flow

1. **Host Setup**: The host opens a local TCP port and waits for a client.
2. **Client Connection**: The client connects to the host's IP and port.
3. **Capture Loop (Host)**: The host continuously captures the desktop into an in-memory pixel buffer at approximately 30 frames per second.
4. **Transmission**: The host packages the raw pixel buffer with a small metadata header and sends it over the TCP socket.
5. **Reception (Client)**: The client reads the header, allocates memory, and reads the exact number of bytes for the incoming frame.
6. **Rendering (Client)**: The client passes the pixel data to a Win32 window, which scales and draws the frame onto the screen.

---

## File-by-File Breakdown

### 1. `src/main.c` — The Entry Point & Loops
This file contains the main application logic, handling command-line arguments and orchestrating the infinite loops for both the host and client modes.

* **`main(int argc, char *argv[])`**
  * Calls `SetProcessDPIAware()` to tell Windows not to scale the application. This ensures that when we ask for the screen dimensions, we get the true physical pixels (e.g., 1920x1080) instead of a scaled-down version (e.g., 1536x864).
  * Parses arguments to determine if the program should run as `--host` or `--client`.

* **`run_host(uint16_t port)`**
  * Calls `net_host_init()` to start listening for a connection. This blocks until a client connects.
  * Enters an infinite `while (1)` loop:
    1. Calls `capture_frame()` to get a fresh image of the desktop.
    2. Calls `net_send_frame()` to push it to the client.
    3. Calculates elapsed time and calls `Sleep()` to throttle the loop to ~30 FPS (33ms per frame).
    4. Prints the actual framerate to the console every second.

* **`run_client(const char *ip, uint16_t port)`**
  * Calls `net_client_connect()` to connect to the host.
  * Receives exactly one initial frame via `net_recv_frame()` so it knows the remote screen's dimensions.
  * Calls `viewer_init()` to create the GUI window (sized to half the host's resolution initially).
  * Enters a `while (viewer_poll_events())` loop:
    1. Waits for and receives the next frame from the network.
    2. Sends the frame to the window using `viewer_render_frame()`.
    3. Calculates the FPS and updates the window's title bar every second.

---

### 2. `src/capture.c` — Screen Grabbing
This module is responsible for capturing the screen using the Windows GDI (Graphics Device Interface).

* **`capture_frame(int *out_width, int *out_height)`**
  * Uses `GetDC(NULL)` to get the device context for the entire screen.
  * Queries `HORZRES` and `VERTRES` for the screen width and height.
  * Creates an in-memory device context and a compatible bitmap.
  * Uses `BitBlt` with `SRCCOPY` to blast the screen pixels onto the memory bitmap (this is extremely fast).
  * Sets up a `BITMAPINFOHEADER` requesting a 24-bit BGR format. Crucially, the height is negative (`-height`), which tells Windows to return the image "top-down" (matching standard screen coordinates) rather than BMP's default "bottom-up".
  * Allocates a heap buffer (`malloc`) and uses `GetDIBits` to extract the raw pixel bytes into the buffer.
  * Cleans up GDI objects to prevent memory leaks and returns the buffer.

* **`capture_screenshot(const char *filename)`**
  * A utility function (primarily for debugging). It calls `capture_frame()`, prepends a `BITMAPFILEHEADER` and `BITMAPINFOHEADER` to the raw pixels, and writes them to a standard `.bmp` file on disk.

---

### 3. `src/network.c` — TCP Transport Layer
This file abstracts away the complexities of Winsock2, establishing the TCP connection and defining how frames are packed.

**The Wire Protocol**:
Before sending the massive pixel array, the host sends a 16-byte header so the client knows exactly what is coming:
* Magic bytes: `0x504E5447` (`"PNTG"`) — ensures the client is actually talking to a PhantomGate host.
* Length: How many bytes the pixel data takes up.
* Width: Frame width in pixels.
* Height: Frame height in pixels.

* **`winsock_init()`**: A private helper that calls `WSAStartup` to activate networking on Windows.
* **`send_all()` / `recv_all()`**: TCP is a stream protocol; a single `send()` or `recv()` call might only transfer part of a massive 6MB frame. These helpers loop around the socket functions to guarantee every single byte is transferred before moving on.
* **`net_host_init(uint16_t port)`**: Standard TCP server setup: `socket()`, `bind()`, `listen()`, and `accept()`.
* **`net_client_connect(...)`**: Standard TCP client setup: `socket()` and `connect()`.
* **`net_send_frame(...)`**: Packs the width, height, magic bytes, and data length into an array, sends the header via `send_all()`, and then pushes the raw pixel buffer.
* **`net_recv_frame(...)`**: Reads the 16-byte header, verifies the magic bytes to ensure data isn't corrupted, allocates a buffer using the specified length, and receives the pixel bytes. Returns the buffer.

---

### 4. `src/viewer.c` — The GUI & Renderer
This module creates a native Win32 window to display the stream.

* **Global State**: The file holds the window handle (`g_hwnd`) and pointers to the most recent frame (`g_frame_pixels`, width, height) in static global variables. Since the client is single-threaded, this is safe.

* **`viewer_init(int width, int height, const char *title)`**
  * Sets up a `WNDCLASSEX` and registers it.
  * Calls `AdjustWindowRect` to calculate how big the actual window needs to be so that the *inner drawing area* matches the requested width/height.
  * Calls `CreateWindowEx` to show the window.

* **`WndProc(HWND hwnd, UINT msg, ...)`** (The Window Procedure)
  * This is the callback function Windows invokes for window events.
  * **`WM_PAINT`**: Fired whenever the window needs to be redrawn (e.g., resizing, uncovering). It takes the global `g_frame_pixels` and uses `StretchDIBits` to aggressively scale the raw pixels to fit whatever size the client window currently is.
  * **`WM_ERASEBKGND`**: We intercept this and return `1`. By default, Windows paints a white background before every frame, which causes severe flickering. Intercepting it stops the flicker.
  * **`WM_DESTROY`**: Triggers when the user clicks the "X", shutting down the client loop.

* **`viewer_render_frame(...)`**
  * Updates the global pointers to point to the newest frame received from the network.
  * Calls `InvalidateRect(g_hwnd, NULL, FALSE)` and `UpdateWindow(g_hwnd)`. This forces Windows to immediately fire a `WM_PAINT` message, which triggers `StretchDIBits` to draw the frame.

* **`viewer_poll_events()`**
  * Standard Win32 message pump. It uses `PeekMessage` to grab any pending OS events (like mouse clicks, dragging the window, resizing) and dispatches them to `WndProc`.
  * Without this function, the window would freeze, turn white, and display a "Not Responding" error.
