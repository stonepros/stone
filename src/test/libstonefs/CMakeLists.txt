if(${WITH_STONEFS})
  add_executable(stone_test_libstonefs
    test.cc
    readdir_r_cb.cc
    caps.cc
    multiclient.cc
    flock.cc
    recordlock.cc
    acl.cc
    main.cc
    deleg.cc
    monconfig.cc
    vxattr.cc
  )
  target_link_libraries(stone_test_libstonefs
    stone-common
    stonefs
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )
  install(TARGETS stone_test_libstonefs
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(stone_test_libstonefs_reclaim
    reclaim.cc
  )
  target_link_libraries(stone_test_libstonefs_reclaim
    stonefs
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )
  install(TARGETS stone_test_libstonefs_reclaim
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(stone_test_libstonefs_lazyio
    lazyio.cc
  )
  target_link_libraries(stone_test_libstonefs_lazyio
    stonefs
    librados
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )   
  install(TARGETS stone_test_libstonefs_lazyio
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(stone_test_libstonefs_access
    test.cc
    access.cc
  )
  target_link_libraries(stone_test_libstonefs_access
    stone-common
    stonefs
    librados
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )
  install(TARGETS stone_test_libstonefs_access
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(${WITH_STONEFS})  

