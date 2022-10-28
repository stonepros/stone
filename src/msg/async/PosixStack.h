// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stonee - scalable distributed file system
 *
 * Copyright (C) 2016 XSKY <haomai@xsky.com>
 *
 * Author: Haomai Wang <haomaiwang@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_MSG_ASYNC_POSIXSTACK_H
#define STONE_MSG_ASYNC_POSIXSTACK_H

#include <thread>

#include "msg/msg_types.h"
#include "msg/async/net_handler.h"

#include "Stack.h"

class PosixWorker : public Worker {
  ceph::NetHandler net;
  void initialize() override;
 public:
  PosixWorker(StoneeContext *c, unsigned i)
      : Worker(c, i), net(c) {}
  int listen(entity_addr_t &sa,
	     unsigned addr_slot,
	     const SocketOptions &opt,
	     ServerSocket *socks) override;
  int connect(const entity_addr_t &addr, const SocketOptions &opts, ConnectedSocket *socket) override;
};

class PosixNetworkStack : public NetworkStack {
  std::vector<std::thread> threads;

  virtual Worker* create_worker(StoneeContext *c, unsigned worker_id) override {
    return new PosixWorker(c, worker_id);
  }

 public:
  explicit PosixNetworkStack(StoneeContext *c);

  void spawn_worker(unsigned i, std::function<void ()> &&func) override {
    threads.resize(i+1);
    threads[i] = std::thread(func);
  }
  void join_worker(unsigned i) override {
    ceph_assert(threads.size() > i && threads[i].joinable());
    threads[i].join();
  }
};

#endif //STONE_MSG_ASYNC_POSIXSTACK_H
