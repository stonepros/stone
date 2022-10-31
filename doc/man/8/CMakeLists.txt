set(client_srcs
  stone-syn.rst
  stone-conf.rst
  stone.rst
  stone-authtool.rst
  stone-kvstore-tool.rst
  rados.rst
  stone-post-file.rst
  stone-dencoder.rst)

set(server_srcs
  stone-deploy.rst
  crushtool.rst
  stone-run.rst
  mount.stone.rst
  mount.fuse.stone.rst
  stone-create-keys.rst)
if(WITH_TESTS)
list(APPEND server_srcs
  stone-debugpack.rst)
endif(WITH_TESTS)

set(osd_srcs
  stone-clsinfo.rst
  stone-volume.rst
  stone-volume-systemd.rst
  stone-osd.rst
  osdmaptool.rst
  stone-bluestore-tool.rst)

set(mon_srcs
  stone-mon.rst
  monmaptool.rst)

list(APPEND man_srcs
  ${client_srcs}
  ${server_srcs}
  ${osd_srcs}
  ${mon_srcs}
  stone-mds.rst
  stonefs-top.rst
  librados-config.rst
  stoneadm.rst
  stonefs-mirror.rst)

if(HAVE_LIBFUSE)
  list(APPEND man_srcs
    stone-fuse.rst
    rbd-fuse.rst)
endif()

if(WITH_RADOSGW)
  list(APPEND man_srcs
	radosgw.rst
	radosgw-admin.rst
	rgw-orphan-list.rst
	stone-diff-sorted.rst)
endif()

if(WITH_RBD)
  list(APPEND man_srcs
	stone-rbdnamer.rst
	rbd-mirror.rst
	rbd-replay-many.rst
	rbd-replay-prep.rst
	rbd-replay.rst
	rbdmap.rst
	rbd.rst)
  if(LINUX)
    list(APPEND man_srcs rbd-nbd.rst)
  endif()
  if(FREEBSD)
    list(APPEND man_srcs rbd-ggate.rst)
  endif()
endif()

list(APPEND man_srcs stone-immutable-object-cache.rst)
foreach(man ${man_srcs})
  list(APPEND sphinx_input ${CMAKE_CURRENT_SOURCE_DIR}/${man})
  # mount.stone.rst => mount if we use
  # get_filename_component(cmd ${man} NAME_WE)
  string(REGEX REPLACE ".rst$" "" cmd ${man})
  list(APPEND sphinx_output ${sphinx_output_dir}/${cmd}.8)
  install(FILES ${sphinx_output_dir}/${cmd}.8
    DESTINATION ${STONE_MAN_DIR}/man8)
endforeach()

set(sphinx_input ${sphinx_input} PARENT_SCOPE)
set(sphinx_output ${sphinx_output} PARENT_SCOPE)
