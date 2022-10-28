// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <array>
#include "stonefs_features.h"
#include "mdstypes.h"

static const std::array feature_names
{
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "jewel",
  "kraken",
  "luminous",
  "mimic",
  "reply_encoding",
  "reclaim_client",
  "lazy_caps_wanted",
  "multi_reconnect",
  "deleg_ino",
  "metric_collect",
  "alternate_name",
};
static_assert(feature_names.size() == STONEFS_FEATURE_MAX + 1);

std::string_view stonefs_feature_name(size_t id)
{
  if (id > feature_names.size())
    return "unknown"sv;
  return feature_names[id];
}

int stonefs_feature_from_name(std::string_view name)
{
  if (name == "reserved"sv) {
    return -1;
  }
  for (size_t i = 0; i < feature_names.size(); ++i) {
    if (name == feature_names[i])
      return i;
  }
  return -1;
}

std::string stonefs_stringify_features(const feature_bitset_t& features)
{
  CachedStackStringStream css;
  bool first = true;
  *css << "{";
  for (size_t i = 0; i < feature_names.size(); ++i) {
    if (!features.test(i))
      continue;
    if (!first)
      *css << ",";
    *css << i << "=" << stonefs_feature_name(i);
    first = false;
  }
  *css << "}";
  return css->str();
}

void stonefs_dump_features(stone::Formatter *f, const feature_bitset_t& features)
{
  for (size_t i = 0; i < feature_names.size(); ++i) {
    if (!features.test(i))
      continue;
    char s[18];
    snprintf(s, sizeof(s), "feature_%lu", i);
    f->dump_string(s, stonefs_feature_name(i));
  }
}

