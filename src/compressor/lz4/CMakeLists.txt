# lz4

set(lz4_sources
  CompressionPluginLZ4.cc
)

add_library(stone_lz4 SHARED ${lz4_sources})
target_link_libraries(stone_lz4
  PRIVATE LZ4::LZ4 compressor $<$<PLATFORM_ID:Windows>:stone-common>)
set_target_properties(stone_lz4 PROPERTIES
  VERSION 2.0.0
  SOVERSION 2
  INSTALL_RPATH "")
install(TARGETS stone_lz4 DESTINATION ${compressor_plugin_dir})
