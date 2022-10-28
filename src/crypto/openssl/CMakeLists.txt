## openssl

set(openssl_crypto_plugin_srcs
  openssl_crypto_accel.cc
  openssl_crypto_plugin.cc)

add_library(stone_crypto_openssl SHARED ${openssl_crypto_plugin_srcs})
target_link_libraries(stone_crypto_openssl
    PRIVATE OpenSSL::Crypto
    $<$<PLATFORM_ID:Windows>:stone-common>)
target_include_directories(stone_crypto_openssl PRIVATE ${OPENSSL_INCLUDE_DIR})
add_dependencies(crypto_plugins stone_crypto_openssl)
set_target_properties(stone_crypto_openssl PROPERTIES INSTALL_RPATH "")
install(TARGETS stone_crypto_openssl DESTINATION ${crypto_plugin_dir})
