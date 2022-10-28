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
#include "include/int_types.h"

#include "gtest/gtest.h"
#include "include/stone_fs.h"
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
#include <signal.h>

TEST(Caps, ReadZero) {

  int mypid = getpid();
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  int i = 0;
  for(; i < 30; ++i) {

    char c_path[1024];
    sprintf(c_path, "/caps_rzfile_%d_%d", mypid, i);
    int fd = stone_open(cmount, c_path, O_CREAT|O_TRUNC|O_WRONLY, 0644);
    ASSERT_LT(0, fd);

    int expect = STONE_CAP_FILE_EXCL | STONE_CAP_FILE_WR | STONE_CAP_FILE_BUFFER;
    int caps = stone_debug_get_fd_caps(cmount, fd);

    ASSERT_EQ(expect, caps & expect);
    ASSERT_EQ(0, stone_close(cmount, fd));

    caps = stone_debug_get_file_caps(cmount, c_path);
    ASSERT_EQ(expect, caps & expect);

    char cw_path[1024];
    sprintf(cw_path, "/caps_wzfile_%d_%d", mypid, i);
    int wfd = stone_open(cmount, cw_path, O_CREAT|O_TRUNC|O_WRONLY, 0644);
    ASSERT_LT(0, wfd);

    char wbuf[4096];
    ASSERT_EQ(4096, stone_write(cmount, wfd, wbuf, 4096, 0));

    ASSERT_EQ(0, stone_close(cmount, wfd));

    struct stone_statx stx;
    ASSERT_EQ(0, stone_statx(cmount, c_path, &stx, STONE_STATX_MTIME, 0));

    caps = stone_debug_get_file_caps(cmount, c_path);
    ASSERT_EQ(expect, caps & expect);
  }

  ASSERT_EQ(0, stone_conf_set(cmount, "client_debug_inject_tick_delay", "20"));

  for(i = 0; i < 30; ++i) {

    char c_path[1024];
    sprintf(c_path, "/caps_rzfile_%d_%d", mypid, i);

    int fd = stone_open(cmount, c_path, O_RDONLY, 0);
    ASSERT_LT(0, fd);
    char buf[256];

    int expect = STONE_CAP_FILE_RD | STONE_STAT_CAP_SIZE | STONE_CAP_FILE_CACHE;
    int caps = stone_debug_get_fd_caps(cmount, fd);
    ASSERT_EQ(expect, caps & expect);
    ASSERT_EQ(0, stone_read(cmount, fd, buf, 256, 0));

    caps = stone_debug_get_fd_caps(cmount, fd);
    ASSERT_EQ(expect, caps & expect);
    ASSERT_EQ(0, stone_close(cmount, fd));

  }
  stone_shutdown(cmount);
}
