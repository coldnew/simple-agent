#-------------------------------------------------------------------------------
# CMake Modules
#-------------------------------------------------------------------------------
INCLUDE (CheckLibraryExists)
INCLUDE (CheckIPOSupported)
INCLUDE (FindThreads)

#-------------------------------------------------------------------------------
# Compoler Options
#-------------------------------------------------------------------------------
# we use C11 in this project
SET(CMAKE_C_STANDARD   11)
SET(CMAKE_C_STANDARD_REQUIRED ON)

# we use C++17 in this project
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# check if we can use LTO optimize
CHECK_IPO_SUPPORTED(RESULT BUILD_WITH_LTO OUTPUT ERROR_MSG)
if(BUILD_WITH_LTO)
  SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  ADD_DEFINITIONS("-DBUILD_WITH_LTO=1")
else()
  MESSAGE(WARNING "LTO unavailable: ${ERROR_MSG}")
  ADD_DEFINITIONS("-DBUILD_WITH_LTO=0")
endif()
ADD_FEATURE_INFO(LTO BUILD_WITH_LTO "build with LTO (Link Time Optimization) enabled")

#-------------------------------------------------------------------------------
# Generate options
#-------------------------------------------------------------------------------

# curl
PKG_CHECK_MODULES(CURL REQUIRED libcurl)
SET_PACKAGE_PROPERTIES(libcurl PROPERTIES
  DESCRIPTION "A Client that groks URLs"
  URL "https://curl.se/"
  TYPE REQUIRED)
IF(BREAKPAD_FOUND)
  SET_PROPERTY(GLOBAL APPEND PROPERTY PACKAGES_FOUND libcurl)
ENDIF()

# nlohmann_json
FIND_PACKAGE(nlohmann_json QUIET)
IF(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
      nlohmann_json
      GIT_REPOSITORY https://github.com/nlohmann/json.git
      GIT_TAG        v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)
ENDIF()


#-------------------------------------------------------------------------------
# Basic deps we need in this project
#-------------------------------------------------------------------------------

# curl
INCLUDE_DIRECTORIES (${CURL_INCLUDE_DIRS})


