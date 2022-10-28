find_package(Java COMPONENTS Development REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

set(java_srcs
  java/com/stone/crush/Bucket.java
  java/com/stone/fs/StoneAlreadyMountedException.java
  java/com/stone/fs/StoneFileAlreadyExistsException.java
  java/com/stone/fs/StoneFileExtent.java
  java/com/stone/fs/StoneMount.java
  java/com/stone/fs/StoneNativeLoader.java
  java/com/stone/fs/StoneNotDirectoryException.java
  java/com/stone/fs/StoneNotMountedException.java
  java/com/stone/fs/StonePoolException.java
  java/com/stone/fs/StoneStat.java
  java/com/stone/fs/StoneStatVFS.java)

# note: for the -source 1.7 builds, we add
#   -Xlint:-options
# to get rid of the warning
#   warning: [options] bootstrap class path not set in conjunction with -source 1.7
# as per
#   https://blogs.oracle.com/darcy/entry/bootclasspath_older_source
set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.8" "-target" "1.8" "-Xlint:-options")
set(jni_header_dir "${CMAKE_CURRENT_BINARY_DIR}/native")
if(CMAKE_VERSION VERSION_LESS 3.11)
  set(CMAKE_JAVA_COMPILE_FLAGS ${CMAKE_JAVA_COMPILE_FLAGS} "-h" ${jni_header_dir})
  add_jar(libstonefs ${java_srcs})
  add_custom_target(
    jni-header
    DEPENDS libstonefs)
  add_dependencies(jni-header libstonefs)
else()
  add_jar(libstonefs ${java_srcs}
    GENERATE_NATIVE_HEADERS jni-header
    DESTINATION ${jni_header_dir})
endif()
get_property(libstonefs_jar TARGET libstonefs PROPERTY JAR_FILE)
install_jar(libstonefs share/java)

find_jar(JUNIT_JAR
  NAMES junit4 junit
  PATHS "/usr/share/java")
if(JUNIT_JAR)
  set(CMAKE_JAVA_INCLUDE_PATH ${JUNIT_JAR} ${libstonefs_jar})
  set(java_test_srcs
    test/com/stone/fs/StoneAllTests.java
    test/com/stone/fs/StoneDoubleMountTest.java
    test/com/stone/fs/StoneMountCreateTest.java
    test/com/stone/fs/StoneMountTest.java
    test/com/stone/fs/StoneUnmountedTest.java)
  add_jar(libstonefs-test ${java_test_srcs})
  add_dependencies(libstonefs-test libstonefs)
  install_jar(libstonefs-test share/java)
endif(JUNIT_JAR)

add_subdirectory(native)

add_custom_target(java DEPENDS
  libstonefs.jar
  libstonefs_jni)
