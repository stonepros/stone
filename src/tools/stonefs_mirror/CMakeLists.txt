set(stonefs_mirror_internal
  ClusterWatcher.cc
  Mirror.cc
  FSMirror.cc
  InstanceWatcher.cc
  MirrorWatcher.cc
  PeerReplayer.cc
  ServiceDaemon.cc
  Types.cc
  Utils.cc
  Watcher.cc
  watcher/RewatchRequest.cc)

add_executable(stonefs-mirror
  main.cc)

add_library(stonefs_mirror_internal STATIC
  ${stonefs_mirror_internal})

target_link_libraries(stonefs-mirror
  stonefs_mirror_internal
  global
  stone-common
  cls_stonefs_client
  librados
  mds
  stonefs
  ${ALLOC_LIBS})

install(TARGETS stonefs-mirror DESTINATION bin)
