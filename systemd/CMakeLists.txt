set(CMAKE_INSTALL_SYSTEMD_SERVICEDIR "${CMAKE_INSTALL_LIBEXECDIR}/systemd/system"
  CACHE PATH "Location for systemd service files")
set(STONE_SYSTEMD_ENV_DIR "/etc/sysconfig"
  CACHE PATH "Location for systemd service environmental variable settings files")
set(SYSTEMD_ENV_FILE "${STONE_SYSTEMD_ENV_DIR}/stone")
foreach(service
    stone-crash
    stone-fuse@
    stone-mds@
    stone-mgr@
    stone-mon@
    stone-osd@
    stone-radosgw@
    stone-rbd-mirror@
    stone-immutable-object-cache@
    stonefs-mirror@
    rbdmap)
  configure_file(
    ${service}.service.in
    ${service}.service
    @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${service}.service
    DESTINATION ${CMAKE_INSTALL_SYSTEMD_SERVICEDIR})
endforeach()

install(FILES
  stone.target
  stone-fuse.target
  stone-osd.target
  stone-mgr.target
  stone-mon.target
  stone-mds.target
  stone-radosgw.target
  stone-rbd-mirror.target
  stone-immutable-object-cache.target
  stone-volume@.service
  stonefs-mirror.target
  DESTINATION ${CMAKE_INSTALL_SYSTEMD_SERVICEDIR})
