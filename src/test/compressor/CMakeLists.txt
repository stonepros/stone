add_library(stone_example SHARED compressor_plugin_example.cc)
target_link_libraries(stone_example stone-common)

# unittest_compression
add_executable(unittest_compression
  test_compression.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_compression)
target_link_libraries(unittest_compression global GTest::GTest)
add_dependencies(unittest_compression stone_example)
