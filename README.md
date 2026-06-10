# PhantomGate

Open-source remote desktop software built in C — screen capture, video streaming, and input injection over TCP.

Connect to any machine and use it as if you were there.

---

## Requirements

- Windows
- [MinGW-W64 GCC](https://github.com/brechtsanders/winlibs_mingw)

---

## Build

Navigate to the `src/` folder and compile:

```bash
cd src
gcc main.c -o rdp.exe -lgdi32
```

---

## Usage

### Host mode
Run this on the machine you want to control remotely:

```bash
./rdp.exe --host
```

### Client mode
Run this on the machine you want to connect from:

```bash
./rdp.exe --client
```

---

## Project Structure

```
PhantomGate/
├── src/
│   └── main.c
├── .gitignore
└── README.md
```

---

## Roadmap

- [x] Screen capture via Win32 BitBlt
- [ ] Send frames over TCP (Winsock)
- [ ] Video encoding with FFmpeg
- [ ] Keyboard and mouse input injection
- [ ] Real-time streaming
- [ ] NAT traversal / relay server

---

## License

MIT