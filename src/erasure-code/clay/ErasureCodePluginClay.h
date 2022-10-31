// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone distributed storage system
 *
 * Copyright (C) 2018 Indian Institute of Science <office.ece@iisc.ac.in>
 *
 * Author: Myna Vajha <mynaramana@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * 
 */

#ifndef STONE_ERASURE_CODE_PLUGIN_CLAY_H
#define STONE_ERASURE_CODE_PLUGIN_CLAY_H

#include "erasure-code/ErasureCodePlugin.h"

class ErasureCodePluginClay : public stone::ErasureCodePlugin {
public:
  int factory(const std::string& directory,
	      stone::ErasureCodeProfile &profile,
	      stone::ErasureCodeInterfaceRef *erasure_code,
	      std::ostream *ss) override;
};

#endif
