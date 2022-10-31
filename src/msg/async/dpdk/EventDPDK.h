// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2015 XSky <haomai@xsky.com>
 *
 * Author: Haomai Wang <haomaiwang@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_EVENTDPDK_H
#define STONE_EVENTDPDK_H

#include "msg/async/Event.h"
#include "msg/async/Stack.h"
#include "UserspaceEvent.h"

class DPDKDriver : public EventDriver {
  StoneContext *cct;

 public:
  UserspaceEventManager manager;

  explicit DPDKDriver(StoneContext *c): cct(c), manager(c) {}
  virtual ~DPDKDriver() { }

  int init(EventCenter *c, int nevent) override;
  int add_event(int fd, int cur_mask, int add_mask) override;
  int del_event(int fd, int cur_mask, int del_mask) override;
  int resize_events(int newsize) override;
  int event_wait(vector<FiredFileEvent> &fired_events, struct timeval *tp) override;
  bool need_wakeup() override { return false; }
};

#endif //STONE_EVENTDPDK_H
