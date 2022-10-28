if(WITH_LIBSTONESQLITE)
  add_executable(stone_test_libstonesqlite
    main.cc
  )
  target_link_libraries(stone_test_libstonesqlite
    stonesqlite
    librados
    stone-common
    SQLite3::SQLite3
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )
  install(TARGETS stone_test_libstonesqlite DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(WITH_LIBSTONESQLITE)
