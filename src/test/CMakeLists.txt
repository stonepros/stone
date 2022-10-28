include(AddStoneTest)

set(UNITTEST_LIBS GMock::Main GMock::GMock GTest::GTest ${CMAKE_THREAD_LIBS_INIT} 
    ${GSSAPI_LIBRARIES} ${OPENLDAP_LIBRARIES} ${CMAKE_DL_LIBS})

if(WIN32)
  # Needed by Boost.
  list(APPEND UNITTEST_LIBS ws2_32)
endif()

add_library(unit-main OBJECT unit.cc)
target_include_directories(unit-main PRIVATE
  $<TARGET_PROPERTY:GTest::GTest,INTERFACE_INCLUDE_DIRECTORIES>)

add_subdirectory(common)
add_subdirectory(compressor)
add_subdirectory(crush)
add_subdirectory(direct_messenger)
add_subdirectory(encoding)
add_subdirectory(librados)
add_subdirectory(librados_test_stub)
if(WITH_LIBRADOSSTRIPER)
  add_subdirectory(libradosstriper)
endif()
if(WITH_RBD AND NOT WIN32)
  # librbd tests require libcls*, which in turn require libos and libosd, which
  # haven't been ported to Windows yet.
  add_subdirectory(librbd)
endif(WITH_RBD AND NOT WIN32)
if (WITH_STONEFS)
  add_subdirectory(mds)
endif()
add_subdirectory(pybind)

# Not available on Windows for the time being.
if(NOT WIN32)
  # libcls_* dependencies cascade to osd, kv and other libs that are not
  # available on Windows yet.
  add_subdirectory(cls_hello)
  add_subdirectory(cls_fifo)
  add_subdirectory(cls_cas)
  add_subdirectory(cls_lock)
  add_subdirectory(cls_log)
  add_subdirectory(cls_numops)
  add_subdirectory(cls_sdk)
  if(WITH_RBD)
    add_subdirectory(cls_journal)
    add_subdirectory(cls_rbd)
  endif(WITH_RBD)
  add_subdirectory(cls_refcount)
  add_subdirectory(cls_rgw)
  add_subdirectory(cls_version)
  add_subdirectory(cls_lua)
  add_subdirectory(cls_rgw_gc)
  add_subdirectory(cls_queue)
  add_subdirectory(cls_2pc_queue)
  add_subdirectory(cls_cmpomap)
  add_subdirectory(journal)

  add_subdirectory(erasure-code)
  add_subdirectory(filestore)
  add_subdirectory(fs)
  add_subdirectory(libstonefs)
  add_subdirectory(libstonesqlite)
  add_subdirectory(client)
  add_subdirectory(mon)
  add_subdirectory(mgr)
  add_subdirectory(msgr)
  add_subdirectory(neorados)
  add_subdirectory(objectstore)
  add_subdirectory(ObjectMap)
  add_subdirectory(os)
  add_subdirectory(osd)
  add_subdirectory(osdc)
  add_subdirectory(immutable_object_cache)
endif()

if(WITH_RADOSGW)
  set(rgw_libs rgw_a)
  if(WITH_RADOSGW_AMQP_ENDPOINT)
    list(APPEND rgw_libs amqp_mock)
  endif()
  if(WITH_RADOSGW_KAFKA_ENDPOINT)
    list(APPEND rgw_libs kafka_stub)
  endif()
  add_subdirectory(rgw)
endif(WITH_RADOSGW)
if(WITH_RBD AND NOT WIN32)
add_subdirectory(rbd_mirror)
endif(WITH_RBD AND NOT WIN32)
if(WITH_SEASTAR)
  add_subdirectory(crimson)
endif()
add_subdirectory(system)
if(WITH_FIO)
  add_subdirectory(fio)
endif()
add_subdirectory(lazy-omap-stats)

# test_timers
add_executable(stone_test_timers
  TestTimers.cc
  )
target_link_libraries(stone_test_timers global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

# test_signal_handlers
add_executable(stone_test_signal_handlers
  TestSignalHandlers.cc
  )
target_link_libraries(stone_test_signal_handlers global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

# test_rewrite_latency
add_executable(stone_test_rewrite_latency
  test_rewrite_latency.cc
  )
target_link_libraries(stone_test_rewrite_latency stone-common)

# test_crypt
add_executable(test_crypto
  testcrypto.cc)
target_link_libraries(test_crypto
  stone-common)

add_executable(test_build_libcommon buildtest_skeleton.cc)
target_link_libraries(test_build_libcommon stone-common pthread ${CRYPTO_LIBS} ${EXTRALIBS})

if(WITH_RADOSGW)
  add_executable(test_build_librgw buildtest_skeleton.cc)
  target_link_libraries(test_build_librgw rgw_a pthread ${CRYPTO_LIBS} ${EXTRALIBS})
endif(WITH_RADOSGW)

if(WITH_LIBSTONEFS)
  # From src/test/Makefile-client.am: I dont get this one... testing the osdc build but link in libstonefs?
  add_executable(test_build_libstonefs buildtest_skeleton.cc)
  target_link_libraries(test_build_libstonefs stonefs pthread ${CRYPTO_LIBS} ${EXTRALIBS})
endif(WITH_LIBSTONEFS)

add_executable(test_build_librados buildtest_skeleton.cc)
target_link_libraries(test_build_librados librados pthread ${CRYPTO_LIBS} ${EXTRALIBS} stone-common ${BLKID_LIBRARIES})
if(NOT WIN32)
  target_link_libraries(test_build_librados os osdc osd cls_lock_client)
endif()

# bench_log
set(bench_log_srcs
  bench_log.cc
  )
add_executable(stone_bench_log
  ${bench_log_srcs}
  )
target_link_libraries(stone_bench_log global pthread ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})
if(NOT WIN32)
  target_link_libraries(stone_bench_log rt)
endif()

# stone_test_mutate
add_executable(stone_test_mutate
  test_mutate.cc
  )
target_link_libraries(stone_test_mutate global librados ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS})

if(NOT WIN32)
# test_trans
add_executable(test_trans
  test_trans.cc
  )
target_link_libraries(test_trans os global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

## Benchmarks


# stone_omapbench
set(omapbench_srcs
  omap_bench.cc
  )
add_executable(stone_omapbench
  ${omapbench_srcs}
  )
target_link_libraries(stone_omapbench
  librados
  stone-common
  Boost::program_options)

if(WITH_KVS)
  # stone_kvstorebench
  set(kvstorebench_srcs
    kv_store_bench.cc
    ${CMAKE_SOURCE_DIR}/src/key_value_store/kv_flat_btree_async.cc
    )
  add_executable(stone_kvstorebench ${kvstorebench_srcs})
  target_link_libraries(stone_kvstorebench librados global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})
  install(TARGETS stone_kvstorebench DESTINATION bin)
endif(WITH_KVS)

if(NOT WIN32)
  # stone_objectstore_bench
  add_executable(stone_objectstore_bench objectstore_bench.cc)
  target_link_libraries(stone_objectstore_bench os global ${BLKID_LIBRARIES})
endif()

if(${WITH_RADOSGW})
  # test_cors
  set(test_cors_srcs test_cors.cc)
  add_executable(test_cors
    ${test_cors_srcs}
    )
  target_link_libraries(test_cors
    librados
    ${rgw_libs}
    global
    ${BLKID_LIBRARIES}
    ${CURL_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${CMAKE_DL_LIBS} ${UNITTEST_LIBS})

  # stone_test_cls_rgw_meta
  set(test_cls_rgw_meta_srcs test_rgw_admin_meta.cc)
  add_executable(stone_test_cls_rgw_meta
    ${test_cls_rgw_meta_srcs}
    )
  target_link_libraries(stone_test_cls_rgw_meta
    librados
    ${rgw_libs}
    global
    cls_version_client
    cls_log_client
    cls_refcount_client
    cls_rgw_client
    cls_user_client
    cls_lock_client
    ${BLKID_LIBRARIES}
    ${CURL_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${CMAKE_DL_LIBS} ${UNITTEST_LIBS} ${CRYPTO_LIBS})

  install(TARGETS
    stone_test_cls_rgw_meta
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  # stone_test_cls_rgw_log
  set(stone_test_cls_rgw_log_srcs
    test_rgw_admin_log.cc
    )
  add_executable(stone_test_cls_rgw_log
    ${stone_test_cls_rgw_log_srcs}
    )
  target_link_libraries(stone_test_cls_rgw_log
    librados
    ${rgw_libs}
    global
    cls_version_client
    cls_log_client
    cls_refcount_client
    cls_rgw_client
    cls_user_client
    cls_lock_client
    ${BLKID_LIBRARIES}
    ${CURL_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CRYPTO_LIBS}
    )

# stone_test_librgw_file (nfs-like RGW interface)
add_executable(stone_test_librgw_file
  librgw_file.cc
  )
if(WITH_RADOSGW_FCGI_FRONTEND)
target_include_directories(stone_test_librgw_file SYSTEM PRIVATE ${FCGI_INCLUDE_DIR})
endif(WITH_RADOSGW_FCGI_FRONTEND)
target_link_libraries(stone_test_librgw_file
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS stone_test_librgw_file DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_cd (just the rgw_file create-delete bucket ops)
add_executable(stone_test_librgw_file_cd
  librgw_file_cd.cc
  )
target_link_libraries(stone_test_librgw_file_cd
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS stone_test_librgw_file_cd DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_gp (just the rgw_file get-put bucket ops)
add_executable(stone_test_librgw_file_gp
  librgw_file_gp.cc
  )
target_link_libraries(stone_test_librgw_file_gp
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS stone_test_librgw_file_gp DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_nfsns (nfs namespace tests)
add_executable(stone_test_librgw_file_nfsns
  librgw_file_nfsns.cc
  )
target_include_directories(stone_test_librgw_file_nfsns SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/src/rgw")
if(WITH_RADOSGW_FCGI_FRONTEND)
target_include_directories(stone_test_librgw_file_nfsns SYSTEM PRIVATE ${FCGI_INCLUDE_DIR})
endif(WITH_RADOSGW_FCGI_FRONTEND)
target_link_libraries(stone_test_librgw_file_nfsns
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
  target_link_libraries(stone_test_librgw_file_nfsns spawn)
install(TARGETS stone_test_librgw_file_nfsns DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_aw (nfs write transaction [atomic write] tests)
add_executable(stone_test_librgw_file_aw
  librgw_file_aw.cc
  )
target_link_libraries(stone_test_librgw_file_aw
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS stone_test_librgw_file_aw DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_marker (READDIR with string and uint64 offsets)
add_executable(stone_test_librgw_file_marker
  librgw_file_marker.cc
  )
target_include_directories(stone_test_librgw_file_marker SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/src/rgw")
target_link_libraries(stone_test_librgw_file_marker
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
  target_link_libraries(stone_test_librgw_file_marker spawn)
install(TARGETS stone_test_librgw_file_marker DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_librgw_file_xattr (attribute ops)
add_executable(stone_test_librgw_file_xattr
  librgw_file_xattr.cc
  )
target_include_directories(stone_test_librgw_file_xattr SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/src/rgw")
target_link_libraries(stone_test_librgw_file_xattr
  rgw
  librados
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )
target_link_libraries(stone_test_librgw_file_xattr spawn)

# stone_test_rgw_token
add_executable(stone_test_rgw_token
  test_rgw_token.cc
  )
target_link_libraries(stone_test_rgw_token
  rgw
  stone-common
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  )

# librgw_file_gp (just the rgw_file get-put bucket ops)
add_executable(test_rgw_ldap
  ${CMAKE_SOURCE_DIR}/src/rgw/rgw_ldap.cc
  test_rgw_ldap.cc
  )
target_link_libraries(test_rgw_ldap
  librados
  stone-common
  ${OPENLDAP_LIBRARIES}
  ${UNITTEST_LIBS}
  )
endif(${WITH_RADOSGW})

# stone_multi_stress_watch
add_executable(stone_multi_stress_watch
  multi_stress_watch.cc
  )
target_link_libraries(stone_multi_stress_watch librados global radostest-cxx)

install(TARGETS
  stone_bench_log
  stone_multi_stress_watch
  stone_omapbench
  DESTINATION bin)

if(NOT WIN32)
#stone_perf_local
add_executable(stone_perf_local
  perf_local.cc
  perf_helper.cc)
if(HAVE_SSE)
  set(PERF_LOCAL_FLAGS ${SSE3_FLAGS})
endif(HAVE_SSE)
if(HAVE_NEON)
  set(PERF_LOCAL_FLAGS ${ARM_NEON_FLAGS})
endif(HAVE_NEON)
if(PERF_LOCAL_FLAGS)
  set_target_properties(stone_perf_local PROPERTIES COMPILE_FLAGS
    ${PERF_LOCAL_FLAGS})
endif()
target_link_libraries(stone_perf_local global ${UNITTEST_LIBS})

install(TARGETS
  stone_objectstore_bench
  stone_perf_local
  DESTINATION bin)

# stone_test_filejournal
add_executable(stone_test_filejournal
  test_filejournal.cc
  )
target_link_libraries(stone_test_filejournal
  os
  stone-common
  ${UNITTEST_LIBS}
  global
  ${EXTRALIBS}
  ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS
  stone_test_filejournal
  DESTINATION ${CMAKE_INSTALL_BINDIR})

# stone_test_keys
add_executable(stone_test_keys
  testkeys.cc
  )
target_link_libraries(stone_test_keys mon global ${CMAKE_DL_LIBS})

# stone_test_snap_mapper
add_executable(stone_test_snap_mapper
  test_snap_mapper.cc
  $<TARGET_OBJECTS:unit-main>
  )
target_link_libraries(stone_test_snap_mapper osd global ${BLKID_LIBRARIES} ${UNITTEST_LIBS})
endif(NOT WIN32)

add_executable(stone_test_stress_watch
  test_stress_watch.cc
  )
target_link_libraries(stone_test_stress_watch
  librados
  ${UNITTEST_LIBS}
  radostest-cxx
  ${EXTRALIBS}
  ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS}
  )
install(TARGETS
  stone_test_stress_watch
  DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WITH_FUSE)
  add_executable(stone_test_cfuse_cache_invalidate
    test_cfuse_cache_invalidate.cc
    )
  target_link_libraries(stone_test_cfuse_cache_invalidate
    stone-common
  )
endif(WITH_FUSE)

if(${WITH_STONEFS})
  add_executable(test_c_headers
    test_c_headers.c
    )
  target_link_libraries(test_c_headers
    librados
    stonefs)
endif(${WITH_STONEFS})

if(HAVE_BLKID OR FREEBSD)
  add_executable(stone_test_get_blkdev_props
    test_get_blkdev_props.cc
    )
  target_link_libraries(stone_test_get_blkdev_props
    stone-common
    pthread
    ${EXTRALIBS}
    ${BLKID_LIBRARIES}
    ${CMAKE_DL_LIBS}
    )
endif(HAVE_BLKID OR FREEBSD)

# stone_test_admin_socket_output

if(StdFilesystem_FOUND)
  add_executable(stone_test_admin_socket_output
    test_admin_socket_output.cc
    admin_socket_output.cc
    admin_socket_output_tests.cc)
  target_link_libraries(stone_test_admin_socket_output
    stone-common StdFilesystem::filesystem)
  install(TARGETS
    stone_test_admin_socket_output
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

#make check starts here

#following dependencies are run inside make check unit tests
add_dependencies(tests
  stone-authtool
  stone-conf
  rados
  stone_snappy)
if(NOT WIN32)
  # Not currently supported on Windows
  add_dependencies(tests
  stone-mon
  get_command_descriptions
  stone-dencoder
  stone-objectstore-tool
  stone-kvstore-tool
  stone-monstore-tool
  stone-osd
  osdmaptool
  stone_example
  stone_snappy
  cls_lock
  stone_test_objectstore
  stone_erasure_code_non_regression
  cython_modules
  crushtool
  monmaptool)

if (WITH_STONEFS)
  add_dependencies(tests stone-mds)
endif()
if(WITH_MGR)
  add_dependencies(tests stone-mgr)
endif()
if(WITH_RBD)
  add_dependencies(tests unittest_librbd rbd)
  if(FREEBSD)
    add_dependencies(tests rbd-ggate)
  endif(FREEBSD)
endif(WITH_RBD)
if(WITH_RADOSGW)
  add_dependencies(tests radosgwd radosgw-admin)
endif()
#add dependency from fio just to ensure the plugin build isn't failing
if(WITH_FIO)
  add_dependencies(tests fio_stone_objectstore)
endif()

if(WITH_RBD)
  # Run rbd-unit-tests separate so they an run in parallel
  # For values see: src/include/rbd/features.h
  add_stone_test(run-rbd-unit-tests-N.sh   ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh N)
  add_stone_test(run-rbd-unit-tests-0.sh   ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh 0)
  add_stone_test(run-rbd-unit-tests-1.sh   ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh 1)
  add_stone_test(run-rbd-unit-tests-61.sh  ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh 61)
  add_stone_test(run-rbd-unit-tests-109.sh ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh 109)
  add_stone_test(run-rbd-unit-tests-127.sh ${CMAKE_CURRENT_SOURCE_DIR}/run-rbd-unit-tests.sh 127)
  if(FREEBSD)
    add_stone_test(rbd-ggate.sh ${CMAKE_CURRENT_SOURCE_DIR}/rbd-ggate.sh)
  endif(FREEBSD)
endif(WITH_RBD)

endif(NOT WIN32)

add_stone_test(run-cli-tests ${CMAKE_CURRENT_SOURCE_DIR}/run-cli-tests)

# flaky, see https://tracker.stone.com/issues/44243
#add_stone_test(test_objectstore_memstore.sh ${CMAKE_CURRENT_SOURCE_DIR}/test_objectstore_memstore.sh)

# flaky
#add_stone_test(test_pidfile.sh ${CMAKE_CURRENT_SOURCE_DIR}/test_pidfile.sh)

add_stone_test(smoke.sh ${CMAKE_CURRENT_SOURCE_DIR}/smoke.sh)

set_property(
  TEST ${tox_tests}
  PROPERTY ENVIRONMENT ${env_vars_for_tox_tests})

# unittest_admin_socket
add_executable(unittest_admin_socket
  admin_socket.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_admin_socket)
target_link_libraries(unittest_admin_socket global)

# unittest_encoding
add_executable(unittest_encoding
  encoding.cc
  )
add_stone_unittest(unittest_encoding)
target_link_libraries(unittest_encoding stone-common)

# unittest_addrs
add_executable(unittest_addrs
  test_addrs.cc
  )
add_stone_unittest(unittest_addrs)
target_link_libraries(unittest_addrs stone-common)

# unittest_auth
add_executable(unittest_auth
  test_auth.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_auth)
target_link_libraries(unittest_auth global)

# unittest_workqueue
add_executable(unittest_workqueue
  test_workqueue.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_workqueue)
target_link_libraries(unittest_workqueue global)

# unittest_striper
if(WITH_LIBRADOSSTRIPER)
  add_executable(unittest_striper
    test_striper.cc
    $<TARGET_OBJECTS:unit-main>
    )
  add_stone_unittest(unittest_striper)
  target_link_libraries(unittest_striper global ${BLKID_LIBRARIES})
endif()

# unittest_str_list
add_executable(unittest_str_list
  test_str_list.cc
  )
add_stone_unittest(unittest_str_list)
target_link_libraries(unittest_str_list global)

# This test's usage of templates generates more sections than a PE file can
# contain.
if(NOT MINGW)
# unittest_log
add_executable(unittest_log
  ${CMAKE_SOURCE_DIR}/src/log/test.cc
  )
add_stone_unittest(unittest_log)
target_link_libraries(unittest_log global)
endif(NOT MINGW)

# unittest_base64
add_executable(unittest_base64
  base64.cc
  )
add_stone_unittest(unittest_base64)
target_link_libraries(unittest_base64 global)

# unittest_stone_argparse
add_executable(unittest_stone_argparse
  stone_argparse.cc
  )
add_stone_unittest(unittest_stone_argparse)
target_link_libraries(unittest_stone_argparse global)

# unittest_stone_compatset
add_executable(unittest_stone_compatset
  stone_compatset.cc
  )
add_stone_unittest(unittest_stone_compatset)
target_link_libraries(unittest_stone_compatset global)

# unittest_gather
add_executable(unittest_gather
  gather.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_gather)
target_link_libraries(unittest_gather global)

# unittest_run_cmd
add_executable(unittest_run_cmd
  run_cmd.cc
  )
add_stone_unittest(unittest_run_cmd)
target_link_libraries(unittest_run_cmd global)

if(NOT WIN32)
# signals
add_executable(unittest_signals
  signals.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_signals)
target_link_libraries(unittest_signals global)
endif()

# unittest_simple_spin
add_executable(unittest_simple_spin
  simple_spin.cc
  )
add_stone_unittest(unittest_simple_spin)
target_link_libraries(unittest_simple_spin global)

# unittest_bufferlist
add_executable(unittest_bufferlist
  bufferlist.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_bufferlist)
target_link_libraries(unittest_bufferlist global)

# compiletest_cxx11_client
add_executable(compiletest_cxx11_client
  cxx11_client.cc
  )
if(CMAKE_VERSION VERSION_LESS "3.8")
  # this is ugly as we'll end with -std=c++11 overriding the previous -std=c++17
  # I would love to have a better way for old Cmakes
  set_target_properties(compiletest_cxx11_client
    PROPERTIES COMPILE_FLAGS "-std=c++11 -Werror -pedantic"
    )
else()
  set_target_properties(compiletest_cxx11_client
    PROPERTIES COMPILE_FLAGS "-Werror -pedantic"
    CMAKE_CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    )
endif()
add_stone_test(compiletest_cxx11_client
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/compiletest_cxx11_client"
  )
target_link_libraries(compiletest_cxx11_client global)

# unittest_xlist
add_executable(unittest_xlist
  test_xlist.cc
  )
add_stone_unittest(unittest_xlist)
target_link_libraries(unittest_xlist stone-common)

# unittest_arch
add_executable(unittest_arch
  test_arch.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_arch)
target_link_libraries(unittest_arch global)

# unittest_denc
add_executable(unittest_denc
  test_denc.cc
  )
add_stone_unittest(unittest_denc)
target_link_libraries(unittest_denc global)

# unittest_mempool
add_executable(unittest_mempool
  test_mempool.cc
  )
add_stone_unittest(unittest_mempool)
target_link_libraries(unittest_mempool global)

# unittest_features
add_executable(unittest_features
  test_features.cc
  )
add_stone_unittest(unittest_features)
target_link_libraries(unittest_features global)

# unittest_crypto
add_executable(unittest_crypto
  crypto.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_crypto)
target_link_libraries(unittest_crypto global)

# unittest_crypto_init
add_executable(unittest_crypto_init
  crypto_init.cc
  )
add_stone_unittest(unittest_crypto_init)
target_link_libraries(unittest_crypto_init global)

# unittest_perf_counters
add_executable(unittest_perf_counters
  perf_counters.cc
  )
add_stone_unittest(unittest_perf_counters)
target_link_libraries(unittest_perf_counters global)

# unittest_stone_crypto
add_executable(unittest_stone_crypto
  stone_crypto.cc)
add_stone_unittest(unittest_stone_crypto)
target_link_libraries(unittest_stone_crypto global)

# unittest_utf8
add_executable(unittest_utf8
  utf8.cc)
add_stone_unittest(unittest_utf8)
target_link_libraries(unittest_utf8 global)

# unittest_mime
add_executable(unittest_mime
  mime.cc)
add_stone_unittest(unittest_mime)
target_link_libraries(unittest_mime stone-common)

# unittest_escape
add_executable(unittest_escape
  escape.cc)
add_stone_unittest(unittest_escape)
target_link_libraries(unittest_escape stone-common)

# unittest_strtol
add_executable(unittest_strtol
  strtol.cc)
add_stone_unittest(unittest_strtol)
target_link_libraries(unittest_strtol stone-common)

# unittest_confutils
add_executable(unittest_confutils
  confutils.cc)
add_stone_unittest(unittest_confutils)
target_link_libraries(unittest_confutils stone-common)

# unittest_heartbeatmap
add_executable(unittest_heartbeatmap
  heartbeat_map.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_heartbeatmap)
target_link_libraries(unittest_heartbeatmap global stone-common)

if(${WITH_RADOSGW})
  # unittest_formatter
  add_executable(unittest_formatter
    formatter.cc
    $<TARGET_OBJECTS:unit-main>
    )
  add_stone_unittest(unittest_formatter)
  target_link_libraries(unittest_formatter global)
endif(${WITH_RADOSGW})

# unittest_daemon_config
add_executable(unittest_daemon_config
  daemon_config.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_daemon_config)
target_link_libraries(unittest_daemon_config
  stone-common
  global
  ${BLKID_LIBRARIES}
  ${EXTRALIBS}
  )

if(WITH_LIBSTONEFS)
# unittest_libstonefs_config
add_executable(unittest_libstonefs_config
  libstonefs_config.cc
  )
add_stone_unittest(unittest_libstonefs_config)
target_link_libraries(unittest_libstonefs_config stonefs)
endif(WITH_LIBSTONEFS)

if(WITH_RBD)
# unittest_rbd_replay
add_executable(unittest_rbd_replay
  test_rbd_replay.cc)
add_stone_unittest(unittest_rbd_replay)
target_link_libraries(unittest_rbd_replay
  librbd
  librados
  global
  stone-common
  rbd_replay
  rbd_replay_ios
  ${BLKID_LIBRARIES}
  )
endif(WITH_RBD)

# unittest_ipaddr
add_executable(unittest_ipaddr
  test_ipaddr.cc)
add_stone_unittest(unittest_ipaddr)
target_link_libraries(unittest_ipaddr global GTest::Main)

# unittest_utime
add_executable(unittest_utime
  test_utime.cc)
add_stone_unittest(unittest_utime)
target_link_libraries(unittest_utime stone-common)

# unittest_texttable
add_executable(unittest_texttable
  test_texttable.cc
  $<TARGET_OBJECTS:common_texttable_obj>)
add_stone_unittest(unittest_texttable)
target_link_libraries(unittest_texttable stone-common)

if(NOT WIN32)
# unittest_on_exit
add_executable(unittest_on_exit
  on_exit.cc)
add_stone_unittest(unittest_on_exit)
target_link_libraries(unittest_on_exit stone-common)
endif()

# unittest_subprocess
add_executable(unittest_subprocess
  test_subprocess.cc)
add_stone_unittest(unittest_subprocess)
target_link_libraries(unittest_subprocess stone-common)

# unittest_pageset
add_executable(unittest_pageset test_pageset.cc)
add_stone_unittest(unittest_pageset)
target_link_libraries(unittest_pageset stone-common)

add_executable(unittest_random_string test_random_string.cc $<TARGET_OBJECTS:unit-main>)
add_stone_unittest(unittest_random_string)
target_link_libraries(unittest_random_string global)

# unittest_any_
add_executable(unittest_any test_any.cc)
add_stone_unittest(unittest_any)

# unittest_weighted_shuffle
add_executable(unittest_weighted_shuffle test_weighted_shuffle.cc)
add_stone_unittest(unittest_weighted_shuffle)

#make check ends here
