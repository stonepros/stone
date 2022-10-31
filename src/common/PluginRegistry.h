// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone distributed storage system
 *
 * Copyright (C) 2013,2014 Cloudwatt <libre.licensing@cloudwatt.com>
 * Copyright (C) 2014 Red Hat <contact@redhat.com>
 *
 * Author: Loic Dachary <loic@dachary.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * 
 */

#ifndef STONE_COMMON_PLUGINREGISTRY_H
#define STONE_COMMON_PLUGINREGISTRY_H

#include <map>
#include <string>
#include "common/stone_mutex.h"
#include "include/common_fwd.h"

extern "C" {
  const char *__stone_plugin_version();
  int __stone_plugin_init(StoneContext *cct,
			 const std::string& type,
			 const std::string& name);
}

namespace stone {

  class Plugin {
  public:
    void *library;
    StoneContext *cct;

    explicit Plugin(StoneContext *cct) : library(NULL), cct(cct) {}
    virtual ~Plugin() {}
  };

  class PluginRegistry {
  public:
    StoneContext *cct;
    stone::mutex lock = stone::make_mutex("PluginRegistery::lock");
    bool loading;
    bool disable_dlclose;
    std::map<std::string,std::map<std::string,Plugin*> > plugins;

    explicit PluginRegistry(StoneContext *cct);
    ~PluginRegistry();

    int add(const std::string& type, const std::string& name,
	    Plugin *factory);
    int remove(const std::string& type, const std::string& name);
    Plugin *get(const std::string& type, const std::string& name);
    Plugin *get_with_load(const std::string& type, const std::string& name);

    int load(const std::string& type,
	     const std::string& name);
    int preload();
    int preload(const std::string& type);
  };
}

#endif
