# gtest
# --------------------

INCLUDE(FetchContent)

FIND_PACKAGE(GTest QUIET)
IF(NOT GTest_FOUND)
  SET(GTEST_VERSION 1.14.0)

  FETCHCONTENT_DECLARE(
    gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v${GTEST_VERSION}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/gtest-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/gtest-build
  )

  SET(gtest_force_shared_cxxrt ON CACHE BOOL "" FORCE)
  SET(googletest cmake)
  SET(BUILD_GMOCK OFF)
  SET(INSTALL_GTEST OFF)

  FETCHCONTENT_MAKEAVAILABLE(gtest)

  INCLUDE(GoogleTest)
ENDIF()

ADD_FEATURE_INFO(GTest TRUE "Google Test framework")

MACRO(ADD_UNITTEST _exename)
  ADD_EXECUTABLE(${_exename} ${_exename}.cpp)
  TARGET_LINK_LIBRARIES(${_exename} gtest gtest_main ${ARGN})
  gtest_discover_tests(${_exename})
ENDMACRO()