# PhantomGate

Open-source remote desktop software built in C — screen capture, video streaming, and input injection over TCP.

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
gcc main.c capture.c -o rdp.exe -lgdi32
```

---

## Usage

### Host mode
Run this on the machine you want to control remotely.  
Captures a screenshot every second in a continuous loop:

```bash
./rdp.exe --host
```

Output example:
```
[HOST] Starting...
Screenshot saved to screenshot.bmp (1536x864)
[HOST] Frame 1 captured
Screenshot saved to screenshot.bmp (1536x864)
[HOST] Frame 2 captured
...
```

### Client mode
Run this on the machine you want to connect from:

```bash
./rdp.exe --client
```

> ⚠️ Client mode is not yet implemented.

---

## Project Structure

```
PhantomGate/
├── src/
│   ├── main.c        → entry point, arg parsing, host/client logic
│   ├── capture.c     → screen capture (Win32 BitBlt)
│   ├── capture.h     → header for capture.c
│   ├── network.c     → Winsock networking (next milestone)
│   └── network.h     → header for network.c
├── .gitignore
└── README.md
```

---

## Roadmap

- [x] Screen capture via Win32 BitBlt
- [x] Continuous frame capture loop (1 fps)
- [x] Modular source structure (capture / network split)
- [ ] Send frames over TCP (Winsock)
- [ ] Video encoding with FFmpeg
- [ ] Keyboard and mouse input injection
- [ ] Real-time streaming
- [ ] NAT traversal / relay server

---

## License

MIT