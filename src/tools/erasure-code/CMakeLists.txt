add_executable(stone-erasure-code-tool
  ${PROJECT_SOURCE_DIR}/src/osd/ECUtil.cc
  stone-erasure-code-tool.cc)
target_link_libraries(stone-erasure-code-tool global stone-common)
install(TARGETS stone-erasure-code-tool DESTINATION bin)
