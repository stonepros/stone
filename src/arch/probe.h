#ifndef STONE_ARCH_PROBE_H
#define STONE_ARCH_PROBE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int stone_arch_probed;  /* non-zero if we've probed features */

extern int stone_arch_probe(void);

#ifdef __cplusplus
}
#endif

#endif
