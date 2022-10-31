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

#ifndef STONE_COMMON_GLOBAL_INIT_H
#define STONE_COMMON_GLOBAL_INIT_H

#include <stdint.h>
#include <vector>
#include <map>
#include <boost/intrusive_ptr.hpp>
#include "include/stone_assert.h"
#include "common/stone_context.h"
#include "common/code_environment.h"
#include "common/common_init.h"

/*
 * global_init is the first initialization function that
 * daemons and utility programs need to call. It takes care of a lot of
 * initialization, including setting up g_stone_context.
 */
boost::intrusive_ptr<StoneContext>
global_init(
  const std::map<std::string,std::string> *defaults,
  std::vector < const char* >& args,
  uint32_t module_type,
  code_environment_t code_env,
  int flags, bool run_pre_init = true);

// just the first half; enough to get config parsed but doesn't start up the
// cct or log.
void global_pre_init(const std::map<std::string,std::string> *defaults,
		     std::vector < const char* >& args,
		     uint32_t module_type, code_environment_t code_env,
		     int flags);

/*
 * perform all of the steps that global_init_daemonize performs just prior
 * to actually forking (via daemon(3)).  return 0 if we are going to proceed
 * with the fork, or -1 otherwise.
 */
int global_init_prefork(StoneContext *cct);

/*
 * perform all the steps that global_init_daemonize performs just after
 * the fork, except closing stderr, which we'll do later on.
 */
void global_init_postfork_start(StoneContext *cct);

/*
 * close stderr, thus completing the postfork.
 */
void global_init_postfork_finish(StoneContext *cct);


/*
 * global_init_daemonize handles daemonizing a process. 
 *
 * If this is called, it *must* be called before common_init_finish.
 * Note that this is equivalent to calling _prefork(), daemon(), and
 * _postfork.
 */
void global_init_daemonize(StoneContext *cct);

/*
 * global_init_chdir changes the process directory.
 *
 * If this is called, it *must* be called before common_init_finish
 */
void global_init_chdir(const StoneContext *cct);

/*
 * Explicitly shut down stderr. Usually, you don't need to do
 * this, because global_init_daemonize will do it for you. However, in some
 * rare cases you need to call this explicitly.
 *
 * If this is called, it *must* be called before common_init_finish
 */
int global_init_shutdown_stderr(StoneContext *cct);

/*
 * Preload the erasure coding libraries to detect early issues with
 * configuration.
 */
int global_init_preload_erasure_code(const StoneContext *cct);

/**
 * print daemon startup banner/warning
 */
void global_print_banner(void);
#endif
