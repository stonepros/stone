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

#ifndef STONE_COMPRESSION_PLUGIN_ZSTD_H
#define STONE_COMPRESSION_PLUGIN_ZSTD_H

// -----------------------------------------------------------------------------
#include "stone_ver.h"
#include "compressor/CompressionPlugin.h"
#include "ZstdCompressor.h"
// -----------------------------------------------------------------------------

class CompressionPluginZstd : public stone::CompressionPlugin {

public:

  explicit CompressionPluginZstd(StoneContext* cct) : CompressionPlugin(cct)
  {}

  int factory(CompressorRef *cs,
                      std::ostream *ss) override
  {
    if (compressor == 0) {
      ZstdCompressor *interface = new ZstdCompressor(cct);
      compressor = CompressorRef(interface);
    }
    *cs = compressor;
    return 0;
  }
};

#endif
