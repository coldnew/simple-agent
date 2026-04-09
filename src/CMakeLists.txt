#-------------------------------------------------------------------------------
# Generate Binary
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(simple_agent
    main.cpp
    agent.cpp
    message.cpp
    tools.cpp
)

TARGET_LINK_LIBRARIES(simple_agent PRIVATE
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
