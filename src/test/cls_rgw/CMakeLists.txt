if(${WITH_RADOSGW})
  add_executable(stone_test_cls_rgw
    test_cls_rgw.cc
    )
  target_link_libraries(stone_test_cls_rgw
    cls_rgw_client
    librados
    global
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${BLKID_LIBRARIES}
    ${CMAKE_DL_LIBS}
    radostest-cxx)
  install(TARGETS
    stone_test_cls_rgw
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(${WITH_RADOSGW})

