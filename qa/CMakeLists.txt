set(STONE_BUILD_VIRTUALENV $ENV{TMPDIR})
if(NOT STONE_BUILD_VIRTUALENV)
  set(STONE_BUILD_VIRTUALENV ${CMAKE_BINARY_DIR})
endif()

if(WITH_TESTS)
  include(AddStoneTest)
  add_tox_test(qa TOX_ENVS py3 flake8 import-tasks)
endif()
