# Unit tests for the Chrono::Networking module
# ==================================================================

#--------------------------------------------------------------
# Additional include paths (for ChronoEngine_Utils)
include_directories(${CH_NETWORKING_INCLUDES})

# Libraries
SET(LIBRARIES
    ChronoEngine
    ChronoModels_vehicle
    ChronoEngine_vehicle
    ChronoEngine_networking
    boost_system
    pthread
    protobuf
)

SET(TESTS
    network-tests
    server-tests
    world-tests
)

MESSAGE(STATUS "Unit test programs for NETWORKING module")

FOREACH(PROGRAM ${TESTS})
    MESSAGE(STATUS "...and ${PROGRAM}")

    add_executable(${PROGRAM} "${PROGRAM}.cpp")
    SOURCE_GROUP("" FILES "${PROGRAM}.cpp")

    SET_TARGET_PROPERTIES(${PROGRAM} PROPERTIES
        FOLDER demos
        COMPILE_FLAGS "${CH_CXX_FLAGS}"
        LINK_FLAGS "${CH_LINKERFLAG_EXE}"
    )

    TARGET_LINK_LIBRARIES(${PROGRAM} ${LIBRARIES})
    ADD_DEPENDENCIES(${PROGRAM} ${LIBRARIES})

    INSTALL(TARGETS ${PROGRAM} DESTINATION ${CH_INSTALL_DEMO})
    ADD_TEST(${PROGRAM} ${PROJECT_BINARY_DIR}/bin/${PROGRAM})

ENDFOREACH(PROGRAM)
