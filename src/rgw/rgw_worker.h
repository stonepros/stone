

// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */


#pragma once

#include <atomic>

#include "common/Thread.h"
#include "common/stone_mutex.h"
#include "include/common_fwd.h"

#define dout_subsys stone_subsys_rgw

class RGWRados;

class RGWRadosThread {
  class Worker : public Thread, public DoutPrefixProvider {
    StoneContext *cct;
    RGWRadosThread *processor;
    stone::mutex lock = stone::make_mutex("RGWRadosThread::Worker");
    stone::condition_variable cond;

    void wait() {
      std::unique_lock l{lock};
      cond.wait(l);
    };

    void wait_interval(const stone::real_clock::duration& wait_time) {
      std::unique_lock l{lock};
      cond.wait_for(l, wait_time);
    }

  public:
    Worker(StoneContext *_cct, RGWRadosThread *_p) : cct(_cct), processor(_p) {}
    void *entry() override;
    void signal() {
      std::lock_guard l{lock};
      cond.notify_all();
    }

  StoneContext *get_cct() const { return cct; }
  unsigned get_subsys() const { return dout_subsys; }
  std::ostream& gen_prefix(std::ostream& out) const { return out << "rgw rados thread: "; }

  };

  Worker *worker;

protected:
  StoneContext *cct;
  RGWRados *store;

  std::atomic<bool> down_flag = { false };

  string thread_name;

  virtual uint64_t interval_msec() = 0;
  virtual void stop_process() {}
public:
  RGWRadosThread(RGWRados *_store, const string& thread_name = "radosgw")
    : worker(NULL), cct(_store->ctx()), store(_store), thread_name(thread_name) {}
  virtual ~RGWRadosThread() {
    stop();
  }

  virtual int init(const DoutPrefixProvider *dpp) { return 0; }
  virtual int process(const DoutPrefixProvider *dpp) = 0;

  bool going_down() { return down_flag; }

  void start();
  void stop();

  void signal() {
    if (worker) {
      worker->signal();
    }
  }
};

