# unittest_crush_wrapper
add_executable(unittest_crush_wrapper
  CrushWrapper.cc)
add_stone_unittest(unittest_crush_wrapper)
target_link_libraries(unittest_crush_wrapper stone-common)

# unittest_crush
add_executable(unittest_crush
  crush.cc)
add_stone_unittest(unittest_crush PARALLEL)
target_link_libraries(unittest_crush stone-common)

add_stone_test(crush_weights.sh ${CMAKE_CURRENT_SOURCE_DIR}/crush_weights.sh)
