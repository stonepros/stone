find_package(PkgConfig QUIET REQUIRED)
pkg_check_modules(CAPNG REQUIRED libcap-ng)

set(mount_stone_srcs
  mount.stone.c conf.cc)
add_executable(mount.stone ${mount_stone_srcs}
  $<TARGET_OBJECTS:parse_secret_objs>
  $<TARGET_OBJECTS:common_mountstonefs_objs>)
target_link_libraries(mount.stone keyutils::keyutils ${CAPNG_LIBRARIES} global stone-common)
install(TARGETS mount.stone DESTINATION ${CMAKE_INSTALL_SBINDIR})
