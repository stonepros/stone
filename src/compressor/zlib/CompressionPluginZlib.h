/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2015 Mirantis, Inc.
 *
 * Author: Alyona Kiseleva <akiselyova@mirantis.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 */

#ifndef STONE_COMPRESSION_PLUGIN_ZLIB_H
#define STONE_COMPRESSION_PLUGIN_ZLIB_H

// -----------------------------------------------------------------------------
#include "arch/probe.h"
#include "arch/intel.h"
#include "arch/arm.h"
#include "common/stone_context.h"
#include "compressor/CompressionPlugin.h"
#include "ZlibCompressor.h"

// -----------------------------------------------------------------------------

class CompressionPluginZlib : public stone::CompressionPlugin {
public:
  bool has_isal = false;

  explicit CompressionPluginZlib(StoneContext *cct) : CompressionPlugin(cct)
  {}

  int factory(CompressorRef *cs,
                      std::ostream *ss) override
  {
    bool isal = false;
#if defined(__i386__) || defined(__x86_64__)
    // other arches or lack of support result in isal = false
    if (cct->_conf->compressor_zlib_isal) {
      stone_arch_probe();
      isal = (stone_arch_intel_pclmul && stone_arch_intel_sse41);
    }
#endif
    if (compressor == 0 || has_isal != isal) {
      compressor = std::make_shared<ZlibCompressor>(cct, isal);
      has_isal = isal;
    }
    *cs = compressor;
    return 0;
  }
};

#endif
