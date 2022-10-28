set(rados_srcs
  rados/rados.cc
  RadosDump.cc
  rados/RadosImport.cc
  rados/PoolDump.cc
  ${PROJECT_SOURCE_DIR}/src/common/util.cc
  ${PROJECT_SOURCE_DIR}/src/common/obj_bencher.cc
  ${PROJECT_SOURCE_DIR}/src/osd/ECUtil.cc)
add_executable(rados ${rados_srcs})

target_link_libraries(rados librados global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})
if(WITH_LIBRADOSSTRIPER)
  target_link_libraries(rados radosstriper)
else()
  target_link_libraries(rados cls_lock_client)
endif()
install(TARGETS rados DESTINATION bin)

if(NOT WIN32)
  set(neorados_srcs
      neorados.cc)
  add_executable(neorados ${neorados_srcs})
  target_link_libraries(neorados libneorados spawn fmt::fmt ${CMAKE_DL_LIBS})
  #install(TARGETS neorados DESTINATION bin)
endif()

if(WITH_TESTS)
add_executable(stone_scratchtool scratchtool.c)
target_link_libraries(stone_scratchtool librados global)
install(TARGETS stone_scratchtool DESTINATION bin)

add_executable(stone_scratchtoolpp scratchtoolpp.cc)
target_link_libraries(stone_scratchtoolpp librados global)
install(TARGETS stone_scratchtoolpp DESTINATION bin)

add_executable(stone_radosacl radosacl.cc)
target_link_libraries(stone_radosacl librados global)
install(TARGETS stone_radosacl DESTINATION bin)

install(PROGRAMS
  stone-monstore-update-crush.sh
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/stone)
endif(WITH_TESTS)

add_executable(stone-osdomap-tool stone_osdomap_tool.cc)
target_link_libraries(stone-osdomap-tool os global Boost::program_options)
install(TARGETS stone-osdomap-tool DESTINATION bin)

add_executable(stone-monstore-tool
  stone_monstore_tool.cc
  ../auth/stonex/StonexKeyServer.cc
  ../mgr/mgr_commands.cc)
target_link_libraries(stone-monstore-tool os global Boost::program_options)
install(TARGETS stone-monstore-tool DESTINATION bin)

add_executable(stone-objectstore-tool
  stone_objectstore_tool.cc
  rebuild_mondb.cc
  RadosDump.cc)
target_link_libraries(stone-objectstore-tool osd os global Boost::program_options ${CMAKE_DL_LIBS})
if(WITH_FUSE)
  target_link_libraries(stone-objectstore-tool FUSE::FUSE)
endif(WITH_FUSE)
install(TARGETS stone-objectstore-tool DESTINATION bin)

if(WITH_LIBSTONEFS)
if(WITH_TESTS)
  add_executable(stone-client-debug stone-client-debug.cc)
  target_link_libraries(stone-client-debug stonefs global client)
  install(TARGETS stone-client-debug DESTINATION bin)
endif(WITH_TESTS)
endif(WITH_LIBSTONEFS)

add_executable(stone-kvstore-tool
  kvstore_tool.cc
  stone_kvstore_tool.cc)
target_link_libraries(stone-kvstore-tool os global)
install(TARGETS stone-kvstore-tool DESTINATION bin)

set(stone_conf_srcs stone_conf.cc)
add_executable(stone-conf ${stone_conf_srcs})
target_link_libraries(stone-conf global)
install(TARGETS stone-conf DESTINATION bin)

set(crushtool_srcs crushtool.cc)
add_executable(crushtool ${crushtool_srcs})
target_link_libraries(crushtool global)
install(TARGETS crushtool DESTINATION bin)

set(monmaptool_srcs monmaptool.cc)
add_executable(monmaptool ${monmaptool_srcs})
target_link_libraries(monmaptool global)
install(TARGETS monmaptool DESTINATION bin)

set(osdomaptool_srcs osdmaptool.cc)
add_executable(osdmaptool ${osdomaptool_srcs})
target_link_libraries(osdmaptool global)
install(TARGETS osdmaptool DESTINATION bin)

set(stone-diff-sorted_srcs stone-diff-sorted.cc)
add_executable(stone-diff-sorted ${stone-diff-sorted_srcs})
set_target_properties(stone-diff-sorted PROPERTIES
  SKIP_RPATH TRUE
  INSTALL_RPATH "")
install(TARGETS stone-diff-sorted DESTINATION bin)

if(WITH_TESTS)
set(stone_psim_srcs psim.cc)
add_executable(stone_psim ${stone_psim_srcs})
target_link_libraries(stone_psim global)
install(TARGETS stone_psim DESTINATION bin)
endif(WITH_TESTS)

set(stone_authtool_srcs stone_authtool.cc)
add_executable(stone-authtool ${stone_authtool_srcs})
target_link_libraries(stone-authtool global ${EXTRALIBS} ${CRYPTO_LIBS})
install(TARGETS stone-authtool DESTINATION bin)

if(WITH_TESTS)
set(stone_dedup_tool_srcs stone_dedup_tool.cc)
add_executable(stone-dedup-tool ${stone_dedup_tool_srcs})
target_link_libraries(stone-dedup-tool
  librados
  global
  cls_cas_client
  cls_cas_internal)
install(TARGETS stone-dedup-tool DESTINATION bin)
endif(WITH_TESTS)

if(WITH_STONEFS)
  add_subdirectory(stonefs)
  add_subdirectory(stonefs_mirror)
endif(WITH_STONEFS)

if(WITH_RBD)
  add_subdirectory(rbd)
  add_subdirectory(rbd_mirror)
  if(LINUX)
    add_subdirectory(rbd_nbd)
  endif()
  if(WIN32)
    add_subdirectory(rbd_wnbd)
  endif()
  if(FREEBSD)
    add_subdirectory(rbd_ggate)
  endif()
endif(WITH_RBD)

if(WITH_SEASTAR)
  add_subdirectory(crimson)
endif()

add_subdirectory(immutable_object_cache)
add_subdirectory(stone-dencoder)
add_subdirectory(erasure-code)
