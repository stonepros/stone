// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2012 Inktank Storage, Inc.
 * Copyright (C) 2014 Red Hat <contact@redhat.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 */
#ifndef STONE_UTIL_H
#define STONE_UTIL_H

#include "common/Formatter.h"
#include "include/types.h"

std::string bytes2str(uint64_t count);

struct stone_data_stats
{
  uint64_t byte_total;
  uint64_t byte_used;
  uint64_t byte_avail;
  int avail_percent;

  stone_data_stats() :
    byte_total(0),
    byte_used(0),
    byte_avail(0),
    avail_percent(0)
  { }

  void dump(stone::Formatter *f) const {
    stone_assert(f != NULL);
    f->dump_int("total", byte_total);
    f->dump_int("used", byte_used);
    f->dump_int("avail", byte_avail);
    f->dump_int("avail_percent", avail_percent);
  }

  void encode(stone::buffer::list &bl) const {
    ENCODE_START(1, 1, bl);
    encode(byte_total, bl);
    encode(byte_used, bl);
    encode(byte_avail, bl);
    encode(avail_percent, bl);
    ENCODE_FINISH(bl);
  }

  void decode(stone::buffer::list::const_iterator &p) {
    DECODE_START(1, p);
    decode(byte_total, p);
    decode(byte_used, p);
    decode(byte_avail, p);
    decode(avail_percent, p);
    DECODE_FINISH(p);
  }

  static void generate_test_instances(std::list<stone_data_stats*>& ls) {
    ls.push_back(new stone_data_stats);
    ls.push_back(new stone_data_stats);
    ls.back()->byte_total = 1024*1024;
    ls.back()->byte_used = 512*1024;
    ls.back()->byte_avail = 512*1024;
    ls.back()->avail_percent = 50;
  }
};
typedef struct stone_data_stats stone_data_stats_t;
WRITE_CLASS_ENCODER(stone_data_stats)

int get_fs_stats(stone_data_stats_t &stats, const char *path);

/// get memory limit for the current cgroup
int get_cgroup_memory_limit(uint64_t *limit);

/// collect info from @p uname(2), @p /proc/meminfo and @p /proc/cpuinfo
void collect_sys_info(std::map<std::string, std::string> *m, StoneContext *cct);

#ifdef _WIN32
/// Retrieve the actual Windows version, regardless of the app manifest.
int get_windows_version(POSVERSIONINFOEXW ver);
#endif

/// dump service ids grouped by their host to the specified formatter
/// @param f formatter for the output
/// @param services a map from hostname to a list of service id hosted by this host
/// @param type the service type of given @p services, for example @p osd or @p mon.
void dump_services(stone::Formatter* f,
		   const std::map<std::string, std::list<int> >& services,
		   const char* type);
/// dump service names grouped by their host to the specified formatter
/// @param f formatter for the output
/// @param services a map from hostname to a list of service name hosted by this host
/// @param type the service type of given @p services, for example @p osd or @p mon.
void dump_services(stone::Formatter* f, const std::map<std::string,
		   std::list<std::string> >& services, const char* type);

std::string cleanbin(stone::buffer::list &bl, bool &b64, bool show = false);
std::string cleanbin(std::string &str);

namespace stone::util {

// Returns true if s matches any parameters:
template <typename ...XS>
bool match_str(const std::string& s, const XS& ...xs)
{
 return ((s == xs) || ...);
}

} // namespace stone::util
#endif /* STONE_UTIL_H */
