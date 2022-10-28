/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2016 Mirantis, Inc.
 *
 * Author: Adam Kupczyk <akupczykd@mirantis.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 */


// -----------------------------------------------------------------------------
#include "crypto/isa-l/isal_crypto_plugin.h"

#include "stone_ver.h"
// -----------------------------------------------------------------------------

const char *__stone_plugin_version()
{
  return STONE_GIT_NICE_VER;
}

int __stone_plugin_init(StoneContext *cct,
                       const std::string& type,
                       const std::string& name)
{
  auto instance = cct->get_plugin_registry();

  return instance->add(type, name, new ISALCryptoPlugin(cct));
}
