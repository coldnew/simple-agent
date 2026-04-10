#-------------------------------------------------------------------------------
# Headers
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/src)

#-------------------------------------------------------------------------------
# Generate Binary
#-------------------------------------------------------------------------------

ADD_LIBRARY (tool-static STATIC
  read_file.cpp
  write_file.cpp
  edit_file.cpp
  shell.cpp
)	

#-------------------------------------------------------------------------------
# Export Interface
#-------------------------------------------------------------------------------
TARGET_INCLUDE_DIRECTORIES(tool-static
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

TARGET_LINK_LIBRARIES(
  tool-static
  PRIVATE
  nlohmann_json
  fmt::fmt)



