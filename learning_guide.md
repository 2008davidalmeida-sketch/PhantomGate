# PhantomGate — Core Concepts & Learning Guide

To fully grasp how PhantomGate works under the hood, you need an understanding of several overlapping domains: the C programming language, the Windows Operating System API, network programming, and performance engineering. 

Here is a comprehensive breakdown of everything you need to know.

---

## 1. Core C Programming Concepts

PhantomGate relies heavily on low-level C mechanics to manipulate memory and data efficiently.

* **Manual Memory Management (`malloc` & `free`)**: Grabbing a 1080p frame requires allocating roughly 6MB of memory per frame. Understanding heap allocation, and rigorously freeing that memory after it is sent or rendered, is critical to avoid memory leaks.
* **Pointers & Buffer Manipulation**: Passing around `unsigned char *pixels` to transfer raw byte data between functions.
* **Structs & Memory Layout**: Grouping related data (like `BITMAPINFOHEADER` or `sockaddr_in`). You must understand how C pads structs and how to interpret byte sizes using `sizeof()`.
* **Bitwise Operations**: Calculating BMP row padding requires bitwise math: `((width * 3 + 3) & ~3)`. This ensures each row of pixels is perfectly aligned to a 4-byte boundary, which Windows graphics APIs strictly require.
* **Endianness**: Understanding that Windows (x86/x64) is "little-endian". When we send a 32-bit integer like `0x504E5447` over the network, it is stored backward in memory.

---

## 2. Windows OS & Graphics (Win32 API & GDI)

To capture the screen and draw windows, PhantomGate talks directly to the Windows OS without using modern heavy frameworks.

* **DPI Awareness (`SetProcessDPIAware`)**: By default, Windows lies to applications about screen resolution if the user has display scaling turned on (e.g., 150% scaling). This function forces Windows to give us the true physical pixel count.
* **Device Contexts (HDC)**: An HDC is essentially a "canvas" managed by Windows. We use `GetDC(NULL)` to get the canvas of the entire monitor, and memory DCs to manipulate images in the background.
* **`BitBlt` (Bit Block Transfer)**: The absolute fastest way to copy pixels from one place to another in Windows. We use it to instantly clone the screen into our memory.
* **DIBs (Device-Independent Bitmaps)**: Using `GetDIBits` to extract the raw BGR (Blue, Green, Red) byte arrays from the Windows canvas, and `StretchDIBits` to draw those raw bytes onto the client window, scaling it up or down automatically.
* **The Message Pump (`PeekMessage`, `DispatchMessage`)**: Windows are event-driven. A window must continuously poll the OS for messages (mouse clicks, resize events) in a loop. If it stops, the window freezes.
* **Window Procedures (`WndProc`)**: The callback function where we handle events like `WM_PAINT` (time to draw the screen) and `WM_DESTROY` (time to close).
* **Flicker Prevention (`WM_ERASEBKGND`)**: Understanding that Windows clears the window to a solid color before painting. We intercept this command and tell Windows *not* to clear it, which eliminates screen flickering when painting video frames rapidly.

---

## 3. Network Engineering (Winsock2)

Getting bytes from one computer to another reliably over the internet.

* **TCP vs UDP**: PhantomGate uses TCP (Transmission Control Protocol). TCP guarantees that bytes will arrive perfectly in order without corruption, which is vital for our custom framing. (Modern video streams often use UDP because speed is favored over perfect reliability, but UDP requires complex packet-loss handling).
* **Sockets API**: The standard flow of networking:
  * **Server (Host)**: `socket()` ➔ `bind()` ➔ `listen()` ➔ `accept()`
  * **Client**: `socket()` ➔ `connect()`
* **Stream Paradigm & Partial I/O**: TCP does not know what a "frame" is; it only sees a continuous stream of bytes. When you ask TCP to `send()` or `recv()` 6MB of data, it might only process 60KB at a time. Understanding how to wrap these calls in loops (like our `send_all` and `recv_all`) is fundamental to network programming.
* **Application-Layer Framing**: Because TCP is just a stream, we have to define our own "protocol". We prepend every frame with a 16-byte header:
  * `[Magic Word] [Data Length] [Width] [Height]`
  * The Magic Word (`"PNTG"`) acts as a sanity check so we don't accidentally try to allocate 4 billion bytes of RAM if a random bot connects and sends garbage data.

---

## 4. Performance & Systems Engineering

Real-time streaming is all about fighting latency and bandwidth saturation.

* **Bandwidth Calculation**: 
  * A 1920x1080 screen has 2,073,600 pixels.
  * 3 bytes per pixel (Red, Green, Blue) = ~6.2 MB per frame.
  * At 30 FPS, that is **186 Megabytes per second (MB/s)**.
  * Understanding that a standard gigabit LAN can barely handle this, and Wi-Fi/Internet will instantly choke. This highlights the absolute necessity of adding video compression (like JPEG/H.264) in the future.
* **Frame Pacing (`Sleep`, `GetTickCount`)**: If we capture as fast as the CPU allows, we will crush the CPU to 100% and overwhelm the network buffer. We calculate how long a capture takes and `Sleep()` for the remainder of a 33.3ms window to ensure a smooth, polite 30 FPS.
* **Disk I/O vs. Memory**: Understanding why the initial version of PhantomGate (which wrote `.bmp` files to the hard drive) was unsuited for streaming. Disk writing is orders of magnitude slower than passing pointers to RAM buffers.
