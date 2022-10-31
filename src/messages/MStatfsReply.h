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


#ifndef STONE_MSTATFSREPLY_H
#define STONE_MSTATFSREPLY_H

class MStatfsReply : public Message {
public:
  struct stone_mon_statfs_reply h{};

  MStatfsReply() : Message{STONE_MSG_STATFS_REPLY} {}
  MStatfsReply(uuid_d &f, stone_tid_t t, epoch_t epoch)
    : Message{STONE_MSG_STATFS_REPLY} {
    memcpy(&h.fsid, f.bytes(), sizeof(h.fsid));
    header.tid = t;
    h.version = epoch;
  }

  std::string_view get_type_name() const override { return "statfs_reply"; }
  void print(std::ostream& out) const override {
    out << "statfs_reply(" << header.tid << ")";
  }

  void encode_payload(uint64_t features) override {
    using stone::encode;
    encode(h, payload);
  }
  void decode_payload() override {
    auto p = payload.cbegin();
    decode(h, p);
  }
private:
  template<class T, typename... Args>
  friend boost::intrusive_ptr<T> stone::make_message(Args&&... args);
};

#endif
