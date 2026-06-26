#ifndef VIEWER_H
#define VIEWER_H

// ---------------------------------------------------------------------------
// viewer — Win32 window that renders incoming screen frames in real time.
// ---------------------------------------------------------------------------

// Opens a resizable Win32 window sized to *width* x *height*.
// *title* is the initial window title.
// Must be called before viewer_render_frame() or viewer_poll_events().
void viewer_init(int width, int height, const char *title);

// Blits a raw 24-bit BGR pixel buffer (top-down, row-padded to 4 bytes) into
// the viewer window, scaled to fit.  Call this once per received frame.
void viewer_render_frame(const unsigned char *pixels, int width, int height);

// Pumps the Win32 message queue.  Call this every frame on the same thread
// that called viewer_init().
// Returns 1 while the window is open, 0 after the user closes it.
int viewer_poll_events(void);

// Updates the window title (e.g. to show the current FPS).
void viewer_set_title(const char *title);

#endif // VIEWER_H
