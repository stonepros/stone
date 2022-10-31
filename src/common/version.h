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

#ifndef STONE_COMMON_VERSION_H
#define STONE_COMMON_VERSION_H

#include <string>

// Return a string describing the Stone version
const char *stone_version_to_str();

// Return a string with the Stone release
const char *stone_release_to_str(void);

// Return a string describing the git version
const char *git_version_to_str(void);

// Return a formatted string describing the stone and git versions
std::string const pretty_version_to_str(void);

// Release type ("dev", "rc", or "stable")
const char *stone_release_type(void);

#endif
