# stone_test_async_driver
add_executable(stone_test_async_driver
  test_async_driver.cc
  $<TARGET_OBJECTS:unit-main>
  )
target_link_libraries(stone_test_async_driver os global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS} ${UNITTEST_LIBS})

# stone_test_msgr
add_executable(stone_test_msgr
  test_msgr.cc
  )
target_link_libraries(stone_test_msgr os global ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS} ${UNITTEST_LIBS})

# stone_test_async_networkstack
add_executable(stone_test_async_networkstack
  test_async_networkstack.cc
  $<TARGET_OBJECTS:unit-main>
  )
target_link_libraries(stone_test_async_networkstack global ${CRYPTO_LIBS} ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS} ${UNITTEST_LIBS})

#stone_perf_msgr_server
add_executable(stone_perf_msgr_server perf_msgr_server.cc)
target_link_libraries(stone_perf_msgr_server os global ${UNITTEST_LIBS})

#stone_perf_msgr_client
add_executable(stone_perf_msgr_client perf_msgr_client.cc)
target_link_libraries(stone_perf_msgr_client os global ${UNITTEST_LIBS})

# unitttest_frames_v2
add_executable(unittest_frames_v2 test_frames_v2.cc)
add_stone_unittest(unittest_frames_v2)
target_link_libraries(unittest_frames_v2 os global ${UNITTEST_LIBS})

# test_userspace_event
if(HAVE_DPDK)
  add_executable(stone_test_userspace_event
    test_userspace_event.cc
    $<TARGET_OBJECTS:unit-main>)
  target_link_libraries(stone_test_userspace_event
    global
    ${CMAKE_DL_LIBS}
    ${UNITTEST_LIBS})
endif(HAVE_DPDK)

install(TARGETS
  stone_test_async_driver
  stone_test_msgr
  stone_test_async_networkstack
  stone_perf_msgr_server
  stone_perf_msgr_client
  DESTINATION ${CMAKE_INSTALL_BINDIR})
