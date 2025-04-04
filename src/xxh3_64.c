#include <stdio.h>
#include <stdint.h>
#include <defs.h>

#define XXH3_PRIME64_1 0x9E3779B185EBCA87ULL
#define XXH3_PRIME64_2 0xC2B2AE3D27D4EB4FULL
#define XXH3_PRIME64_3 0x165667B19E3779F9ULL

uint64 xxh3_64(const void *data) {
    size_t len = strlen(data);

    const uint8_t *p = (const uint8_t *)data;
    uint64 hash = XXH3_PRIME64_1 + len * XXH3_PRIME64_2;
    size_t i;

    for (i = 0; i + 8 <= len; i += 8) {
        uint64 k1 = *(uint64 *)(p + i) * XXH3_PRIME64_2;
        k1 = (k1 << 31) | (k1 >> (64 - 31));
        k1 *= XXH3_PRIME64_1;
        hash ^= k1;
        hash = (hash << 27) | (hash >> (64 - 27));
        hash *= XXH3_PRIME64_3;
    }

    // Process remaining bytes
    uint64 tail = 0;
    switch (len & 7) {
        #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
        case 7: tail ^= (uint64)p[i + 6] << 48;
        case 6: tail ^= (uint64)p[i + 5] << 40;
        case 5: tail ^= (uint64)p[i + 4] << 32;
        case 4: tail ^= (uint64)p[i + 3] << 24;
        case 3: tail ^= (uint64)p[i + 2] << 16;
        case 2: tail ^= (uint64)p[i + 1] << 8;
        case 1: tail ^= (uint64)p[i];
                tail *= XXH3_PRIME64_2;
                tail = (tail << 31) | (tail >> (64 - 31));
                tail *= XXH3_PRIME64_1;
                hash ^= tail;
    }

    // Final mixing
    hash ^= hash >> 33;
    hash *= XXH3_PRIME64_2;
    hash ^= hash >> 29;
    hash *= XXH3_PRIME64_3;
    hash ^= hash >> 32;

    return hash;
}
