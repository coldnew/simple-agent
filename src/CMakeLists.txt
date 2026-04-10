#-------------------------------------------------------------------------------
# Sub Projects
#-------------------------------------------------------------------------------
ADD_SUBDIRECTORY(tools)

#-------------------------------------------------------------------------------
# Generate Binary
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(simple_agent
    main.cpp
    agent.cpp
    message.cpp
    sandbox.cpp
    tool_manager.cpp
)

TARGET_LINK_LIBRARIES(simple_agent PRIVATE
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    fmt::fmt
    tool-static
)

#-------------------------------------------------------------------------------
# Unittests
#-------------------------------------------------------------------------------
# Add all *_unitest.cpp to unittest
FILE(GLOB unittests "*_unittest.cpp")
FOREACH(unittest ${unittests})
  GET_FILENAME_COMPONENT(unittest_name ${unittest} NAME_WE)
  ADD_UNITTEST(${unittest_name}
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    fmt::fmt
    tool-static
  )
ENDFOREACH()
