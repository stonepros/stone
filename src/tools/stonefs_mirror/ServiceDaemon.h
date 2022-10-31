// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONEFS_MIRROR_SERVICE_DAEMON_H
#define STONEFS_MIRROR_SERVICE_DAEMON_H

#include "common/stone_mutex.h"
#include "common/Timer.h"
#include "mds/FSMap.h"
#include "Types.h"

namespace stonefs {
namespace mirror {

class ServiceDaemon {
public:
  ServiceDaemon(StoneContext *cct, RadosRef rados);
  ~ServiceDaemon();

  int init();

  void add_filesystem(fs_cluster_id_t fscid, std::string_view fs_name);
  void remove_filesystem(fs_cluster_id_t fscid);

  void add_peer(fs_cluster_id_t fscid, const Peer &peer);
  void remove_peer(fs_cluster_id_t fscid, const Peer &peer);

  void add_or_update_fs_attribute(fs_cluster_id_t fscid, std::string_view key,
                                  AttributeValue value);
  void add_or_update_peer_attribute(fs_cluster_id_t fscid, const Peer &peer,
                                    std::string_view key, AttributeValue value);

private:
  struct Filesystem {
    std::string fs_name;
    Attributes fs_attributes;
    std::map<Peer, Attributes> peer_attributes;

    Filesystem(std::string_view fs_name)
      : fs_name(fs_name) {
    }
  };

  const std::string STONEFS_MIRROR_AUTH_ID_PREFIX = "stonefs-mirror.";

  StoneContext *m_cct;
  RadosRef m_rados;
  SafeTimer *m_timer;
  stone::mutex m_timer_lock = stone::make_mutex("stonefs::mirror::ServiceDaemon");

  stone::mutex m_lock = stone::make_mutex("stonefs::mirror::service_daemon");
  Context *m_timer_ctx = nullptr;
  std::map<fs_cluster_id_t, Filesystem> m_filesystems;

  void schedule_update_status();
  void update_status();
};

} // namespace mirror
} // namespace stonefs

#endif // STONEFS_MIRROR_SERVICE_DAEMON_H
