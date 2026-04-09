#-------------------------------------------------------------------------------
# Generate Binary
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(simple_agent
    main.cpp
    agent.cpp
    message.cpp
    tools.cpp
    tool/read_file.cpp
)

TARGET_LINK_LIBRARIES(simple_agent PRIVATE
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
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
  )
ENDFOREACH()
