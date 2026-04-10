# fmt
# --------------------

INCLUDE(FetchContent)

FIND_PACKAGE(fmt QUIET)
IF(NOT fmt_FOUND)
  SET(FMT_VERSION 11.1.2)

  FETCHCONTENT_DECLARE(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG ${FMT_VERSION}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/fmt-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/fmt-build
  )

  SET(FMT_TEST OFF)
  SET(FMT_DOC OFF)
  SET(FMT_INSTALL OFF)

  FETCHCONTENT_MAKEAVAILABLE(fmt)
ENDIF()

ADD_FEATURE_INFO(fmt TRUE "Formatting library")
