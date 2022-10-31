// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string_view>

#include "common/stone_strings.h"

// the C++ version of STONE_RELEASE_* defined by include/rados.h
enum class stone_release_t : std::uint8_t {
  unknown = 0,
  argonaut,
  bobtail,
  cuttlefish,
  dumpling,
  emperor,
  firefly,
  giant,
  hammer,
  infernalis,
  jewel,
  kraken,
  luminous,
  mimic,
  nautilus,
  octopus,
  pacific,
  max,
};

std::ostream& operator<<(std::ostream& os, const stone_release_t r);

inline bool operator!(stone_release_t& r) {
  return (r < stone_release_t::unknown ||
          r == stone_release_t::unknown);
}

inline stone_release_t& operator--(stone_release_t& r) {
  r = static_cast<stone_release_t>(static_cast<uint8_t>(r) - 1);
  return r;
}

inline stone_release_t& operator++(stone_release_t& r) {
  r = static_cast<stone_release_t>(static_cast<uint8_t>(r) + 1);
  return r;
}

inline bool operator<(stone_release_t lhs, stone_release_t rhs) {
  // we used to use -1 for invalid release
  if (static_cast<int8_t>(lhs) < 0) {
    return true;
  } else if (static_cast<int8_t>(rhs) < 0) {
    return false;
  }
  return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs);
}

inline bool operator>(stone_release_t lhs, stone_release_t rhs) {
  // we used to use -1 for invalid release
  if (static_cast<int8_t>(lhs) < 0) {
    return false;
  } else if (static_cast<int8_t>(rhs) < 0) {
    return true;
  }
  return static_cast<uint8_t>(lhs) > static_cast<uint8_t>(rhs);
}

inline bool operator>=(stone_release_t lhs, stone_release_t rhs) {
  return !(lhs < rhs);
}

bool can_upgrade_from(stone_release_t from_release,
		      std::string_view from_release_name,
		      std::ostream& err);

stone_release_t stone_release_from_name(std::string_view sv);
stone_release_t stone_release();

inline std::string_view to_string(stone_release_t r) {
  return stone_release_name(static_cast<int>(r));
}
template<typename IntType> IntType to_integer(stone_release_t r) {
  return static_cast<IntType>(r);
}
