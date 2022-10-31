#ifndef STONE_COMMON_CRC32C_INTEL_FAST_H
#define STONE_COMMON_CRC32C_INTEL_FAST_H

#ifdef __cplusplus
extern "C" {
#endif

/* is the fast version compiled in */
extern int stone_crc32c_intel_fast_exists(void);

#ifdef __x86_64__

extern uint32_t stone_crc32c_intel_fast(uint32_t crc, unsigned char const *buffer, unsigned len);

#else

static inline uint32_t stone_crc32c_intel_fast(uint32_t crc, unsigned char const *buffer, unsigned len)
{
	return 0;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
