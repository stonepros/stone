set(stone_immutable_object_cache_files
  ObjectCacheStore.cc
  CacheController.cc
  CacheServer.cc
  CacheClient.cc
  CacheSession.cc
  SimplePolicy.cc
  Types.cc
  )
add_library(stone_immutable_object_cache_lib STATIC ${stone_immutable_object_cache_files})

add_executable(stone-immutable-object-cache
  main.cc)
target_link_libraries(stone-immutable-object-cache
  stone_immutable_object_cache_lib
  librados
  StdFilesystem::filesystem
  global)
install(TARGETS stone-immutable-object-cache DESTINATION bin)
