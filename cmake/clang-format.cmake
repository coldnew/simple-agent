INCLUDE(FeatureSummary)

if(CLANG_FORMAT_PROGRAM)
  find_program(CLANG_FORMAT_BIN NAMES ${CLANG_FORMAT_PROGRAM})
else()
  find_program(CLANG_FORMAT_BIN NAMES clang-format)
endif()

if(CLANG_FORMAT_BIN)
  ADD_FEATURE_INFO(CLANG_FORMAT TRUE "Use cmake --build . --target format to format all source files using clang-format")
  file(
    GLOB_RECURSE CPP_SOURCE_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/include/*.c"
    "${CMAKE_SOURCE_DIR}/include/*.cc"
    "${CMAKE_SOURCE_DIR}/include/*.cpp")
  file(
    GLOB_RECURSE CPP_HEADER_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.hh"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.h"
    "${CMAKE_SOURCE_DIR}/include/*.hh"
    "${CMAKE_SOURCE_DIR}/include/*.hpp")
  set(CLANG_FORMAT_FILES ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES})

  set(FORMAT_SOURCES_STAMPS "")
  set(FORMAT_HEADERS_STAMPS "")
  set(FORMAT_CHECK_STAMPS "")

  foreach(FMT_FILE IN LISTS CPP_SOURCE_FILES)
    string(MD5 FMT_HASH "${FMT_FILE}")
    set(FMT_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/.format/format-source-${FMT_HASH}.stamp")
    set(FMT_CHECK_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/.format/check-source-${FMT_HASH}.stamp")

    add_custom_command(
      OUTPUT ${FMT_STAMP}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMAKE_CURRENT_BINARY_DIR}/.format"
      COMMAND ${CLANG_FORMAT_BIN} --style=file -i "${FMT_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${FMT_STAMP}
      DEPENDS ${FMT_FILE} "${CMAKE_SOURCE_DIR}/.clang-format"
      VERBATIM)

    add_custom_command(
      OUTPUT ${FMT_CHECK_STAMP}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMAKE_CURRENT_BINARY_DIR}/.format"
      COMMAND ${CLANG_FORMAT_BIN} --style=file --dry-run --Werror "${FMT_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${FMT_CHECK_STAMP}
      DEPENDS ${FMT_FILE} "${CMAKE_SOURCE_DIR}/.clang-format"
      VERBATIM)

    list(APPEND FORMAT_SOURCES_STAMPS ${FMT_STAMP})
    list(APPEND FORMAT_CHECK_STAMPS ${FMT_CHECK_STAMP})
  endforeach()

  foreach(FMT_FILE IN LISTS CPP_HEADER_FILES)
    string(MD5 FMT_HASH "${FMT_FILE}")
    set(FMT_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/.format/format-header-${FMT_HASH}.stamp")
    set(FMT_CHECK_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/.format/check-header-${FMT_HASH}.stamp")

    add_custom_command(
      OUTPUT ${FMT_STAMP}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMAKE_CURRENT_BINARY_DIR}/.format"
      COMMAND ${CLANG_FORMAT_BIN} --style=file -i "${FMT_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${FMT_STAMP}
      DEPENDS ${FMT_FILE} "${CMAKE_SOURCE_DIR}/.clang-format"
      VERBATIM)

    add_custom_command(
      OUTPUT ${FMT_CHECK_STAMP}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMAKE_CURRENT_BINARY_DIR}/.format"
      COMMAND ${CLANG_FORMAT_BIN} --style=file --dry-run --Werror "${FMT_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${FMT_CHECK_STAMP}
      DEPENDS ${FMT_FILE} "${CMAKE_SOURCE_DIR}/.clang-format"
      VERBATIM)

    list(APPEND FORMAT_HEADERS_STAMPS ${FMT_STAMP})
    list(APPEND FORMAT_CHECK_STAMPS ${FMT_CHECK_STAMP})
  endforeach()

  add_custom_target(format-sources DEPENDS ${FORMAT_SOURCES_STAMPS})
  add_custom_target(format-headers DEPENDS ${FORMAT_HEADERS_STAMPS})
  add_custom_target(format DEPENDS format-sources format-headers)
  add_custom_target(format-check DEPENDS ${FORMAT_CHECK_STAMPS})
endif()
