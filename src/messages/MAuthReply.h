// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef STONE_MAUTHREPLY_H
#define STONE_MAUTHREPLY_H

#include "msg/Message.h"
#include "common/errno.h"

class MAuthReply final : public Message {
public:
  __u32 protocol;
  errorcode32_t result;
  uint64_t global_id;      // if zero, meaningless
  std::string result_msg;
  stone::buffer::list result_bl;

  MAuthReply() : Message(STONE_MSG_AUTH_REPLY), protocol(0), result(0), global_id(0) {}
  MAuthReply(__u32 p, stone::buffer::list *bl = NULL, int r = 0, uint64_t gid=0, const char *msg = "") :
    Message(STONE_MSG_AUTH_REPLY),
    protocol(p), result(r), global_id(gid),
    result_msg(msg) {
    if (bl)
      result_bl = *bl;
  }
private:
  ~MAuthReply() final {}

public:
  std::string_view get_type_name() const override { return "auth_reply"; }
  void print(std::ostream& o) const override {
    o << "auth_reply(proto " << protocol << " " << result << " " << cpp_strerror(result);
    if (result_msg.length())
      o << ": " << result_msg;
    o << ")";
  }

  void decode_payload() override {
    using stone::decode;
    auto p = payload.cbegin();
    decode(protocol, p);
    decode(result, p);
    decode(global_id, p);
    decode(result_bl, p);
    decode(result_msg, p);
  }
  void encode_payload(uint64_t features) override {
    using stone::encode;
    encode(protocol, payload);
    encode(result, payload);
    encode(global_id, payload);
    encode(result_bl, payload);
    encode(result_msg, payload);
  }
};

#endif
