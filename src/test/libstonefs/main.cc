// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network
 * Copyright (C) 2016 Red Hat
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "gtest/gtest.h"
#include "include/stonefs/libstonefs.h"

static int update_root_mode()
{
  struct stone_mount_info *admin;
  int r = stone_create(&admin, NULL);
  if (r < 0)
    return r;
  stone_conf_read_file(admin, NULL);
  stone_conf_parse_env(admin, NULL);
  stone_conf_set(admin, "client_permissions", "false");
  r = stone_mount(admin, "/");
  if (r < 0)
    goto out;
  r = stone_chmod(admin, "/", 01777);
out:
  stone_shutdown(admin);
  return r;
}


int main(int argc, char **argv)
{
  int r = update_root_mode();
  if (r < 0)
    exit(1);

  ::testing::InitGoogleTest(&argc, argv);

  srand(getpid());

  return RUN_ALL_TESTS();
}
