// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network/Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 */
#ifndef HEAP_PROFILER_H_
#define HEAP_PROFILER_H_

#include <string>
#include <vector>
#include "common/config.h"

class LogClient;

/*
 * Stone glue for the Google perftools heap profiler, included
 * as part of tcmalloc. This replaces ugly function pointers
 * and #ifdef hacks!
 */
bool stone_using_tcmalloc();

/*
 * Configure the heap profiler
 */
void stone_heap_profiler_init();

void stone_heap_profiler_stats(char *buf, int length);

void stone_heap_release_free_memory();

double stone_heap_get_release_rate();

void stone_heap_get_release_rate(double value);

bool stone_heap_profiler_running();

void stone_heap_profiler_start();

void stone_heap_profiler_stop();

void stone_heap_profiler_dump(const char *reason);

bool stone_heap_get_numeric_property(const char *property, size_t *value);

bool stone_heap_set_numeric_property(const char *property, size_t value);

void stone_heap_profiler_handle_command(const std::vector<std::string> &cmd,
                                       std::ostream& out);

#endif /* HEAP_PROFILER_H_ */
