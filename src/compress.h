#ifndef COMPRESS_H
#define COMPRESS_H

// ---------------------------------------------------------------------------
// Compression module — handles converting raw BGR pixels to/from JPEG
// ---------------------------------------------------------------------------

// Compresses a raw 24-bit BGR pixel buffer into a JPEG buffer in memory.
// Quality is between 1 and 100.
// The caller must free() the returned pointer.
// Returns NULL on failure, and sets *out_size to the size of the JPEG buffer.
unsigned char *compress_frame_to_jpeg(const unsigned char *pixels, int width, int height, int quality, int *out_size);

// Decompresses a JPEG buffer from memory into a raw 24-bit BGR pixel buffer.
// The caller must free() the returned pointer.
// Returns NULL on failure, and sets *out_width and *out_height.
unsigned char *decompress_jpeg_to_frame(const unsigned char *jpeg_data, int jpeg_size, int *out_width, int *out_height);

#endif // COMPRESS_H
