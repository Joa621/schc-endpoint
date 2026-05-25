set(EXEC_NAME    "schc-decompressor")
set(LOGGER_LIB   "logger-lib")
set(SCHC_SERVICE "schc-service-lib")

# ---------------------------------------------------------------------------
# Logger
# ---------------------------------------------------------------------------
add_library(${LOGGER_LIB} OBJECT "logger_helper.c")
target_include_directories(${LOGGER_LIB} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/schc_endpoint"
)
target_compile_definitions(${LOGGER_LIB} PRIVATE LOG_CONFIG_FILE="${LOG_CONFIG_FILE}")
target_link_libraries(${LOGGER_LIB} PRIVATE ${ZLOG_LIB})

# ---------------------------------------------------------------------------
# SCHC service (Rule 28 + Rule 150, RX/sink perspective)
# ---------------------------------------------------------------------------
add_library(${SCHC_SERVICE} OBJECT "schc_service.c" $<TARGET_OBJECTS:${LOGGER_LIB}>)
target_include_directories(${SCHC_SERVICE} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/schc_endpoint"
)
target_link_libraries(${SCHC_SERVICE} PRIVATE ${SCHC_FULL_SDK_LIB} ${ZLOG_LIB})

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(${EXEC_NAME}
    "main.c"
    $<TARGET_OBJECTS:${LOGGER_LIB}>
    $<TARGET_OBJECTS:${SCHC_SERVICE}>
)

target_include_directories(${EXEC_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/schc_endpoint"
)

target_link_libraries(${EXEC_NAME}
    ${SCHC_FULL_SDK_LIB}
    ${ZLOG_LIB}
)
