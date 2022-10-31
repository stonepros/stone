// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone distributed storage system
 *
 * Copyright (C) 2014 Cloudwatt <libre.licensing@cloudwatt.com>
 * Copyright (C) 2014 Red Hat <contact@redhat.com>
 *
 * Author: Loic Dachary <loic@dachary.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * 
 */

#ifndef STONE_ERASURE_CODE_PLUGIN_ISA_H
#define STONE_ERASURE_CODE_PLUGIN_ISA_H

#include "erasure-code/ErasureCodePlugin.h"
#include "ErasureCodeIsaTableCache.h"

class ErasureCodePluginIsa : public stone::ErasureCodePlugin {
public:
  ErasureCodeIsaTableCache tcache;

  int factory(const std::string &directory,
	      stone::ErasureCodeProfile &profile,
	      stone::ErasureCodeInterfaceRef *erasure_code,
	      std::ostream *ss) override;
};

#endif
