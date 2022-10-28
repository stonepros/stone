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

#include "common/version.h"

#include <stdlib.h>
#include <sstream>

#include "stone_ver.h"
#include "common/stone_strings.h"

#define _STR(x) #x
#define STRINGIFY(x) _STR(x)

const char *stone_version_to_str()
{
  char* debug_version_for_testing = getenv("stone_debug_version_for_testing");
  if (debug_version_for_testing) {
    return debug_version_for_testing;
  } else {
    return STONE_GIT_NICE_VER;
  }
}

const char *stone_release_to_str(void)
{
  return stone_release_name(STONE_RELEASE);
}

const char *git_version_to_str(void)
{
  return STRINGIFY(STONE_GIT_VER);
}

std::string const pretty_version_to_str(void)
{
  std::ostringstream oss;
  oss << "stone version " << STONE_GIT_NICE_VER
      << " (" << STRINGIFY(STONE_GIT_VER) << ") "
      << stone_release_name(STONE_RELEASE)
      << " (" << STONE_RELEASE_TYPE << ")";
  return oss.str();
}

const char *stone_release_type(void)
{
  return STONE_RELEASE_TYPE;
}
