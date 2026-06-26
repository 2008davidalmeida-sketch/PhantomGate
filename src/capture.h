#ifndef CAPTURE_H
#define CAPTURE_H

// Saves a BMP snapshot to disk. Useful for debugging.
void capture_screenshot(const char *filename);

// Captures the screen into a heap-allocated 24-bit BGR pixel buffer (top-down,
// row-major). The caller is responsible for calling free() on the returned
// pointer. Returns NULL on failure. Sets *out_width and *out_height.
unsigned char *capture_frame(int *out_width, int *out_height);

#endif // CAPTURE_H
