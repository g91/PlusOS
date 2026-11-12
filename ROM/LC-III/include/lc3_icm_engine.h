/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Image Compression Manager (ICM) Core Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ICM_ENGINE_H
#define LC3_ICM_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define ICM_NO_ERR             0
#define ICM_CODEC_NOT_FOUND -8961
#define ICM_PARAM_ERR         -50

typedef uint32_t CodecType;
typedef int32_t CodecQ;
typedef int32_t Fixed;

#define CODEC_RAW   0x72617720 // 'raw '
#define CODEC_RLE   0x726C6520 // 'rle '
#define CODEC_RPZA  0x72707A61 // 'rpza' (Apple Video)
#define CODEC_JPEG  0x6A706567 // 'jpeg'

typedef struct ImageDescription {
    uint32_t idSize;
    CodecType cType;
    uint32_t resvd1;
    uint16_t resvd2;
    uint16_t dataRefIndex;
    uint16_t version;
    uint16_t revisionLevel;
    uint32_t vendor;
    CodecQ temporalQuality;
    CodecQ spatialQuality;
    int16_t width;
    int16_t height;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t dataSize;
    uint16_t frameCount;
    char name[32];
    int16_t depth;
    int16_t clutID;
} ImageDescription, *ImageDescriptionPtr, **ImageDescriptionHandle;

/* ICM Core API */
int16_t lc3_decompress_image(const void *data, ImageDescriptionHandle desc, void *dstPixMap,
                             const void *srcRect, const void *dstRect, int16_t mode, void *maskRgn);
int16_t lc3_compress_image(void *srcPixMap, const void *srcRect, CodecQ quality,
                           CodecType cType, ImageDescriptionHandle desc, void *data);
int16_t lc3_get_similarity(void *srcPixMap, const void *srcRect, ImageDescriptionHandle desc,
                           const void *data, Fixed *similarity);
int16_t lc3_get_compressed_image_size(ImageDescriptionHandle desc, const void *data,
                                      int32_t bufferSize, int32_t *dataSize);
int16_t lc3_get_max_compression_size(void *srcPixMap, const void *srcRect, int16_t depth,
                                     CodecQ quality, CodecType cType, int32_t *size);
int16_t lc3_trim_image(ImageDescriptionHandle desc, const void *inData, int32_t inFlags,
                       void *outData, int32_t *outFlags, const void *trimRect);

#endif /* LC3_ICM_ENGINE_H */
