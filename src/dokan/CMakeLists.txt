set(stone_dokan_srcs
  stone_dokan.cc
  dbg.cc
  utils.cc
  options.cc)
add_executable(stone-dokan ${stone_dokan_srcs})
target_link_libraries(stone-dokan ${DOKAN_LIBRARIES}
  ${GSSAPI_LIBRARIES}
  stonefs stone-common global ${EXTRALIBS})
set_target_properties(stone-dokan PROPERTIES
  COMPILE_FLAGS "-I${DOKAN_INCLUDE_DIRS}")
install(TARGETS stone-dokan DESTINATION bin)
