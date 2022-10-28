set(stonefs_journal_tool_srcs
  stonefs-journal-tool.cc
  JournalTool.cc
  JournalFilter.cc
  JournalScanner.cc
  EventOutput.cc
  Dumper.cc
  Resetter.cc
  RoleSelector.cc
  MDSUtility.cc)
add_executable(stonefs-journal-tool ${stonefs_journal_tool_srcs})
target_link_libraries(stonefs-journal-tool librados mds osdc global
  ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

set(stonefs-meta-injection_srcs
  stonefs-meta-injection.cc
  MetaTool.cc
  RoleSelector.cc
  MDSUtility.cc)
add_executable(stonefs-meta-injection ${stonefs-meta-injection_srcs})
target_link_libraries(stonefs-meta-injection librados mds osdc global
  ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

set(stonefs_table_tool_srcs
  stonefs-table-tool.cc
  TableTool.cc
  RoleSelector.cc
  MDSUtility.cc)
add_executable(stonefs-table-tool ${stonefs_table_tool_srcs})
target_link_libraries(stonefs-table-tool librados mds osdc global
  ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

set(stonefs_data_scan_srcs
  stonefs-data-scan.cc
  DataScan.cc
  RoleSelector.cc
  PgFiles.cc
  MDSUtility.cc)
add_executable(stonefs-data-scan ${stonefs_data_scan_srcs})
target_link_libraries(stonefs-data-scan librados stonefs mds osdc global
  cls_stonefs_client
  ${BLKID_LIBRARIES} ${CMAKE_DL_LIBS})

install(TARGETS
  stonefs-journal-tool
  stonefs-table-tool
  stonefs-data-scan
  DESTINATION bin)

option(WITH_STONEFS_SHELL "install stonefs-shell" OFF)
if(WITH_STONEFS_SHELL)
  include(Distutils)
  distutils_install_module(stonefs-shell)
  if(WITH_TESTS)
    include(AddStoneTest)
    add_tox_test(stonefs-shell)
  endif()
endif()

option(WITH_STONEFS_TOP "install stonefs-top utility" ON)
if(WITH_STONEFS_TOP)
  add_subdirectory(top)
endif()
