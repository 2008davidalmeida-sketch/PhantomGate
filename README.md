# PhantomGate

Open-source remote desktop software built in C — screen capture, real-time video streaming, and input injection over TCP.

Connect to any machine and use it as if you were there.

---

## Requirements

- Windows
- [MinGW-W64 GCC](https://github.com/brechtsanders/winlibs_mingw)

---

## Build

Navigate to the `src/` folder and compile all source files together:

```bash
cd src
gcc main.c capture.c network.c viewer.c -o rdp.exe -lgdi32 -lws2_32
```

---

## Usage

### Host mode
Run this on the machine you want to control remotely.  
Captures and streams frames at ~30 fps over TCP:

```bash
./rdp.exe --host <port>
# Example:
./rdp.exe --host 5900
```

Output example:
```
[HOST] Waiting for client on port 5900...
[net] Client connected.
[HOST] Streaming — press Ctrl+C to stop.
[HOST] 28.4 fps
[HOST] 29.1 fps
```

### Client mode
Run this on the machine you want to connect from:

```bash
./rdp.exe --client <ip> <port>
# Example (loopback):
./rdp.exe --client 127.0.0.1 5900
```

A window will open showing the live screen of the host.  
The title bar displays the current frame rate.

---

## Project Structure

```
PhantomGate/
├── src/
│   ├── main.c        → entry point, arg parsing, host/client streaming loops
│   ├── capture.c     → screen capture (Win32 BitBlt) — in-memory & file output
│   ├── capture.h     → header for capture.c
│   ├── network.c     → Winsock2 TCP framing (send/recv frames)
│   ├── network.h     → header for network.c
│   ├── viewer.c      → Win32 window renderer (StretchDIBits)
│   └── viewer.h      → header for viewer.c
├── .gitignore
└── README.md
```

---

## Wire Protocol

Each frame is sent as:

```
[4 bytes]  magic   = 0x504E5447  ("PNTG")
[4 bytes]  length  = byte count of pixel data
[4 bytes]  width   = frame width  in pixels
[4 bytes]  height  = frame height in pixels
[N bytes]  pixels  = raw 24-bit BGR, top-down, rows padded to 4 bytes
```

---

## Roadmap

- [x] Screen capture via Win32 BitBlt
- [x] In-memory frame capture API (`capture_frame`)
- [x] TCP networking layer (Winsock2)
- [x] Real-time frame streaming (~30 fps)
- [x] Win32 viewer window with FPS counter
- [ ] JPEG/PNG frame compression (reduce bandwidth)
- [ ] Keyboard and mouse input injection
- [ ] Multi-client broadcast
- [ ] NAT traversal / relay server

---

## License

MIT