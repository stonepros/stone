// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_RBD_MIRROR_H
#define STONE_RBD_MIRROR_H

#include "common/stone_context.h"
#include "common/stone_mutex.h"
#include "include/rados/librados.hpp"
#include "include/utime.h"
#include "ClusterWatcher.h"
#include "PoolReplayer.h"
#include "tools/rbd_mirror/Types.h"

#include <set>
#include <map>
#include <memory>
#include <atomic>

namespace journal { class CacheManagerHandler; }

namespace librbd { struct ImageCtx; }

namespace rbd {
namespace mirror {

template <typename> struct ServiceDaemon;
template <typename> struct Threads;
class CacheManagerHandler;
class MirrorAdminSocketHook;
class PoolMetaCache;

/**
 * Contains the main loop and overall state for rbd-mirror.
 *
 * Sets up mirroring, and coordinates between noticing config
 * changes and applying them.
 */
class Mirror {
public:
  Mirror(StoneContext *cct, const std::vector<const char*> &args);
  Mirror(const Mirror&) = delete;
  Mirror& operator=(const Mirror&) = delete;
  ~Mirror();

  int init();
  void run();
  void handle_signal(int signum);

  void print_status(Formatter *f);
  void start();
  void stop();
  void restart();
  void flush();
  void release_leader();

private:
  typedef ClusterWatcher::PoolPeers PoolPeers;
  typedef std::pair<int64_t, PeerSpec> PoolPeer;

  void update_pool_replayers(const PoolPeers &pool_peers,
                             const std::string& site_name);

  void create_cache_manager();
  void run_cache_manager(utime_t *next_run_interval);

  StoneContext *m_cct;
  std::vector<const char*> m_args;
  Threads<librbd::ImageCtx> *m_threads = nullptr;
  stone::mutex m_lock = stone::make_mutex("rbd::mirror::Mirror");
  stone::condition_variable m_cond;
  RadosRef m_local;
  std::unique_ptr<ServiceDaemon<librbd::ImageCtx>> m_service_daemon;

  // monitor local cluster for config changes in peers
  std::unique_ptr<ClusterWatcher> m_local_cluster_watcher;
  std::unique_ptr<CacheManagerHandler> m_cache_manager_handler;
  std::unique_ptr<PoolMetaCache> m_pool_meta_cache;
  std::map<PoolPeer, std::unique_ptr<PoolReplayer<>>> m_pool_replayers;
  std::atomic<bool> m_stopping = { false };
  bool m_manual_stop = false;
  MirrorAdminSocketHook *m_asok_hook;
  std::string m_site_name;
};

} // namespace mirror
} // namespace rbd

#endif // STONE_RBD_MIRROR_H
