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
#include "include/stonefs/libstonefs.h"
#include <errno.h>
#include <fcntl.h>

TEST(LibStoneFS, ReaddirRCB) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  char c_dir[256];
  sprintf(c_dir, "/readdir_r_cb_tests_%d", getpid());
  struct stone_dir_result *dirp;
  ASSERT_EQ(0, stone_mkdirs(cmount, c_dir, 0777));
  ASSERT_LE(0, stone_opendir(cmount, c_dir, &dirp));

  // dir is empty, check that it only contains . and ..
  int buflen = 100;
  char *buf = new char[buflen];
  // . is 2, .. is 3 (for null terminators)
  ASSERT_EQ(5, stone_getdnames(cmount, dirp, buf, buflen));
  char c_file[256];
  sprintf(c_file, "/readdir_r_cb_tests_%d/foo", getpid());
  int fd = stone_open(cmount, c_file, O_CREAT, 0777);
  ASSERT_LT(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  // check correctness with one entry
  ASSERT_LE(0, stone_closedir(cmount, dirp));
  ASSERT_LE(0, stone_opendir(cmount, c_dir, &dirp));
  ASSERT_EQ(9, stone_getdnames(cmount, dirp, buf, buflen)); // ., .., foo

  // check correctness if buffer is too small
  ASSERT_LE(0, stone_closedir(cmount, dirp));
  ASSERT_GE(0, stone_opendir(cmount, c_dir, &dirp));
  ASSERT_EQ(-ERANGE, stone_getdnames(cmount, dirp, buf, 1));

  //check correctness if it needs to split listing
  ASSERT_LE(0, stone_closedir(cmount, dirp));
  ASSERT_LE(0, stone_opendir(cmount, c_dir, &dirp));
  ASSERT_EQ(5, stone_getdnames(cmount, dirp, buf, 6));
  ASSERT_EQ(4, stone_getdnames(cmount, dirp, buf, 6));

  // free cmount after finishing testing
  ASSERT_LE(0, stone_closedir(cmount, dirp));
  ASSERT_EQ(0, stone_unmount(cmount));
  ASSERT_EQ(0, stone_release(cmount));
}
