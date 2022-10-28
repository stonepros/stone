if(NOT WIN32)
# get_command_descriptions
# libmon not currently available on Windows.
add_executable(get_command_descriptions
  get_command_descriptions.cc
  $<TARGET_OBJECTS:common_texttable_obj>
  )
target_link_libraries(get_command_descriptions
  mon
  global
  ${LEVELDB_LIBRARIES}
  ${EXTRALIBS}
  ${BLKID_LIBRARIES}
  ${CMAKE_DL_LIBS}
  )
endif(NOT WIN32)

# Though FreeBSD has blkdev support, the unittests' mocks only work in Linux
if(HAVE_BLKID AND LINUX)
  # unittest_blkdev
  add_executable(unittest_blkdev
    test_blkdev.cc)
  add_stone_unittest(unittest_blkdev)
  target_link_libraries(unittest_blkdev global ${BLKID_LIBRARIES})
endif()

# unittest_lockdep
add_executable(unittest_lockdep
  test_lockdep.cc)
add_stone_unittest(unittest_lockdep)
target_link_libraries(unittest_lockdep stone-common)

# unittest_counter
add_executable(unittest_counter
  test_counter.cc)
add_stone_unittest(unittest_counter)
target_link_libraries(unittest_counter stone-common)

# FreeBSD only has shims to support NUMA, no functional code.
if(LINUX)
# unittest_numa
add_executable(unittest_numa
  test_numa.cc
  )
add_stone_unittest(unittest_numa)
target_link_libraries(unittest_numa stone-common)
endif()

# unittest_bloom_filter
add_executable(unittest_bloom_filter
  test_bloom_filter.cc
  )
add_stone_unittest(unittest_bloom_filter)
target_link_libraries(unittest_bloom_filter stone-common)

# unittest_histogram
add_executable(unittest_histogram
  histogram.cc
  )
add_stone_unittest(unittest_histogram)
target_link_libraries(unittest_histogram stone-common)

# unittest_prioritized_queue
add_executable(unittest_prioritized_queue
  test_prioritized_queue.cc
  )
target_link_libraries(unittest_prioritized_queue stone-common)
add_stone_unittest(unittest_prioritized_queue)

if(NOT WIN32)
# unittest_mclock_priority_queue
add_executable(unittest_mclock_priority_queue
  test_mclock_priority_queue.cc
  )
add_stone_unittest(unittest_mclock_priority_queue)
target_link_libraries(unittest_mclock_priority_queue
  stone-common
  dmclock::dmclock)
endif(NOT WIN32)

# unittest_str_map
add_executable(unittest_str_map
  test_str_map.cc
  )
add_stone_unittest(unittest_str_map)
target_link_libraries(unittest_str_map stone-common)

# unittest_json_formattable
add_executable(unittest_json_formattable
  test_json_formattable.cc
  )
add_stone_unittest(unittest_json_formattable ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unittest_json_formattable)
# add_dependencies(unittest_json_formattable stone-common)
target_link_libraries(unittest_json_formattable stone-common global ${BLKID_LIBRARIES})

# unittest_json_formatter
add_executable(unittest_json_formatter
  test_json_formatter.cc
  )
add_stone_unittest(unittest_json_formatter ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unittest_json_formatter)
# add_dependencies(unittest_json_formatter stone-common)
target_link_libraries(unittest_json_formatter stone-common global ${BLKID_LIBRARIES})

# unittest_sharedptr_registry
add_executable(unittest_sharedptr_registry
  test_sharedptr_registry.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_sharedptr_registry)
target_link_libraries(unittest_sharedptr_registry global)

# unittest_shared_cache
add_executable(unittest_shared_cache
  test_shared_cache.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_shared_cache)
target_link_libraries(unittest_shared_cache global)

# unittest_sloppy_crc_map
add_executable(unittest_sloppy_crc_map
  test_sloppy_crc_map.cc
  )
add_stone_unittest(unittest_sloppy_crc_map)
target_link_libraries(unittest_sloppy_crc_map global)

# unittest_time
add_executable(unittest_time
  test_time.cc
  ${CMAKE_SOURCE_DIR}/src/common/stone_time.cc
  )
add_stone_unittest(unittest_time)
target_link_libraries(unittest_time stone-common)

# unittest_util
add_executable(unittest_util
  test_util.cc
  ${CMAKE_SOURCE_DIR}/src/common/util.cc
  )
add_stone_unittest(unittest_util)
target_link_libraries(unittest_util global StdFilesystem::filesystem)

# unittest_random
add_executable(unittest_random
  test_random.cc
  )
add_stone_unittest(unittest_random)
target_link_libraries(unittest_random Boost::random)

# unittest_throttle
add_executable(unittest_throttle
  Throttle.cc
  $<TARGET_OBJECTS:unit-main>
  )
add_stone_unittest(unittest_throttle PARALLEL)
target_link_libraries(unittest_throttle global) 

# unittest_lru
add_executable(unittest_lru
  test_lru.cc
  )
add_stone_unittest(unittest_lru)
target_link_libraries(unittest_lru stone-common)

# unittest_intrusive_lru
add_executable(unittest_intrusive_lru
  test_intrusive_lru.cc
  )
add_stone_unittest(unittest_intrusive_lru)
target_link_libraries(unittest_intrusive_lru stone-common)

# unittest_crc32c
add_executable(unittest_crc32c
  test_crc32c.cc
  )
add_stone_unittest(unittest_crc32c)
target_link_libraries(unittest_crc32c stone-common)

# unittest_config
add_executable(unittest_config
  test_config.cc
  test_hostname.cc
  )
add_stone_unittest(unittest_config)
target_link_libraries(unittest_config stone-common)

# unittest_context
add_executable(unittest_context
  test_context.cc
  )
add_stone_unittest(unittest_context)
target_link_libraries(unittest_context stone-common)

# unittest_safe_io
add_executable(unittest_safe_io
  test_safe_io.cc
  )
add_stone_unittest(unittest_safe_io)
target_link_libraries(unittest_safe_io stone-common)

# unittest_url_escape
add_executable(unittest_url_escape
  test_url_escape.cc
  )
add_stone_unittest(unittest_url_escape)
target_link_libraries(unittest_url_escape stone-common)

# unittest_pretty_binary
add_executable(unittest_pretty_binary
  test_pretty_binary.cc
  )
add_stone_unittest(unittest_pretty_binary)
target_link_libraries(unittest_pretty_binary stone-common)

# unittest_readahead
add_executable(unittest_readahead
  Readahead.cc
  )
add_stone_unittest(unittest_readahead)
target_link_libraries(unittest_readahead stone-common)

# unittest_tableformatter
add_executable(unittest_tableformatter
  test_tableformatter.cc
  )
add_stone_unittest(unittest_tableformatter)
target_link_libraries(unittest_tableformatter stone-common)

add_executable(unittest_xmlformatter
    test_xmlformatter.cc
    )
add_stone_unittest(unittest_xmlformatter)
target_link_libraries(unittest_xmlformatter stone-common)

# unittest_bit_vector
add_executable(unittest_bit_vector
  test_bit_vector.cc
  )
add_stone_unittest(unittest_bit_vector)
target_link_libraries(unittest_bit_vector stone-common)

# unittest_interval_map
add_executable(unittest_interval_map
  test_interval_map.cc
)
add_stone_unittest(unittest_interval_map)
target_link_libraries(unittest_interval_map stone-common)

# unittest_interval_set
add_executable(unittest_interval_set
  test_interval_set.cc
)
add_stone_unittest(unittest_interval_set)
target_link_libraries(unittest_interval_set stone-common GTest::Main)

# unittest_weighted_priority_queue
add_executable(unittest_weighted_priority_queue
  test_weighted_priority_queue.cc
  )
target_link_libraries(unittest_weighted_priority_queue stone-common)
add_stone_unittest(unittest_weighted_priority_queue)

# unittest_mutex_debug
add_executable(unittest_mutex_debug
  test_mutex_debug.cc
  )
add_stone_unittest(unittest_mutex_debug)
target_link_libraries(unittest_mutex_debug stone-common)

# unittest_shunique_lock
add_executable(unittest_shunique_lock
  test_shunique_lock.cc
  )
add_stone_unittest(unittest_shunique_lock)
target_link_libraries(unittest_shunique_lock stone-common)

add_executable(unittest_fair_mutex
  test_fair_mutex.cc)
add_stone_unittest(unittest_fair_mutex)
target_link_libraries(unittest_fair_mutex stone-common)

# unittest_perf_histogram
add_executable(unittest_perf_histogram
  test_perf_histogram.cc
  )
add_stone_unittest(unittest_perf_histogram)
target_link_libraries(unittest_perf_histogram stone-common)

# unittest_global_doublefree
if(WITH_STONEFS)
  add_executable(unittest_global_doublefree
    test_global_doublefree.cc
    )
  add_stone_unittest(unittest_global_doublefree)
  target_link_libraries(unittest_global_doublefree stonefs librados)
endif(WITH_STONEFS)

if(NOT WIN32)
add_executable(unittest_dns_resolve
  dns_resolve.cc
  $<TARGET_OBJECTS:unit-main>)
target_link_libraries(unittest_dns_resolve global)
add_stone_unittest(unittest_dns_resolve)
endif()

add_executable(unittest_back_trace
  test_back_trace.cc)
set_source_files_properties(test_back_trace.cc PROPERTIES
  COMPILE_FLAGS -fno-inline)
add_stone_unittest(unittest_back_trace)
target_link_libraries(unittest_back_trace stone-common)

add_executable(unittest_hostname
    test_hostname.cc)
add_stone_unittest(unittest_hostname)
target_link_libraries(unittest_hostname stone-common)

add_executable(unittest_iso_8601
    test_iso_8601.cc)
add_stone_unittest(unittest_iso_8601)
target_link_libraries(unittest_iso_8601 stone-common)

add_executable(unittest_convenience test_convenience.cc)
add_stone_unittest(unittest_convenience)

add_executable(unittest_bounded_key_counter
  test_bounded_key_counter.cc
  $<TARGET_OBJECTS:unit-main>)
target_link_libraries(unittest_bounded_key_counter global)
add_stone_unittest(unittest_bounded_key_counter)

add_executable(unittest_split test_split.cc)
add_stone_unittest(unittest_split)

add_executable(unittest_static_ptr test_static_ptr.cc)
add_stone_unittest(unittest_static_ptr)

add_executable(unittest_hobject test_hobject.cc
  $<TARGET_OBJECTS:unit-main>)
target_link_libraries(unittest_hobject global stone-common)
add_stone_unittest(unittest_hobject)

add_executable(unittest_async_completion test_async_completion.cc)
add_stone_unittest(unittest_async_completion)
target_link_libraries(unittest_async_completion stone-common Boost::system)

add_executable(unittest_async_shared_mutex test_async_shared_mutex.cc)
add_stone_unittest(unittest_async_shared_mutex)
target_link_libraries(unittest_async_shared_mutex stone-common Boost::system)

add_executable(unittest_cdc test_cdc.cc
  $<TARGET_OBJECTS:unit-main>)
target_link_libraries(unittest_cdc global stone-common)
add_stone_unittest(unittest_cdc)

add_executable(unittest_stone_timer test_stone_timer.cc)
add_stone_unittest(unittest_stone_timer)


add_executable(unittest_blocked_completion test_blocked_completion.cc)
add_stone_unittest(unittest_blocked_completion)
target_link_libraries(unittest_blocked_completion Boost::system GTest::GTest)

add_executable(unittest_allocate_unique test_allocate_unique.cc)
add_stone_unittest(unittest_allocate_unique)
