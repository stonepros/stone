# Lazy omap stat collection tests

add_executable(stone_test_lazy_omap_stats
  main.cc
  lazy_omap_stats_test.cc)
target_link_libraries(stone_test_lazy_omap_stats
  librados Boost::system Boost::filesystem ${UNITTEST_LIBS})
install(TARGETS
  stone_test_lazy_omap_stats
  DESTINATION ${CMAKE_INSTALL_BINDIR})
