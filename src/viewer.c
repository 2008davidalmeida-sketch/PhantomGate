#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "viewer.h"

// ---------------------------------------------------------------------------
// Module state (single-window, single-threaded)
// ---------------------------------------------------------------------------

static HWND g_hwnd = NULL;
static int g_running = 0;

// The most recent frame — stored so WM_PAINT can redraw it.
static unsigned char *g_frame_pixels = NULL;
static int g_frame_width = 0;
static int g_frame_height = 0;

// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            g_running = 0;
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            // Re-render the last frame on expose / resize events.
            if (g_frame_pixels && g_frame_width > 0 && g_frame_height > 0) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                // Get the client area dimensions.
                RECT client;
                GetClientRect(hwnd, &client);
                int dst_w = client.right  - client.left;
                int dst_h = client.bottom - client.top;

                // Define the bitmap info header.
                BITMAPINFOHEADER bi = {0};
                bi.biSize        = sizeof(BITMAPINFOHEADER);
                bi.biWidth       = g_frame_width;
                bi.biHeight      = -g_frame_height; // top-down
                bi.biPlanes      = 1;
                bi.biBitCount    = 24;
                bi.biCompression = BI_RGB;

                // Stretch the dib to the client area.
                StretchDIBits(hdc,
                    0, 0, dst_w, dst_h,          // destination rect
                    0, 0, g_frame_width, g_frame_height, // source rect
                    g_frame_pixels,
                    (BITMAPINFO *)&bi,
                    DIB_RGB_COLORS,
                    SRCCOPY);

                EndPaint(hwnd, &ps);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            // Prevent white flash between frames.
            return 1;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// ---------------------------------------------------------------------------
// viewer_init
// ---------------------------------------------------------------------------
void viewer_init(int width, int height, const char *title) {
   
    // Get the instance handle.
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Define the window class.
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "PhantomGateViewer";

    // Register the window class.
    RegisterClassEx(&wc);

    // Calculate window size so the *client area* matches the requested size.
    RECT r = {0, 0, width, height};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    int win_w = r.right  - r.left;
    int win_h = r.bottom - r.top;

    // Create the window.
    g_hwnd = CreateWindowEx(
        0,
        "PhantomGateViewer",
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_w, win_h,
        NULL, NULL, hInstance, NULL
    );

    // Check if the window was created successfully.
    if (!g_hwnd) {
        fprintf(stderr, "[viewer] CreateWindowEx failed: %lu\n", GetLastError());
        return;
    }

    // Show the window.
    ShowWindow(g_hwnd, SW_SHOW);
    // Update the window.
    UpdateWindow(g_hwnd);
    // Set the running flag.
    g_running = 1;
}

// ---------------------------------------------------------------------------
// viewer_render_frame
// ---------------------------------------------------------------------------
void viewer_render_frame(const unsigned char *pixels, int width, int height) {
    
    // Check if the window handle or pixels are valid.
    if (!g_hwnd || !pixels) return;

    // Store the latest frame so WM_PAINT can use it.
    g_frame_pixels = (unsigned char *)pixels; // caller owns the buffer
    g_frame_width  = width;
    g_frame_height = height;

    // Force an immediate repaint.
    InvalidateRect(g_hwnd, NULL, FALSE);
    UpdateWindow(g_hwnd);
}

// ---------------------------------------------------------------------------
// viewer_poll_events
// ---------------------------------------------------------------------------
int viewer_poll_events(void) {
   
    // Check for messages.
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_running = 0;
            return 0;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return g_running;
}

// ---------------------------------------------------------------------------
// viewer_set_title
// ---------------------------------------------------------------------------
void viewer_set_title(const char *title) {
    
    // Check if the window handle is valid.
    if (g_hwnd) SetWindowText(g_hwnd, title);
}
