/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2017 BI SHUN KE <aionshun@livemail.tw>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_COMPRESSION_PLUGIN_BROTLI_H
#define STONE_COMPRESSION_PLUGIN_BROTLI_H

#include "stone_ver.h"
#include "compressor/CompressionPlugin.h"
#include "BrotliCompressor.h"

class CompressionPluginBrotli : public CompressionPlugin {
public:
  explicit CompressionPluginBrotli(StoneContext *cct) : CompressionPlugin(cct)
  {}
  
  virtual int factory(CompressorRef *cs, std::ostream *ss)
  {
    if (compressor == nullptr) {
      BrotliCompressor *interface = new BrotliCompressor();
      compressor = CompressorRef(interface);
    }
    *cs = compressor;
    return 0;
  }
};

#endif
