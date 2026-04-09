#-------------------------------------------------------------------------------
# CMake Setups
#-------------------------------------------------------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_ERROR_DEPRECATED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-------------------------------------------------------------------------------
# Project Setups
#-------------------------------------------------------------------------------
PROJECT(SimpleAgent)
SET(PROJECT_DESCRIPTION "Simple AI Agent using C++")
ENABLE_LANGUAGE(C CXX)

#-------------------------------------------------------------------------------
# CMake Modules
#-------------------------------------------------------------------------------
INCLUDE(FeatureSummary)
INCLUDE(FindPkgConfig)
INCLUDE(FetchContent)
INCLUDE(CTest)

INCLUDE(cmake/clang-format.cmake)
INCLUDE(cmake/gtest.cmake)

# Detect avaliable options directly
INCLUDE(options.cmake)

#-------------------------------------------------------------------------------
# Sub Projects
#-------------------------------------------------------------------------------
ADD_SUBDIRECTORY(src)

#-------------------------------------------------------------------------------
# Sub Projects (unittest)
#-------------------------------------------------------------------------------
ENABLE_TESTING()

#-------------------------------------------------------------------------------
# Show Project Summary Information
#-------------------------------------------------------------------------------
FEATURE_SUMMARY(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
