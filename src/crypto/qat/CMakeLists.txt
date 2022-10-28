##
# QAT wrapper for Stone
##

# Build QAT driver library first
include(BuildQatDrv)

set(qat_crypto_plugin_srcs
  qat_crypto_accel.cc
  qat_crypto_plugin.cc
  qcccrypto.cc)

add_library(stone_crypto_qat SHARED ${qat_crypto_plugin_srcs})
add_dependencies(stone_crypto_qat QatDrv)
target_include_directories(stone_crypto_qat PRIVATE
  ${EXTRA_LIBS}
  ${QatDrv_INCLUDE_DIRS})

add_dependencies(crypto_plugins stone_crypto_qat)

target_link_libraries(stone_crypto_qat
  ${QatDrv_LIBRARIES})
add_dependencies(crypto_plugins stone_crypto_qat)
set_target_properties(stone_crypto_qat PROPERTIES VERSION 1.0.0 SOVERSION 1)
install(TARGETS stone_crypto_qat DESTINATION ${crypto_plugin_dir})
