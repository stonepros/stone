include(Distutils)
distutils_install_module(stonefs-top)

if(WITH_TESTS)
  include(AddStoneTest)
  add_tox_test(stonefs-top)
endif()
