// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2004-2009 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#include "common/debug.h"
#include "AuthSessionHandler.h"
#include "stonex/StonexSessionHandler.h"
#ifdef HAVE_GSSAPI
#include "krb/KrbSessionHandler.hpp"
#endif
#include "none/AuthNoneSessionHandler.h"

#include "common/stone_crypto.h"
#define dout_subsys stone_subsys_auth


AuthSessionHandler *get_auth_session_handler(
  StoneContext *cct, int protocol,
  const CryptoKey& key,
  uint64_t features)
{

  // Should add code to only print the SHA1 hash of the key, unless in secure debugging mode
#ifndef WITH_SEASTAR
  ldout(cct,10) << "In get_auth_session_handler for protocol " << protocol << dendl;
#endif
  switch (protocol) {
  case STONE_AUTH_STONEX:
    // if there is no session key, there is no session handler.
    if (key.get_type() == STONE_CRYPTO_NONE) {
      return nullptr;
    }
    return new StonexSessionHandler(cct, key, features);
  case STONE_AUTH_NONE:
    return new AuthNoneSessionHandler();
#ifdef HAVE_GSSAPI
  case STONE_AUTH_GSS: 
    return new KrbSessionHandler();
#endif
  default:
    return nullptr;
  }
}

