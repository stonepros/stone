// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "arch/probe.h"

#include "arch/intel.h"
#include "arch/arm.h"
#include "arch/ppc.h"

int stone_arch_probe(void)
{
  if (stone_arch_probed)
    return 1;
#if defined(__i386__) || defined(__x86_64__)
  stone_arch_intel_probe();
#elif defined(__arm__) || defined(__aarch64__)
  stone_arch_arm_probe();
#elif defined(__powerpc__) || defined(__ppc__)
  stone_arch_ppc_probe();
#endif
  stone_arch_probed = 1;
  return 1;
}

// do this once using the magic of c++.
int stone_arch_probed = stone_arch_probe();
