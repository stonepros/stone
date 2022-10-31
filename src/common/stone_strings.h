// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <cstdint>

const char *stone_entity_type_name(int type);
const char *stone_con_mode_name(int con_mode);
const char *stone_osd_op_name(int op);
const char *stone_osd_state_name(int s);
const char *stone_release_name(int r);
std::uint64_t stone_release_features(int r);
int stone_release_from_features(std::uint64_t features);
const char *stone_osd_watch_op_name(int o);
const char *stone_osd_alloc_hint_flag_name(int f);
const char *stone_mds_state_name(int s);
const char *stone_session_op_name(int op);
const char *stone_mds_op_name(int op);
const char *stone_cap_op_name(int op);
const char *stone_lease_op_name(int o);
const char *stone_snap_op_name(int o);
const char *stone_watch_event_name(int e);
const char *stone_pool_op_name(int op);
const char *stone_osd_backoff_op_name(int op);
