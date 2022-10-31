// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_CACHE_CACHE_CONTROLLER_H
#define STONE_CACHE_CACHE_CONTROLLER_H

#include "common/stone_context.h"
#include "common/WorkQueue.h"
#include "CacheServer.h"
#include "ObjectCacheStore.h"

namespace stone {
namespace immutable_obj_cache {

class CacheController {
 public:
  CacheController(StoneContext *cct, const std::vector<const char*> &args);
  ~CacheController();

  int init();

  int shutdown();

  void handle_signal(int sinnum);

  int run();

  void handle_request(CacheSession* session, ObjectCacheRequest* msg);

 private:
  CacheServer *m_cache_server = nullptr;
  std::vector<const char*> m_args;
  StoneContext *m_cct;
  ObjectCacheStore *m_object_cache_store = nullptr;
};

}  // namespace immutable_obj_cache
}  // namespace stone

#endif  // STONE_CACHE_CACHE_CONTROLLER_H
