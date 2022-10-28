
set(compressor_srcs
  Compressor.cc)
if (HAVE_QATZIP)
  list(APPEND compressor_srcs QatAccel.cc)
endif()
add_library(compressor_objs OBJECT ${compressor_srcs})

## compressor plugins

set(compressor_plugin_dir ${STONE_INSTALL_PKGLIBDIR}/compressor)

add_subdirectory(snappy)
add_subdirectory(zlib)
add_subdirectory(zstd)

if(HAVE_LZ4)
  add_subdirectory(lz4)
endif()

if(HAVE_BROTLI)
  add_subdirectory(brotli)
endif()

add_library(compressor STATIC ${compressor_srcs})
if(HAVE_QATZIP)
  target_link_libraries(compressor PRIVATE ${QATZIP_LIBRARIES})
endif()

set(stone_compressor_libs
    stone_snappy
    stone_zlib
    stone_zstd)

if(HAVE_LZ4)
  list(APPEND stone_compressor_libs stone_lz4)
endif()

if(HAVE_BROTLI)
  list(APPEND stone_compressor_libs stone_brotli)
endif()

add_custom_target(compressor_plugins DEPENDS
    ${stone_compressor_libs})
