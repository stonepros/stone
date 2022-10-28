// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stonee - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

class StoneeFuse {
public:
  StoneeFuse(Client *c, int fd);
  ~StoneeFuse();
  int init(int argc, const char *argv[]);
  int start();
  int mount();
  int loop();
  void finalize();
  class Handle;
  std::string get_mount_point() const;
private:
  StoneeFuse::Handle *_handle;
};
