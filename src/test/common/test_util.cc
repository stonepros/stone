// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "common/stone_context.h"
#include "include/util.h"
#include "gtest/gtest.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#if defined(__linux__)
TEST(util, collect_sys_info)
{
  if (!fs::exists("/etc/os-release")) {
    GTEST_SKIP() << "skipping as '/etc/os-release' does not exist";
  }

  map<string, string> sys_info;

  StoneContext *cct = (new StoneContext(STONE_ENTITY_TYPE_CLIENT))->get();
  collect_sys_info(&sys_info, cct);

  ASSERT_TRUE(sys_info.find("distro") != sys_info.end());
  ASSERT_TRUE(sys_info.find("distro_description") != sys_info.end());

  cct->put();
}
#endif
