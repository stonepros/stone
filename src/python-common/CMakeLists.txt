include(Distutils)
distutils_install_module(stone)

if(WITH_TESTS)
  include(AddStoneTest)
  add_tox_test(python-common TOX_ENVS py3 lint)
endif()
