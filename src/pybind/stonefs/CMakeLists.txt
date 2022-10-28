distutils_add_cython_module(cython_stonefs
  stonefs
  ${CMAKE_CURRENT_SOURCE_DIR}/stonefs.pyx)
add_dependencies(cython_stonefs stonefs)
distutils_install_cython_module(cython_stonefs)
