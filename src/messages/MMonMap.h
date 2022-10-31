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

#ifndef STONE_MMONMAP_H
#define STONE_MMONMAP_H

#include "include/encoding.h"
#include "include/stone_features.h"
#include "msg/Message.h"
#include "msg/MessageRef.h"
#include "mon/MonMap.h"

class MMonMap final : public Message {
public:
  stone::buffer::list monmapbl;

  MMonMap() : Message{STONE_MSG_MON_MAP} { }
  explicit MMonMap(stone::buffer::list &bl) : Message{STONE_MSG_MON_MAP} {
    monmapbl = std::move(bl);
  }
private:
  ~MMonMap() final {}

public:
  std::string_view get_type_name() const override { return "mon_map"; }

  void encode_payload(uint64_t features) override { 
    if (monmapbl.length() &&
	((features & STONE_FEATURE_MONENC) == 0 ||
	 (features & STONE_FEATURE_MSG_ADDR2) == 0)) {
      // reencode old-format monmap
      MonMap t;
      t.decode(monmapbl);
      monmapbl.clear();
      t.encode(monmapbl, features);
    }

    using stone::encode;
    encode(monmapbl, payload);
  }
  void decode_payload() override { 
    using stone::decode;
    auto p = payload.cbegin();
    decode(monmapbl, p);
  }
private:
  template<class T, typename... Args>
  friend boost::intrusive_ptr<T> stone::make_message(Args&&... args);
};

#endif
