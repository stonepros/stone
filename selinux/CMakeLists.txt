add_custom_target(
  stone.pp ALL
  COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/stone.* .
  COMMAND make -f ${SELINUX_MAKEFILE} stone.pp
  DEPENDS stone.te stone.fc stone.if)

set(datadir ${CMAKE_INSTALL_PREFIX}/share/selinux)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/stone.pp
  DESTINATION ${datadir}/packages)
install(FILES stone.if
  DESTINATION ${datadir}/devel/include/contrib)
