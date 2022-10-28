# cls_test_cls_rbd
add_executable(stone_test_cls_rbd
  test_cls_rbd.cc
  $<TARGET_OBJECTS:common_texttable_obj>)
target_link_libraries(stone_test_cls_rbd
  cls_rbd_client
  cls_lock_client
  libneorados
  librados
  global
  ${UNITTEST_LIBS}
  ${CMAKE_DL_LIBS}
  ${CRYPTO_LIBS}
  ${EXTRALIBS}
  radostest-cxx)
install(TARGETS
  stone_test_cls_rbd
  DESTINATION ${CMAKE_INSTALL_BINDIR})
