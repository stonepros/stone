if(WITH_TESTS)
  include(AddStoneTest)
  add_tox_test(stoneadm TOX_ENVS py3 mypy flake8)
endif()
