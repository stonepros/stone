add_executable(stone_test_cls_lua
  test_cls_lua.cc
)
target_link_libraries(stone_test_cls_lua
  cls_lua_client
  ${LUA_LIBRARIES}
  librados
  global
  ${UNITTEST_LIBS}
  ${EXTRALIBS}
  ${CMAKE_DL_LIBS}
  radostest-cxx)
target_include_directories(stone_test_cls_lua PRIVATE "${LUA_INCLUDE_DIR}")
install(TARGETS
  stone_test_cls_lua
  DESTINATION ${CMAKE_INSTALL_BINDIR})
