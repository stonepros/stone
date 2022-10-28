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


#include <errno.h>

#include "AuthClientHandler.h"
#include "stonex/StonexClientHandler.h"
#ifdef HAVE_GSSAPI
#include "krb/KrbClientHandler.hpp"
#endif
#include "none/AuthNoneClientHandler.h"


AuthClientHandler*
AuthClientHandler::create(StoneContext* cct, int proto,
			  RotatingKeyRing* rkeys)
{
  switch (proto) {
  case STONE_AUTH_STONEX:
    return new StonexClientHandler(cct, rkeys);
  case STONE_AUTH_NONE:
    return new AuthNoneClientHandler{cct};
#ifdef HAVE_GSSAPI
  case STONE_AUTH_GSS: 
    return new KrbClientHandler(cct);
#endif
  default:
    return NULL;
  }
}
