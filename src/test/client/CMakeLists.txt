if(${WITH_STONEFS})
  add_executable(stone_test_client
    main.cc
    alternate_name.cc
    )
  target_link_libraries(stone_test_client
    client
    global
    stone-common
    ${UNITTEST_LIBS}
    ${EXTRALIBS}
    ${CMAKE_DL_LIBS}
    )
  install(TARGETS stone_test_client
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(${WITH_STONEFS})
