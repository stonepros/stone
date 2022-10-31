// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2016 Red Hat Inc
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef COMMAND_TABLE_H_
#define COMMAND_TABLE_H_

#include "messages/MCommand.h"
#include "messages/MMgrCommand.h"

class CommandOp
{
  public:
  ConnectionRef con;
  stone_tid_t tid;

  std::vector<std::string> cmd;
  stone::buffer::list    inbl;
  Context      *on_finish;
  stone::buffer::list   *outbl;
  std::string  *outs;

  MessageRef get_message(const uuid_d &fsid,
			 bool mgr=false) const
  {
    if (mgr) {
      auto m = stone::make_message<MMgrCommand>(fsid);
      m->cmd = cmd;
      m->set_data(inbl);
      m->set_tid(tid);
      return m;
    } else {
      auto m = stone::make_message<MCommand>(fsid);
      m->cmd = cmd;
      m->set_data(inbl);
      m->set_tid(tid);
      return m;
    }
  }

  CommandOp(const stone_tid_t t) : tid(t), on_finish(nullptr),
                                  outbl(nullptr), outs(nullptr) {}
  CommandOp() : tid(0), on_finish(nullptr), outbl(nullptr), outs(nullptr) {}
};

/**
 * Hold client-side state for a collection of in-flight commands
 * to a remote service.
 */
template<typename T>
class CommandTable
{
protected:
  stone_tid_t last_tid;
  std::map<stone_tid_t, T> commands;

public:

  CommandTable()
    : last_tid(0)
  {}

  ~CommandTable()
  {
    stone_assert(commands.empty());
  }

  T& start_command()
  {
    stone_tid_t tid = last_tid++;
    commands.insert(std::make_pair(tid, T(tid)) );

    return commands.at(tid);
  }

  const std::map<stone_tid_t, T> &get_commands() const
  {
    return commands;
  }

  bool exists(stone_tid_t tid) const
  {
    return commands.count(tid) > 0;
  }

  T& get_command(stone_tid_t tid)
  {
    return commands.at(tid);
  }

  void erase(stone_tid_t tid)
  {
    commands.erase(tid);
  }

  void clear() {
    commands.clear();
  }
};

#endif

