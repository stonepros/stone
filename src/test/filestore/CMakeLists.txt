#stone_test_filestore
add_executable(stone_test_filestore 
  TestFileStore.cc)
target_link_libraries(stone_test_filestore os global ${UNITTEST_LIBS})


