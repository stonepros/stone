add_executable(stone_test_cls_cmpomap test_cls_cmpomap.cc)
target_link_libraries(stone_test_cls_cmpomap cls_cmpomap_client librados radostest-cxx ${UNITTEST_LIBS})
install(TARGETS stone_test_cls_cmpomap DESTINATION ${CMAKE_INSTALL_BINDIR})
