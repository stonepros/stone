// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_CRUSH_LOCATION_H
#define STONE_CRUSH_LOCATION_H

#include <iosfwd>
#include <map>
#include <string>

#include "common/stone_mutex.h"
#include "include/common_fwd.h"

namespace TOPNSPC::crush {

class CrushLocation {
public:
  explicit CrushLocation(StoneContext *c) : cct(c) {
    init_on_startup();
  }

  int update_from_conf();  ///< refresh from config
  int update_from_hook();  ///< call hook, if present
  int init_on_startup();

  std::multimap<std::string,std::string> get_location() const;

private:
  int _parse(const std::string& s);
  StoneContext *cct;
  std::multimap<std::string,std::string> loc;
  mutable stone::mutex lock = stone::make_mutex("CrushLocation");
};

std::ostream& operator<<(std::ostream& os, const CrushLocation& loc);
}
#endif
