// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

#ifndef STONE_RGW_USAGE_H
#define STONE_RGW_USAGE_H

#include <string>
#include <map>

#include "common/Formatter.h"
#include "common/dout.h"
#include "rgw_formats.h"
#include "rgw_user.h"

class RGWRados;


class RGWUsage
{
public:
  static int show(const DoutPrefixProvider *dpp, RGWRados *store, const rgw_user& uid, const string& bucket_name, uint64_t start_epoch,
	          uint64_t end_epoch, bool show_log_entries, bool show_log_sum,
		  std::map<std::string, bool> *categories, RGWFormatterFlusher& flusher);

  static int trim(const DoutPrefixProvider *dpp, RGWRados *store, const rgw_user& uid, const string& bucket_name, uint64_t start_epoch,
	          uint64_t end_epoch);

  static int clear(const DoutPrefixProvider *dpp, RGWRados *store);
};


#endif
