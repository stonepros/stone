// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *

 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONEFS_FEATURES_H
#define STONEFS_FEATURES_H

#include "include/cephfs/metrics/Types.h"

class feature_bitset_t;
namespace ceph {
  class Formatter;
}

// When adding a new release, please update the "current" release below, add a
// feature bit for that release, add that feature bit to STONEFS_FEATURES_ALL,
// and update Server::update_required_client_features(). This feature bit
// is used to indicate that operator only wants clients from that release or
// later to mount StoneFS.
#define STONEFS_CURRENT_RELEASE  STONE_RELEASE_PACIFIC

// The first 5 bits are reserved for old ceph releases.
#define STONEFS_FEATURE_JEWEL		5
#define STONEFS_FEATURE_KRAKEN		6
#define STONEFS_FEATURE_LUMINOUS		7
#define STONEFS_FEATURE_MIMIC		8
#define STONEFS_FEATURE_REPLY_ENCODING   9
#define STONEFS_FEATURE_RECLAIM_CLIENT	10
#define STONEFS_FEATURE_LAZY_CAP_WANTED  11
#define STONEFS_FEATURE_MULTI_RECONNECT  12
#define STONEFS_FEATURE_NAUTILUS         12
#define STONEFS_FEATURE_DELEG_INO        13
#define STONEFS_FEATURE_OCTOPUS          13
#define STONEFS_FEATURE_METRIC_COLLECT   14
#define STONEFS_FEATURE_ALTERNATE_NAME   15
#define STONEFS_FEATURE_MAX              15

#define STONEFS_FEATURES_ALL {		\
  0, 1, 2, 3, 4,			\
  STONEFS_FEATURE_JEWEL,			\
  STONEFS_FEATURE_KRAKEN,		\
  STONEFS_FEATURE_LUMINOUS,		\
  STONEFS_FEATURE_MIMIC,			\
  STONEFS_FEATURE_REPLY_ENCODING,        \
  STONEFS_FEATURE_RECLAIM_CLIENT,	\
  STONEFS_FEATURE_LAZY_CAP_WANTED,	\
  STONEFS_FEATURE_MULTI_RECONNECT,	\
  STONEFS_FEATURE_NAUTILUS,              \
  STONEFS_FEATURE_DELEG_INO,             \
  STONEFS_FEATURE_OCTOPUS,               \
  STONEFS_FEATURE_METRIC_COLLECT,        \
  STONEFS_FEATURE_ALTERNATE_NAME,        \
}

#define STONEFS_METRIC_FEATURES_ALL {		\
    CLIENT_METRIC_TYPE_CAP_INFO,		\
    CLIENT_METRIC_TYPE_READ_LATENCY,		\
    CLIENT_METRIC_TYPE_WRITE_LATENCY,		\
    CLIENT_METRIC_TYPE_METADATA_LATENCY,	\
    CLIENT_METRIC_TYPE_DENTRY_LEASE,		\
    CLIENT_METRIC_TYPE_OPENED_FILES,		\
    CLIENT_METRIC_TYPE_PINNED_ICAPS,		\
    CLIENT_METRIC_TYPE_OPENED_INODES,		\
    CLIENT_METRIC_TYPE_READ_IO_SIZES,		\
    CLIENT_METRIC_TYPE_WRITE_IO_SIZES,		\
}

#define STONEFS_FEATURES_MDS_SUPPORTED STONEFS_FEATURES_ALL
#define STONEFS_FEATURES_MDS_REQUIRED {}

#define STONEFS_FEATURES_CLIENT_SUPPORTED STONEFS_FEATURES_ALL
#define STONEFS_FEATURES_CLIENT_REQUIRED {}

extern std::string_view cephfs_feature_name(size_t id);
extern int cephfs_feature_from_name(std::string_view name);
std::string cephfs_stringify_features(const feature_bitset_t& features);
void cephfs_dump_features(ceph::Formatter *f, const feature_bitset_t& features);

#endif
