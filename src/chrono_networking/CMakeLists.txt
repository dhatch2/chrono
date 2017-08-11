#=============================================================================
# CMake configuration file for Chrono Networking module
#
# Cannot be used stand-alone (it is loaded by parent CMake configuration file)
#=============================================================================

option(ENABLE_MODULE_NETWORKING "Enable the Chrono Networking module" OFF)

# Return now if this module is not enabled
if(NOT ENABLE_MODULE_NETWORKING)
   return()
endif()

message(STATUS "==== Chrono Networking module ====")

# ----- BOOST -- required for Networking -----

set(BOOST_ROOT "" CACHE PATH "Where is Boost located?")
find_package(Boost REQUIRED)

if (Boost_FOUND)
  message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
else()
  mark_as_advanced(CLEAR BOOST_ROOT)
endif()

# ----- PROTOBUF -- required for Networking -----

set(PROTOBUF_ROOT "" CACHE PATH "Where is Protobuf located?")
include(FindProtobuf)
find_package(Protobuf REQUIRED)

if (Protobuf_FOUND)
    message(STATUS "Protobuf include dir: ${PROTOBUF_INCLUDE_DIRS}")
else()
    mark_as_advanced(CLEAR PROTOBUF_ROOT)
endif()

add_custom_command(
    OUTPUT protobuf/ChronoMessages.pb.cc protobuf/ChronoMessages.pb.h
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} protobuf/ChronoMessages.proto --cpp_out=.
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# execute_process(
#    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=protobuf protobuf/ChronoMessages.proto
#)

# Generate the configuration header file using substitution variables.

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ChConfigNetworking.h.in
               ${PROJECT_BINARY_DIR}/chrono_networking/ChConfigNetworking.h)

install(FILES "${PROJECT_BINARY_DIR}/chrono_networking/ChConfigNetworking.h"
        DESTINATION include/chrono_networking)

# ------------------------------------------------------------------------------
# Collect all additional include directories necessary for the Networking module
# ------------------------------------------------------------------------------

SET(CH_NETWORKING_INCLUDES
    ${Boost_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cave_server
    ${CMAKE_CURRENT_SOURCE_DIR}/cave_server/World
    ${CMAKE_CURRENT_SOURCE_DIR}/cave_agent
    ${CMAKE_CURRENT_SOURCE_DIR}/network_handler
    ${CMAKE_CURRENT_SOURCE_DIR}/protobuf
)

include_directories(${CH_NETWORKING_INCLUDES})

SET(CH_NETWORKING_INCLUDES "${CH_NETWORKING_INCLUDES}" PARENT_SCOPE)

# List the files in the ChronoEngine_networking library
# ----------------------------------------------------------------------------

# --------------- COMMON FILES

SET(CN_HANDLER_FILES
    ChSafeQueue.h
    network_handler/ChNetworkHandler.cpp
    network_handler/ChNetworkHandler.h
)
source_group("handler" FILES ${CN_HANDLER_FILES})

SET(CN_SERVER_FILES
    ChSafeQueue.h
)
source_group("server" FILES ${CN_SERVER_FILES})

SET(CN_WORLD_FILES
    cave_server/World/World.cpp
    cave_server/World/World.h
)
source_group("world" FILES ${CN_WORLD_FILES})

SET(CN_AGENT_FILES
    cave_agent/MessageConversions.cpp
    cave_agent/MessageConversions.h
    cave_agent/ServerVehicle.cpp
    cave_agent/ServerVehicle.h
    cave_agent/WorldObject.cpp
    cave_agent/WorldObject.h
)
source_group("agent" FILES ${CN_AGENT_FILES})

SET(CN_PROTOBUF_FILES
    protobuf/ChronoMessages.pb.cc
    protobuf/ChronoMessages.pb.h
    protobuf/MessageCodes.h
)
source_group("protobuf" FILES ${CN_PROTOBUF_FILES})

SET(CXX_FLAGS ${CH_CXX_FLAGS})
SET(LINK_FLAGS ${CH_LINKERFLAG_SHARED})
SET(LIBRARIES "ChronoEngine")

add_library(ChronoEngine_networking SHARED
    ${CN_HANDLER_FILES}
    ${CN_SERVER_FILES}
    ${CN_WORLD_FILES}
    ${CN_AGENT_FILES}
    ${CN_PROTOBUF_FILES}
)

add_executable(cave-server
    cave_server/CAVE-Server.cpp
    ${CN_SERVER_FILES}
    ${CN_HANDLER_FILES}
    ${CN_WORLD_FILES}
    ${CN_PROTOBUF_FILES}
)

add_executable(dsrc-agent
    cave_agent/DSRC-Agent.cpp
    ${CN_HANDLER_FILES}
    ${CN_PROTOBUF_FILES}
)

add_executable(dsrc-server
    cave_server/DSRC-Server.cpp
    ${CN_HANDLER_FILES}
    ${CN_PROTOBUF_FILES}
)

target_link_libraries(cave-server boost_system pthread protobuf)
target_link_libraries(dsrc-agent boost_system pthread protobuf)
target_link_libraries(dsrc-server boost_system pthread protobuf)

set_target_properties(ChronoEngine_networking PROPERTIES
                      COMPILE_FLAGS "${CXX_FLAGS}"
                      LINK_FLAGS "${CH_LINKERFLAG_SHARED}"
                      COMPILE_DEFINITIONS "CH_API_COMPILE_NETWORKING")

target_link_libraries(ChronoEngine_networking ${LIBRARIES})
target_link_libraries(cave-server ${LIBRARIES})
target_link_libraries(dsrc-agent ${LIBRARIES})
target_link_libraries(dsrc-server ${LIBRARIES})

install(TARGETS ChronoEngine_networking
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib64
        ARCHIVE DESTINATION lib64
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/chrono_networking
        FILES_MATCHING PATTERN "*.h"
)
