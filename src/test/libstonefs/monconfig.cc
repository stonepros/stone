// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2020 Red Hat
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "gtest/gtest.h"
#include "include/stonefs/libstonefs.h"
#include "common/stone_context.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

class MonConfig : public ::testing::Test
{
  protected:
    struct stone_mount_info *ca;

    void SetUp() override {
      ASSERT_EQ(0, stone_create(&ca, NULL));
      ASSERT_EQ(0, stone_conf_read_file(ca, NULL));
      ASSERT_EQ(0, stone_conf_parse_env(ca, NULL));
    }

    void TearDown() override {
      stone_shutdown(ca);
    }

    // Helper to remove/unset all possible mon information from ConfigProxy
    void clear_mon_config(StoneContext *cct) {
      auto& conf = cct->_conf;
      // Clear safe_to_start_threads, allowing updates to config values
      conf._clear_safe_to_start_threads();
      ASSERT_EQ(0, conf.set_val("monmap", "", nullptr));
      ASSERT_EQ(0, conf.set_val("mon_host", "", nullptr));
      ASSERT_EQ(0, conf.set_val("mon_dns_srv_name", "", nullptr));
      conf.set_safe_to_start_threads();
    }

    // Helper to test basic operation on a mount
    void use_mount(struct stone_mount_info *mnt, string name_prefix) {
      char name[20];
      snprintf(name, sizeof(name), "%s.%d", name_prefix.c_str(), getpid());
      int fd = stone_open(mnt, name, O_CREAT|O_RDWR, 0644);
      ASSERT_LE(0, fd);

      stone_close(mnt, fd);
    }
};

TEST_F(MonConfig, MonAddrsMissing) {
  StoneContext *cct;

  // Test mount failure when there is no known mon config source
  cct = stone_get_mount_context(ca);
  ASSERT_NE(nullptr, cct);
  clear_mon_config(cct);

  ASSERT_EQ(-ENOENT, stone_mount(ca, NULL));
}

TEST_F(MonConfig, MonAddrsInConfigProxy) {
  // Test a successful mount with default mon config source in ConfigProxy
  ASSERT_EQ(0, stone_mount(ca, NULL));

  use_mount(ca, "foo");
}

TEST_F(MonConfig, MonAddrsInCct) {
  struct stone_mount_info *cb;
  StoneContext *cct;

  // Perform mount to bootstrap mon addrs in StoneContext
  ASSERT_EQ(0, stone_mount(ca, NULL));

  // Reuse bootstrapped StoneContext, clearing ConfigProxy mon addr sources
  cct = stone_get_mount_context(ca);
  ASSERT_NE(nullptr, cct);
  clear_mon_config(cct);
  ASSERT_EQ(0, stone_create_with_context(&cb, cct));

  // Test a successful mount with only mon values in StoneContext
  ASSERT_EQ(0, stone_mount(cb, NULL));

  use_mount(ca, "bar");
  use_mount(cb, "bar");

  stone_shutdown(cb);
}
