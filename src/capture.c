#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "capture.h"

// ---------------------------------------------------------------------------
// capture_frame — captures the screen into a malloc'd 24-bit BGR pixel buffer.
// The caller must free() the returned pointer.
// Returns NULL on failure.
// ---------------------------------------------------------------------------
unsigned char *capture_frame(int *out_width, int *out_height) {
    
    // Get the screen device context so we can query physical pixel dimensions.
    HDC screen_dc = GetDC(NULL);

    //Check if the screen device context was retrieved successfully.
    if (!screen_dc) return NULL;

    //Get the screen width and height.
    int width  = GetDeviceCaps(screen_dc, HORZRES);
    int height = GetDeviceCaps(screen_dc, VERTRES);

    // Memory DC + bitmap to receive the BitBlt copy.
    HDC mem_dc = CreateCompatibleDC(screen_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(screen_dc, width, height);
    HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, bitmap);

    // Copy the screen pixels into the bitmap.
    BitBlt(mem_dc, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);

    // Describe the pixel format we want back: 24-bit BGR, top-down.
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // negative = top-down row order
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    // BMP rows are padded to a 4-byte boundary.
    DWORD row_size = ((width * 3 + 3) & ~3);
    DWORD pixel_size = row_size * height;

    //Allocate memory to store the pixel data.
    unsigned char *pixels = malloc(pixel_size);

    //Check if the memory allocation was successful.
    if (!pixels) {
        fprintf(stderr, "[capture] malloc failed (%lu bytes)\n", pixel_size);
        goto cleanup;
    }

    // Copy pixel data from the bitmap to the malloc'd buffer in 24-bit BGR format.
    GetDIBits(mem_dc, bitmap, 0, height, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    //Set the output width and height.
    if (out_width) { *out_width = width; }
    if (out_height) { *out_height = height; }

cleanup:
    SelectObject(mem_dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(mem_dc);
    ReleaseDC(NULL, screen_dc);

    return pixels; // may be NULL if malloc failed above
}

// ---------------------------------------------------------------------------
// capture_screenshot — thin wrapper that saves a frame to a BMP file.
// Useful for debugging; not used in the streaming path.
// ---------------------------------------------------------------------------
void capture_screenshot(const char *filename) {
    
    //Capture the screen.
    int width = 0, height = 0;
    unsigned char *pixels = capture_frame(&width, &height);
    
    //Check if the screen capture was successful.
    if (!pixels) {
        fprintf(stderr, "[capture] capture_frame failed\n");
        return;
    }

    // BMP rows are padded to 4 bytes.
    DWORD row_size = ((width * 3 + 3) & ~3);
    DWORD pixel_size = row_size * height;

    // Create a BITMAPFILEHEADER to describe the BMP file header.
    BITMAPFILEHEADER bf = {0};
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(bf) + sizeof(BITMAPINFOHEADER) + pixel_size;
    bf.bfOffBits = sizeof(bf) + sizeof(BITMAPINFOHEADER);

    //Create a BITMAPINFOHEADER to describe the BMP file format.
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    // Open the file for writing.
    FILE *f = fopen(filename, "wb");

    //Check if the file was opened successfully.
    if (!f) {
        fprintf(stderr, "[capture] failed to open %s for writing\n", filename);
        free(pixels);
        return;
    }

    //Write the BMP file header and pixel data to the file.
    fwrite(&bf, sizeof(bf), 1, f);
    fwrite(&bi, sizeof(bi), 1, f);
    fwrite(pixels, pixel_size, 1, f);
    fclose(f);

    printf("Screenshot saved to %s (%dx%d)\n", filename, width, height);
    free(pixels);
}
