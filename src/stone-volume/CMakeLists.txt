
include(Distutils)

distutils_install_module(stone_volume
  INSTALL_SCRIPT ${CMAKE_INSTALL_FULL_SBINDIR})

if(FREEBSD)
  add_subdirectory(plugin/zfs)
endif()

# Required for running stone-volume inventory in a vstart environment
set(STONE_VOLUME_VIRTUALENV ${STONE_BUILD_VIRTUALENV}/stone-volume-virtualenv)

add_custom_command(
  OUTPUT ${STONE_VOLUME_VIRTUALENV}/bin/python
  COMMAND ${CMAKE_SOURCE_DIR}/src/tools/setup-virtualenv.sh --python=${Python3_EXECUTABLE} ${STONE_VOLUME_VIRTUALENV}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/stone-volume
  COMMENT "stone-volume venv is being created")

add_custom_command(
  OUTPUT ${STONE_VOLUME_VIRTUALENV}/bin/stone-volume
  DEPENDS ${STONE_VOLUME_VIRTUALENV}/bin/python
  COMMAND . ${STONE_VOLUME_VIRTUALENV}/bin/activate && ${STONE_VOLUME_VIRTUALENV}/bin/python setup.py develop && deactivate
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/stone-volume
  COMMENT "${CMAKE_SOURCE_DIR}/src/stone-volume")

add_custom_target(stone-volume-venv-setup
  DEPENDS ${STONE_VOLUME_VIRTUALENV}/bin/stone-volume)

