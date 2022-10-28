# stone_test_cls_numops
add_executable(stone_test_cls_numops 
  test_cls_numops.cc)
target_link_libraries(stone_test_cls_numops 
  librados
  global
  cls_numops_client 
  ${EXTRALIBS}
  ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS}
  radostest-cxx
  ${UNITTEST_LIBS} 
  )
install(TARGETS
  stone_test_cls_numops
  DESTINATION ${CMAKE_INSTALL_BINDIR})
