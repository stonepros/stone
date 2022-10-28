# snappy

set(snappy_sources
  CompressionPluginSnappy.cc
)

add_library(stone_snappy SHARED ${snappy_sources})
target_link_libraries(stone_snappy
  PRIVATE snappy::snappy compressor $<$<PLATFORM_ID:Windows>:stone-common>)
set_target_properties(stone_snappy PROPERTIES
  VERSION 2.0.0
  SOVERSION 2
  INSTALL_RPATH "")
install(TARGETS stone_snappy DESTINATION ${compressor_plugin_dir})
