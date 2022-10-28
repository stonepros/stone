if(${WITH_STONEFS})
  # unittest_mds_types
  add_executable(unittest_mds_types
    mds_types.cc
    )
  add_stone_unittest(unittest_mds_types)
  target_link_libraries(unittest_mds_types global)

  add_executable(stone_test_trim_caps
    test_trim_caps.cc
  )
  target_link_libraries(stone_test_trim_caps stone-common stonefs)
  install(TARGETS stone_test_trim_caps DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(stone_test_ino_release_cb
    test_ino_release_cb.cc
  )
  target_link_libraries(stone_test_ino_release_cb stone-common stonefs)
  install(TARGETS stone_test_ino_release_cb DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(${WITH_STONEFS})
