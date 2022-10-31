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


#ifndef STONE_MCLIENTLEASE_H
#define STONE_MCLIENTLEASE_H

#include <string_view>

#include "msg/Message.h"

class MClientLease final : public SafeMessage {
public:
  struct stone_mds_lease h;
  std::string dname;
  
  int get_action() const { return h.action; }
  stone_seq_t get_seq() const { return h.seq; }
  int get_mask() const { return h.mask; }
  inodeno_t get_ino() const { return inodeno_t(h.ino); }
  snapid_t get_first() const { return snapid_t(h.first); }
  snapid_t get_last() const { return snapid_t(h.last); }

protected:
  MClientLease() : SafeMessage(STONE_MSG_CLIENT_LEASE) {}
  MClientLease(const MClientLease& m) :
    SafeMessage(STONE_MSG_CLIENT_LEASE),
    h(m.h),
    dname(m.dname) {}
  MClientLease(int ac, stone_seq_t seq, int m, uint64_t i, uint64_t sf, uint64_t sl) :
    SafeMessage(STONE_MSG_CLIENT_LEASE) {
    h.action = ac;
    h.seq = seq;
    h.mask = m;
    h.ino = i;
    h.first = sf;
    h.last = sl;
    h.duration_ms = 0;
  }
  MClientLease(int ac, stone_seq_t seq, int m, uint64_t i, uint64_t sf, uint64_t sl, std::string_view d) :
    SafeMessage(STONE_MSG_CLIENT_LEASE),
    dname(d) {
    h.action = ac;
    h.seq = seq;
    h.mask = m;
    h.ino = i;
    h.first = sf;
    h.last = sl;
    h.duration_ms = 0;
  }
  ~MClientLease() final {}

public:
  std::string_view get_type_name() const override { return "client_lease"; }
  void print(std::ostream& out) const override {
    out << "client_lease(a=" << stone_lease_op_name(get_action())
	<< " seq " << get_seq()
	<< " mask " << get_mask();
    out << " " << get_ino();
    if (h.last != STONE_NOSNAP)
      out << " [" << snapid_t(h.first) << "," << snapid_t(h.last) << "]";
    if (dname.length())
      out << "/" << dname;
    out << ")";
  }

  void decode_payload() override {
    using stone::decode;
    auto p = payload.cbegin();
    decode(h, p);
    decode(dname, p);
  }
  void encode_payload(uint64_t features) override {
    using stone::encode;
    encode(h, payload);
    encode(dname, payload);
  }

private:
  template<class T, typename... Args>
  friend boost::intrusive_ptr<T> stone::make_message(Args&&... args);
};

#endif
