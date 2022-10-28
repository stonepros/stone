
add_executable(unittest_stone_immutable_obj_cache
  test_main.cc
  test_SimplePolicy.cc
  test_DomainSocket.cc
  test_multi_session.cc
  test_object_store.cc
  test_message.cc
  )
add_stone_unittest(unittest_stone_immutable_obj_cache)


target_link_libraries(unittest_stone_immutable_obj_cache
  stone_immutable_object_cache_lib
  rados_test_stub
  librados
  global
  radostest-cxx
  StdFilesystem::filesystem
  GTest::GTest
  )


add_executable(stone_test_immutable_obj_cache
  test_main.cc
  )

target_link_libraries(stone_test_immutable_obj_cache
  librados
  radostest-cxx
  ${UNITTEST_LIBS}
  )


install(TARGETS
  stone_test_immutable_obj_cache
  DESTINATION ${CMAKE_INSTALL_BINDIR})
