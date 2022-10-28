# stone_test_object_map
add_executable(stone_test_object_map
  test_object_map.cc
  KeyValueDBMemory.cc
  )
add_stone_unittest(stone_test_object_map)
target_link_libraries(stone_test_object_map
  os
  stone-common
  ${UNITTEST_LIBS}
  global
  ${EXTRALIBS}
  ${CMAKE_DL_LIBS}
  )

# stone_test_keyvaluedb_atomicity
add_executable(stone_test_keyvaluedb_atomicity
  test_keyvaluedb_atomicity.cc
  )
target_link_libraries(stone_test_keyvaluedb_atomicity
  os
  stone-common
  ${UNITTEST_LIBS}
  global
  ${EXTRALIBS}
  ${CMAKE_DL_LIBS}
  )

# stone_test_keyvaluedb_iterators
add_executable(stone_test_keyvaluedb_iterators
  test_keyvaluedb_iterators.cc
  KeyValueDBMemory.cc
  )
target_link_libraries(stone_test_keyvaluedb_iterators
  os
  stone-common
  ${UNITTEST_LIBS}
  global
  ${EXTRALIBS}
  ${CMAKE_DL_LIBS}
  )

