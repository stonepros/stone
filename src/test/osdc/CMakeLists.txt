add_executable(stone_test_objectcacher_stress
  object_cacher_stress.cc
  FakeWriteback.cc
  MemWriteback.cc
  )
target_link_libraries(stone_test_objectcacher_stress
  osdc
  global
  ${EXTRALIBS}
  ${CMAKE_DL_LIBS}
  )
install(TARGETS stone_test_objectcacher_stress
  DESTINATION ${CMAKE_INSTALL_BINDIR})
