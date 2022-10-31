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


#include "auth/AuthSessionHandler.h"
#include "auth/Auth.h"
#include "include/common_fwd.h"

class Message;

class StonexSessionHandler  : public AuthSessionHandler {
  StoneContext *cct;
  int protocol;
  CryptoKey key;                // per mon authentication
  uint64_t features;

  int _calc_signature(Message *m, uint64_t *psig);

public:
  StonexSessionHandler(StoneContext *cct,
		      const CryptoKey& session_key,
		      const uint64_t features)
    : cct(cct),
      protocol(STONE_AUTH_STONEX),
      key(session_key),
      features(features) {
  }
  ~StonexSessionHandler() override = default;

  int sign_message(Message *m) override;
  int check_message_signature(Message *m) override ;
};

