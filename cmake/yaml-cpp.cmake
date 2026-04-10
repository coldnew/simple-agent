INCLUDE(FetchContent)

SET(yaml-cpp_VERSION "0.8.0")

FIND_PACKAGE(yaml-cpp QUIET)
IF(yaml-cpp_FOUND)
  MESSAGE(STATUS "Found system yaml-cpp: ${yaml-cpp_VERSION}")
ELSE()
  FETCHCONTENT_DECLARE(
      yaml-cpp
      URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/${yaml-cpp_VERSION}.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  SET(yaml-cpp_BUILD_SHARED OFF)
  SET(yaml-cpp_BUILD_TESTS OFF)
  SET(yaml-cpp_BUILD_TOOLS OFF)
  SET(yaml-cpp_INSTALL OFF)
  SET(YAML_CPP_FORMAT_SOURCE OFF)

  SET(CLANG_FORMAT_FOUND FALSE)
  SET(CLANG_FORMAT_PROGRAM "CLANG_FORMAT-NOTFOUND")

  FETCHCONTENT_MAKEAVAILABLE(yaml-cpp)
ENDIF()