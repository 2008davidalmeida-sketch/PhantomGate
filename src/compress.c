#include <stdlib.h>
#include <string.h>
#include "compress.h"

// Define the implementations for stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ---------------------------------------------------------------------------
// Memory context for stbi_write_jpg_to_func
// ---------------------------------------------------------------------------
typedef struct {
    unsigned char *data;
    int size;
    int capacity;
} MemContext;

static void stbi_write_mem(void *context, void *data, int size) {
    MemContext *mc = (MemContext *)context;
    if (mc->size + size > mc->capacity) {
        mc->capacity = (mc->capacity == 0) ? (1024 * 1024) : (mc->capacity * 2);
        if (mc->size + size > mc->capacity) {
            mc->capacity = mc->size + size + (1024 * 1024);
        }
        mc->data = realloc(mc->data, mc->capacity);
    }
    memcpy(mc->data + mc->size, data, size);
    mc->size += size;
}

// ---------------------------------------------------------------------------
// Compression (BGR padded -> unpadded RGB -> JPEG)
// ---------------------------------------------------------------------------
unsigned char *compress_frame_to_jpeg(const unsigned char *pixels, int width, int height, int quality, int *out_size) {
    int padded_row_size = ((width * 3 + 3) & ~3);
    int unpadded_row_size = width * 3;

    // 1. Strip padding and convert BGR to RGB
    unsigned char *rgb = malloc(width * height * 3);
    if (!rgb) return NULL;

    for (int y = 0; y < height; y++) {
        const unsigned char *src_row = pixels + y * padded_row_size;
        unsigned char *dst_row = rgb + y * unpadded_row_size;
        for (int x = 0; x < width; x++) {
            dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // R
            dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // B
        }
    }

    // 2. Compress to memory
    MemContext mc = {0};
    mc.capacity = 1024 * 1024; // 1MB initial
    mc.data = malloc(mc.capacity);
    mc.size = 0;

    if (!stbi_write_jpg_to_func(stbi_write_mem, &mc, width, height, 3, rgb, quality)) {
        free(rgb);
        free(mc.data);
        return NULL;
    }

    free(rgb);

    *out_size = mc.size;
    return mc.data;
}

// ---------------------------------------------------------------------------
// Decompression (JPEG -> unpadded RGB -> padded BGR)
// ---------------------------------------------------------------------------
unsigned char *decompress_jpeg_to_frame(const unsigned char *jpeg_data, int jpeg_size, int *out_width, int *out_height) {
    int width, height, channels;
    
    // 1. Decode JPEG to unpadded RGB
    unsigned char *rgb = stbi_load_from_memory(jpeg_data, jpeg_size, &width, &height, &channels, 3);
    if (!rgb) return NULL;

    // 2. Add padding and convert RGB back to BGR
    int padded_row_size = ((width * 3 + 3) & ~3);
    int unpadded_row_size = width * 3;
    unsigned char *bgr = malloc(padded_row_size * height);
    if (!bgr) {
        stbi_image_free(rgb);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        const unsigned char *src_row = rgb + y * unpadded_row_size;
        unsigned char *dst_row = bgr + y * padded_row_size;
        for (int x = 0; x < width; x++) {
            dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // B
            dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // R
        }
    }

    stbi_image_free(rgb);

    if (out_width) *out_width = width;
    if (out_height) *out_height = height;
    
    return bgr;
}
