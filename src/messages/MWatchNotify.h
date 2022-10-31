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


#ifndef STONE_MWATCHNOTIFY_H
#define STONE_MWATCHNOTIFY_H

#include "msg/Message.h"


class MWatchNotify final : public Message {
private:
  static constexpr int HEAD_VERSION = 3;
  static constexpr int COMPAT_VERSION = 1;

 public:
  uint64_t cookie;     ///< client unique id for this watch or notify
  uint64_t ver;        ///< unused
  uint64_t notify_id;  ///< osd unique id for a notify notification
  uint8_t opcode;      ///< STONE_WATCH_EVENT_*
  stone::buffer::list bl;       ///< notify payload (osd->client)
  errorcode32_t return_code; ///< notify result (osd->client)
  uint64_t notifier_gid; ///< who sent the notify

  MWatchNotify()
    : Message{STONE_MSG_WATCH_NOTIFY, HEAD_VERSION, COMPAT_VERSION} { }
  MWatchNotify(uint64_t c, uint64_t v, uint64_t i, uint8_t o, stone::buffer::list b, uint64_t n=0)
    : Message{STONE_MSG_WATCH_NOTIFY, HEAD_VERSION, COMPAT_VERSION},
      cookie(c),
      ver(v),
      notify_id(i),
      opcode(o),
      bl(b),
      return_code(0),
      notifier_gid(n) { }
private:
  ~MWatchNotify() final {}

public:
  void decode_payload() override {
    using stone::decode;
    uint8_t msg_ver;
    auto p = payload.cbegin();
    decode(msg_ver, p);
    decode(opcode, p);
    decode(cookie, p);
    decode(ver, p);
    decode(notify_id, p);
    if (msg_ver >= 1)
      decode(bl, p);
    if (header.version >= 2)
      decode(return_code, p);
    else
      return_code = 0;
    if (header.version >= 3)
      decode(notifier_gid, p);
    else
      notifier_gid = 0;
  }
  void encode_payload(uint64_t features) override {
    using stone::encode;
    uint8_t msg_ver = 1;
    encode(msg_ver, payload);
    encode(opcode, payload);
    encode(cookie, payload);
    encode(ver, payload);
    encode(notify_id, payload);
    encode(bl, payload);
    encode(return_code, payload);
    encode(notifier_gid, payload);
  }

  std::string_view get_type_name() const override { return "watch-notify"; }
  void print(std::ostream& out) const override {
    out << "watch-notify("
	<< stone_watch_event_name(opcode) << " (" << (int)opcode << ")"
	<< " cookie " << cookie
	<< " notify " << notify_id
	<< " ret " << return_code
	<< ")";
  }
private:
  template<class T, typename... Args>
  friend boost::intrusive_ptr<T> stone::make_message(Args&&... args);
};

#endif
