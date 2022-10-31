// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#ifndef STONE_CLS_LOG_TYPES_H
#define STONE_CLS_LOG_TYPES_H

#include "include/encoding.h"
#include "include/types.h"

#include "include/utime.h"

class JSONObj;


struct cls_log_entry {
  std::string id;
  std::string section;
  std::string name;
  utime_t timestamp;
  stone::buffer::list data;

  cls_log_entry() {}

  void encode(stone::buffer::list& bl) const {
    ENCODE_START(2, 1, bl);
    encode(section, bl);
    encode(name, bl);
    encode(timestamp, bl);
    encode(data, bl);
    encode(id, bl);
    ENCODE_FINISH(bl);
  }

  void decode(stone::buffer::list::const_iterator& bl) {
    DECODE_START(2, bl);
    decode(section, bl);
    decode(name, bl);
    decode(timestamp, bl);
    decode(data, bl);
    if (struct_v >= 2)
      decode(id, bl);
    DECODE_FINISH(bl);
  }
};
WRITE_CLASS_ENCODER(cls_log_entry)

struct cls_log_header {
  std::string max_marker;
  utime_t max_time;

  void encode(stone::buffer::list& bl) const {
    ENCODE_START(1, 1, bl);
    encode(max_marker, bl);
    encode(max_time, bl);
    ENCODE_FINISH(bl);
  }

  void decode(stone::buffer::list::const_iterator& bl) {
    DECODE_START(1, bl);
    decode(max_marker, bl);
    decode(max_time, bl);
    DECODE_FINISH(bl);
  }
};
inline bool operator ==(const cls_log_header& lhs, const cls_log_header& rhs) {
  return (lhs.max_marker == rhs.max_marker &&
	  lhs.max_time == rhs.max_time);
}
inline bool operator !=(const cls_log_header& lhs, const cls_log_header& rhs) {
  return !(lhs == rhs);
}
WRITE_CLASS_ENCODER(cls_log_header)


#endif
