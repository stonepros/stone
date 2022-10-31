// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

#ifndef STONE_RGW_RESOLVE_H
#define STONE_RGW_RESOLVE_H

#include "rgw_common.h"

namespace stone {
  class DNSResolver;
}

class RGWResolver {
  DNSResolver *resolver;

public:
  ~RGWResolver();
  RGWResolver();
  int resolve_cname(const string& hostname, string& cname, bool *found);
};


extern void rgw_init_resolver(void);
extern void rgw_shutdown_resolver(void);
extern RGWResolver *rgw_resolver;

#endif
