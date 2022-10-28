add_library(stonefs_jni SHARED
  libstonefs_jni.cc
  ScopedLocalRef.h
  JniConstants.cpp
  JniConstants.h)
set_target_properties(stonefs_jni PROPERTIES
  VERSION 1.0.0
  SOVERSION 1)
add_dependencies(stonefs_jni jni-header)
include_directories(${JNI_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(stonefs_jni PRIVATE stonefs stone-common
  ${EXTRALIBS} ${JNI_LIBRARIES})
install(TARGETS stonefs_jni
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
