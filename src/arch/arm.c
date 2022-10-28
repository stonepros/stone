#include "acconfig.h"
#include "arch/probe.h"

/* flags we export */
int stone_arch_neon = 0;
int stone_arch_aarch64_crc32 = 0;
int stone_arch_aarch64_pmull = 0;

#include <stdio.h>

#if __linux__

#include <elf.h>
#include <link.h> // ElfW macro
#include <sys/auxv.h>

#if __arm__ || __aarch64__
#include <asm/hwcap.h>
#endif // __arm__

#endif // __linux__

int stone_arch_arm_probe(void)
{
#if __linux__
	unsigned long hwcap = getauxval(AT_HWCAP);
#if __arm__
	stone_arch_neon = (hwcap & HWCAP_NEON) == HWCAP_NEON;
#elif __aarch64__
	stone_arch_neon = (hwcap & HWCAP_ASIMD) == HWCAP_ASIMD;
	stone_arch_aarch64_crc32 = (hwcap & HWCAP_CRC32) == HWCAP_CRC32;
	stone_arch_aarch64_pmull = (hwcap & HWCAP_PMULL) == HWCAP_PMULL;
#endif
#endif // __linux__
	return 0;
}

