add_executable(stone_test_cls_log
  test_cls_log.cc)
target_link_libraries(stone_test_cls_log
  librados
  cls_log_client
  global
  radostest-cxx
  ${UNITTEST_LIBS}
  ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CRYPTO_LIBS}
  ${EXTRALIBS}
  )
install(TARGETS
  stone_test_cls_log
  DESTINATION ${CMAKE_INSTALL_BINDIR})
