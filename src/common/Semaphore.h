// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */


#ifndef STONE_Sem_Posix__H
#define STONE_Sem_Posix__H

#include "common/stone_mutex.h"

class Semaphore
{
  stone::mutex m = stone::make_mutex("Semaphore::m");
  stone::condition_variable c;
  int count = 0;

  public:

  void Put()
  { 
    std::lock_guard l(m);
    count++;
    c.notify_all();
  }

  void Get() 
  {
    std::unique_lock l(m);
    while(count <= 0) {
      c.wait(l);
    }
    count--;
  }
};

#endif // !_Mutex_Posix_
