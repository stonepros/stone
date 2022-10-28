// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
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

#include "stone_ver.h"
#include "common/debug.h"
#include "ErasureCodePluginLrc.h"
#include "ErasureCodeLrc.h"

#define dout_subsys stone_subsys_osd
#undef dout_prefix
#define dout_prefix _prefix(_dout)

int ErasureCodePluginLrc::factory(const std::string &directory,
				  stone::ErasureCodeProfile &profile,
				  stone::ErasureCodeInterfaceRef *erasure_code,
				  std::ostream *ss) {
    ErasureCodeLrc *interface;
    interface = new ErasureCodeLrc(directory);
    int r = interface->init(profile, ss);
    if (r) {
      delete interface;
      return r;
    }
    *erasure_code = stone::ErasureCodeInterfaceRef(interface);
    return 0;
};

const char *__erasure_code_version() { return STONE_GIT_NICE_VER; }

int __erasure_code_init(char *plugin_name, char *directory)
{
  auto& instance = stone::ErasureCodePluginRegistry::instance();
  return instance.add(plugin_name, new ErasureCodePluginLrc());
}
