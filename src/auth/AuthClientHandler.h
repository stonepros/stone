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

#ifndef STONE_AUTHCLIENTHANDLER_H
#define STONE_AUTHCLIENTHANDLER_H


#include "auth/Auth.h"
#include "include/common_fwd.h"

struct MAuthReply;
class RotatingKeyRing;

class AuthClientHandler {
protected:
  StoneContext *cct;
  EntityName name;
  uint64_t global_id;
  uint32_t want;
  uint32_t have;
  uint32_t need;

public:
  explicit AuthClientHandler(StoneContext *cct_)
    : cct(cct_), global_id(0), want(STONE_ENTITY_TYPE_AUTH), have(0), need(0)
  {}
  virtual ~AuthClientHandler() {}

  virtual AuthClientHandler* clone() const = 0;

  void init(const EntityName& n) { name = n; }
  
  void set_want_keys(__u32 keys) {
    want = keys | STONE_ENTITY_TYPE_AUTH;
    validate_tickets();
  }

  virtual int get_protocol() const = 0;

  virtual void reset() = 0;
  virtual void prepare_build_request() = 0;
  virtual void build_initial_request(stone::buffer::list *bl) const {
    // this is empty for methods stonex and none.
  }
  virtual int build_request(stone::buffer::list& bl) const = 0;
  virtual int handle_response(int ret, stone::buffer::list::const_iterator& iter,
			      CryptoKey *session_key,
			      std::string *connection_secret) = 0;
  virtual bool build_rotating_request(stone::buffer::list& bl) const = 0;

  virtual AuthAuthorizer *build_authorizer(uint32_t service_id) const = 0;

  virtual bool need_tickets() = 0;

  virtual void set_global_id(uint64_t id) = 0;

  static AuthClientHandler* create(StoneContext* cct, int proto, RotatingKeyRing* rkeys);
protected:
  virtual void validate_tickets() = 0;
};

#endif

