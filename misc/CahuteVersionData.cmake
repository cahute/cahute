# Script to produce the Cahute configuration header.
cmake_minimum_required(VERSION 3.16)

set(CAHUTE_GIT_HASH "not-versioned")
if(CAHUTE_GIT)
    find_package(Git)

    if(Git_FOUND)
        execute_process(
            COMMAND
                ${GIT_EXECUTABLE} --git-dir=.git
                log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE CAHUTE_GIT_COMMIT
            RESULT_VARIABLE CAHUTE_GIT_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(${CAHUTE_GIT_RET} EQUAL 0)
            execute_process(
                COMMAND
                    ${GIT_EXECUTABLE} --git-dir=.git
                    rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE CAHUTE_GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process(
                COMMAND
                    ${GIT_EXECUTABLE} --git-dir=.git
                    tag --points-at HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE CAHUTE_GIT_TAGS
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            string(REPLACE "\n" ";" CAHUTE_GIT_TAGS "${CAHUTE_GIT_TAGS}")

            execute_process(
                COMMAND
                    ${GIT_EXECUTABLE} --git-dir=.git
                    status --porcelain=v1
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE CAHUTE_UNCOMMITED_FILES
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if (NOT "${CAHUTE_UNCOMMITED_FILES}" STREQUAL "")
                set(CAHUTE_GIT_DIRTY 1)
            else()
                set(CAHUTE_GIT_DIRTY 0)
            endif()

            if("${PROJECT_VERSION}" IN_LIST CAHUTE_GIT_TAGS)
                set(CAHUTE_GIT_TAGGED 1)
            else()
                set(CAHUTE_GIT_TAGGED 0)
            endif()

            # Compute the hash.
            list(JOIN CAHUTE_GIT_TAGS "\n" CAHUTE_GIT_TAGS_LIST)
            string(SHA1 CAHUTE_GIT_HASH
                "${CAHUTE_GIT_COMMIT}\n${CAHUTE_GIT_BRANCH}\n${CAHUTE_GIT_DIRTY}\n${CAHUTE_GIT_TAGS_LIST}")
        endif()
    endif()
endif()

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
        "${CMAKE_CURRENT_SOURCE_DIR}/common/version_data.h.in"
        "${CAHUTE_VERSION_DATA_BUILD_PATH}"
        ESCAPE_QUOTES
        @ONLY
    )
endfunction()

# We must read the current hash from the build repository, and force a
# reconfiguration if the hash differs.
if(
    EXISTS "${CAHUTE_VERSION_DATA_BUILD_PATH}"
    AND "${CAHUTE_VERSION_DATA_BUILD_PATH}"
        IS_NEWER_THAN "${CMAKE_CURRENT_SOURCE_DIR}/common/version_data.h.in"
    AND EXISTS "${CAHUTE_GIT_CACHE_PATH}"
)
    file(READ "${CAHUTE_GIT_CACHE_PATH}" CAHUTE_GIT_STORED_HASH)
else()
    set(CAHUTE_GIT_STORED_HASH "")
endif()

if (NOT CAHUTE_GIT_STORED_HASH STREQUAL CAHUTE_GIT_HASH)
    file(WRITE "${CAHUTE_GIT_CACHE_PATH}" "${CAHUTE_GIT_HASH}")
    generate_config()
endif()
