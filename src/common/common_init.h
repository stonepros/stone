// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2009-2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_COMMON_INIT_H
#define STONE_COMMON_INIT_H

#include <deque>

#include "include/common_fwd.h"
#include "common/code_environment.h"

enum common_init_flags_t {
  // Set up defaults that make sense for an unprivileged daemon
  CINIT_FLAG_UNPRIVILEGED_DAEMON_DEFAULTS = 0x1,

  // By default, don't read a configuration file OR contact mons
  CINIT_FLAG_NO_DEFAULT_CONFIG_FILE = 0x2,

  // Don't close stderr (in daemonize)
  CINIT_FLAG_NO_CLOSE_STDERR = 0x4,

  // don't do anything daemonish, like create /var/run/stone, or print a banner
  CINIT_FLAG_NO_DAEMON_ACTIONS = 0x8,

  // don't drop privileges
  CINIT_FLAG_DEFER_DROP_PRIVILEGES = 0x10,

  // don't contact mons for config
  CINIT_FLAG_NO_MON_CONFIG = 0x20,

  // don't expose default cct perf counters
  CINIT_FLAG_NO_CCT_PERF_COUNTERS = 0x40,
};

#ifndef WITH_SEASTAR
class StoneInitParameters;

/*
 * NOTE: If you are writing a Stone daemon, ignore this function and call
 * global_init instead. It will call common_preinit for you.
 *
 * common_preinit creates the StoneContext.
 *
 * After this function gives you a StoneContext, you need to set up the
 * Stone configuration, which lives inside the StoneContext as md_config_t.
 * The initial settings are not very useful because they do not reflect what
 * the user asked for.
 *
 * This is usually done by something like this:
 * cct->_conf.parse_env();
 * cct->_conf.apply_changes();
 *
 * Your library may also supply functions to read a configuration file.
 */
StoneContext *common_preinit(const StoneInitParameters &iparams,
			    enum code_environment_t code_env, int flags);
#endif // #ifndef WITH_SEASTAR

/* Print out some parse error. */
void complain_about_parse_error(StoneContext *cct,
				const std::string& parse_error);

/* This function is called after you have done your last
 * fork. When you make this call, the system will initialize everything that
 * cannot be initialized before a fork.
 *
 * This includes things like starting threads, initializing libraries that
 * can't handle forking, and so forth.
 *
 * If you are writing a Stone library, you can call this pretty much any time.
 * We do not allow our library users to fork and continue using the Stone
 * libraries. The most obvious reason for this is that the threads started by
 * the Stone libraries would be destroyed by a fork().
 */
void common_init_finish(StoneContext *cct);

#endif
