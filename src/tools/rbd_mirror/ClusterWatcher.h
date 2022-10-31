// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_RBD_MIRROR_CLUSTER_WATCHER_H
#define STONE_RBD_MIRROR_CLUSTER_WATCHER_H

#include <map>
#include <memory>
#include <set>

#include "common/stone_context.h"
#include "common/stone_mutex.h"
#include "common/Timer.h"
#include "include/rados/librados.hpp"
#include "tools/rbd_mirror/Types.h"
#include "tools/rbd_mirror/service_daemon/Types.h"
#include <unordered_map>

namespace librbd { struct ImageCtx; }

namespace rbd {
namespace mirror {

template <typename> class ServiceDaemon;

/**
 * Tracks mirroring configuration for pools in a single
 * cluster.
 */
class ClusterWatcher {
public:
  struct PeerSpecCompare {
    bool operator()(const PeerSpec& lhs, const PeerSpec& rhs) const {
      return (lhs.uuid < rhs.uuid);
    }
  };
  typedef std::set<PeerSpec, PeerSpecCompare> Peers;
  typedef std::map<int64_t, Peers>  PoolPeers;

  ClusterWatcher(RadosRef cluster, stone::mutex &lock,
                 ServiceDaemon<librbd::ImageCtx>* service_daemon);
  ~ClusterWatcher() = default;
  ClusterWatcher(const ClusterWatcher&) = delete;
  ClusterWatcher& operator=(const ClusterWatcher&) = delete;

  // Caller controls frequency of calls
  void refresh_pools();
  const PoolPeers& get_pool_peers() const;
  std::string get_site_name() const;

private:
  typedef std::unordered_map<int64_t, service_daemon::CalloutId> ServicePools;

  RadosRef m_cluster;
  stone::mutex &m_lock;
  ServiceDaemon<librbd::ImageCtx>* m_service_daemon;

  ServicePools m_service_pools;
  PoolPeers m_pool_peers;
  std::string m_site_name;

  void read_pool_peers(PoolPeers *pool_peers);

  int read_site_name(std::string* site_name);

  int resolve_peer_site_config_keys(
      int64_t pool_id, const std::string& pool_name, PeerSpec* peer);
};

} // namespace mirror
} // namespace rbd

#endif // STONE_RBD_MIRROR_CLUSTER_WATCHER_H
