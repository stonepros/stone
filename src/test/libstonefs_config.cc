// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "gtest/gtest.h"
#include "include/compat.h"
#include "include/stonefs/libstonefs.h"

#include <sstream>
#include <string>
#include <string.h>

using std::string;

TEST(LibStoneConfig, SimpleSet) {
  struct stone_mount_info *cmount;
  int ret = stone_create(&cmount, NULL);
  ASSERT_EQ(ret, 0);

  ret = stone_conf_set(cmount, "leveldb_max_open_files", "21");
  ASSERT_EQ(ret, 0);

  char buf[128];
  memset(buf, 0, sizeof(buf));
  ret = stone_conf_get(cmount, "leveldb_max_open_files", buf, sizeof(buf));
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(string("21"), string(buf));

  stone_shutdown(cmount);
}

TEST(LibStoneConfig, ArgV) {
  struct stone_mount_info *cmount;
  int ret = stone_create(&cmount, NULL);
  ASSERT_EQ(ret, 0);

  const char *argv[] = { "foo", "--leveldb-max-open-files", "2",
			 "--key", "my-key", NULL };
  size_t argc = (sizeof(argv) / sizeof(argv[0])) - 1;
  stone_conf_parse_argv(cmount, argc, argv);

  char buf[128];
  memset(buf, 0, sizeof(buf));
  ret = stone_conf_get(cmount, "key", buf, sizeof(buf));
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(string("my-key"), string(buf));

  memset(buf, 0, sizeof(buf));
  ret = stone_conf_get(cmount, "leveldb_max_open_files", buf, sizeof(buf));
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(string("2"), string(buf));

  stone_shutdown(cmount);
}
