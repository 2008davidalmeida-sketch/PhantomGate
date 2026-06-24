#include <windows.h>
#include <stdio.h>
#include "capture.h"

void capture_screenshot(const char *filename) {

    // Get the screen device context first so we can query physical pixel size
    HDC screen_dc = GetDC(NULL);

    // Get the screen dimensions
    int width  = GetDeviceCaps(screen_dc, HORZRES);
    int height = GetDeviceCaps(screen_dc, VERTRES);

    // Create a memory DC (Device Context) compatible with the screen DC
    HDC mem_dc = CreateCompatibleDC(screen_dc);

    // Create a bitmap to hold the screen pixels
    HBITMAP bitmap = CreateCompatibleBitmap(screen_dc, width, height);
    SelectObject(mem_dc, bitmap);

    // Copy screen into the bitmap
    BitBlt(mem_dc, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);

    // Save as BMP file
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // negative = top-down
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    // Calculate the size of the pixel data (rows are padded to 4 bytes)
    DWORD row_size = ((width * 3 + 3) & ~3); // rows are 4-byte aligned in BMP
    DWORD pixel_data = row_size * height;

    // Allocate memory for pixel data and get the bitmap bits
    unsigned char *pixels = malloc(pixel_data);
    GetDIBits(mem_dc, bitmap, 0, height, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    // Write the BMP file header and pixel data
    BITMAPFILEHEADER bf = {0};
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(bf) + sizeof(bi) + pixel_data;
    bf.bfOffBits = sizeof(bf) + sizeof(bi);

    // Write the file
    FILE *f = fopen(filename, "wb");
    fwrite(&bf, sizeof(bf), 1, f);
    fwrite(&bi, sizeof(bi), 1, f);
    fwrite(pixels, pixel_data, 1, f);
    fclose(f);

    // Clean up
    free(pixels);
    DeleteObject(bitmap);
    DeleteDC(mem_dc);
    ReleaseDC(NULL, screen_dc);

    printf("Screenshot saved to %s (%dx%d)\n", filename, width, height);
}
