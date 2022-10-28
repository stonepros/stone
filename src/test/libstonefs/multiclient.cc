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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef __linux__
#include <sys/xattr.h>
#endif

TEST(LibStoneFS, MulticlientSimple) {
  struct stone_mount_info *ca, *cb;
  ASSERT_EQ(stone_create(&ca, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(ca, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(ca, NULL));
  ASSERT_EQ(stone_mount(ca, NULL), 0);

  ASSERT_EQ(stone_create(&cb, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cb, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cb, NULL));
  ASSERT_EQ(stone_mount(cb, NULL), 0);

  char name[20];
  snprintf(name, sizeof(name), "foo.%d", getpid());
  int fda = stone_open(ca, name, O_CREAT|O_RDWR, 0644);
  ASSERT_LE(0, fda);
  int fdb = stone_open(cb, name, O_CREAT|O_RDWR, 0644);
  ASSERT_LE(0, fdb);

  char bufa[4] = "foo";
  char bufb[4];

  for (int i=0; i<10; i++) {
    strcpy(bufa, "foo");
    ASSERT_EQ((int)sizeof(bufa), stone_write(ca, fda, bufa, sizeof(bufa), i*6));
    ASSERT_EQ((int)sizeof(bufa), stone_read(cb, fdb, bufb, sizeof(bufa), i*6));
    ASSERT_EQ(0, memcmp(bufa, bufb, sizeof(bufa)));
    strcpy(bufb, "bar");
    ASSERT_EQ((int)sizeof(bufb), stone_write(cb, fdb, bufb, sizeof(bufb), i*6+3));
    ASSERT_EQ((int)sizeof(bufb), stone_read(ca, fda, bufa, sizeof(bufb), i*6+3));
    ASSERT_EQ(0, memcmp(bufa, bufb, sizeof(bufa)));
  }

  stone_close(ca, fda);
  stone_close(cb, fdb);

  stone_shutdown(ca);
  stone_shutdown(cb);
}

TEST(LibStoneFS, MulticlientHoleEOF) {
  struct stone_mount_info *ca, *cb;
  ASSERT_EQ(stone_create(&ca, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(ca, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(ca, NULL));
  ASSERT_EQ(stone_mount(ca, NULL), 0);

  ASSERT_EQ(stone_create(&cb, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cb, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cb, NULL));
  ASSERT_EQ(stone_mount(cb, NULL), 0);

  char name[20];
  snprintf(name, sizeof(name), "foo.%d", getpid());
  int fda = stone_open(ca, name, O_CREAT|O_RDWR, 0644);
  ASSERT_LE(0, fda);
  int fdb = stone_open(cb, name, O_CREAT|O_RDWR, 0644);
  ASSERT_LE(0, fdb);

  ASSERT_EQ(3, stone_write(ca, fda, "foo", 3, 0));
  ASSERT_EQ(0, stone_ftruncate(ca, fda, 1000000));

  char buf[4];
  ASSERT_EQ(2, stone_read(cb, fdb, buf, sizeof(buf), 1000000-2));
  ASSERT_EQ(0, buf[0]);
  ASSERT_EQ(0, buf[1]);

  stone_close(ca, fda);
  stone_close(cb, fdb);

  stone_shutdown(ca);
  stone_shutdown(cb);
}
