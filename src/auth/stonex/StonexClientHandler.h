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

#ifndef STONE_STONEXCLIENTHANDLER_H
#define STONE_STONEXCLIENTHANDLER_H

#include "auth/AuthClientHandler.h"
#include "StonexProtocol.h"
#include "auth/RotatingKeyRing.h"
#include "include/common_fwd.h"

class KeyRing;

class StonexClientHandler : public AuthClientHandler {
  bool starting;

  /* envelope protocol parameters */
  uint64_t server_challenge;

  StoneXTicketManager tickets;
  StoneXTicketHandler* ticket_handler;

  RotatingKeyRing* rotating_secrets;
  KeyRing *keyring;

public:
  StonexClientHandler(StoneContext *cct_,
		     RotatingKeyRing *rsecrets)
    : AuthClientHandler(cct_),
      starting(false),
      server_challenge(0),
      tickets(cct_),
      ticket_handler(NULL),
      rotating_secrets(rsecrets),
      keyring(rsecrets->get_keyring())
  {
    reset();
  }

  StonexClientHandler* clone() const override {
    return new StonexClientHandler(*this);
  }

  void reset() override;
  void prepare_build_request() override;
  int build_request(ceph::buffer::list& bl) const override;
  int handle_response(int ret, ceph::buffer::list::const_iterator& iter,
		      CryptoKey *session_key,
		      std::string *connection_secret) override;
  bool build_rotating_request(ceph::buffer::list& bl) const override;

  int get_protocol() const override { return STONE_AUTH_STONEX; }

  AuthAuthorizer *build_authorizer(uint32_t service_id) const override;

  bool need_tickets() override;

  void set_global_id(uint64_t id) override {
    global_id = id;
    tickets.global_id = id;
  }
private:
  void validate_tickets() override;
  bool _need_tickets() const;
};

#endif
