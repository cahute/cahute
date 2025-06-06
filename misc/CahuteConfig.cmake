# Script to produce the Cahute configuration header.
cmake_minimum_required(VERSION 3.16)

set(CAHUTE_GIT_HASH "not-versioned")

function(generate_config)
    # In addition to PROJECT_VERSION*, we define PROJECT_VERSION_HEX as the "0x"
    # prefixed hexadecimal version of the number, so that <cahute/config.h>
    # can include it.
    execute_process(
        COMMAND printf
            "0x%02X%02X0000"
            ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR}
        OUTPUT_VARIABLE PROJECT_VERSION_HEX
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/cahute/config.h.in"
        "${CAHUTE_CONFIG_BUILD_PATH}"
        ESCAPE_QUOTES
        @ONLY
    )
endfunction()

if(
    NOT EXISTS "${CAHUTE_CONFIG_BUILD_PATH}"
    OR "${CMAKE_CURRENT_SOURCE_DIR}/include/cahute/config.h.in"
        IS_NEWER_THAN "${CAHUTE_CONFIG_BUILD_PATH}"
)
    generate_config()
endif()
