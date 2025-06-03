cmake_minimum_required(VERSION 3.16)
project(
    cahute
    VERSION 0.6
    DESCRIPTION "Communication and file format handling tools for CASIO calculators"
    HOMEPAGE_URL "https://cahuteproject.org/"
    LANGUAGES C
)

# ---
# Options.
# ---

include(CMakeDependentOption)

set(CAHUTE_DEFAULT_LOGLEVEL "warning" CACHE STRING
    "Default logging level used when creating a context?")
set_property(CACHE CAHUTE_DEFAULT_LOGLEVEL PROPERTY STRINGS
    "info" "warning" "error" "fatal" "none")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(CACHE CAHUTE_DEFAULT_LOGLEVEL PROPERTY VALUE "info")
endif()

set(CAHUTE_CLI ON CACHE BOOL
    "Enable command-line utilities?")
if(DEFINED FXSDK_PLATFORM)
    set_property(CACHE CAHUTE_CLI PROPERTY VALUE OFF)
endif()

set(CAHUTE_CLI_EXPERIMENTAL OFF CACHE BOOL
    "Enable experimental command-line utilities?")

set(CAHUTE_GIT ON CACHE BOOL
    "Include git-related information if available?")

set(CAHUTE_LIBUSB ON CACHE BOOL
    "Enable usage of libusb?")
if(DEFINED FXSDK_PLATFORM OR DEFINED AMIGA OR DEFINED WATCOM OR WIN32)
    set_property(CACHE CAHUTE_LIBUSB PROPERTY VALUE OFF)
endif()

set(CAHUTE_PKGCONF ON CACHE BOOL
    "Enable installation of pkgconf files?")

set(CAHUTE_REPORT_URL "https://cahuteproject.org/guides/report.html"
    CACHE STRING "Bug report guide URL?")

set(CAHUTE_SDL ON CACHE BOOL
    "Enable usage of SDL?")
if(DEFINED FXSDK_PLATFORM OR DEFINED AMIGA OR DEFINED WATCOM)
    set_property(CACHE CAHUTE_SDL PROPERTY VALUE OFF)
endif()

set(CAHUTE_UDEV OFF CACHE BOOL
    "Enable installation of udev rule?")
if(LINUX)
    set_property(CACHE CAHUTE_UDEV PROPERTY VALUE ON)
endif()

set(CAHUTE_UDEV_GROUP "uucp" CACHE STRING
    "Name of the group inserted into the udev rule?")

# ---
# Feature switches.
# ---

cmake_dependent_option(CAHUTE_FEATURE_CLI_CAS
    "Enable CaS target?"
    ON "CAHUTE_CLI AND CAHUTE_CLI_EXPERIMENTAL" OFF)

cmake_dependent_option(CAHUTE_FEATURE_CLI_P7
    "Enable p7 target?"
    ON "CAHUTE_CLI" OFF)

cmake_dependent_option(CAHUTE_FEATURE_CLI_P7OS
    "Enable p7os target?"
    ON "CAHUTE_CLI" OFF)

cmake_dependent_option(CAHUTE_FEATURE_CLI_P7SCREEN
    "Enable p7screen target?"
    ON "CAHUTE_CLI AND CAHUTE_SDL" OFF)

cmake_dependent_option(CAHUTE_FEATURE_CLI_XFER9860
    "Enable xfer9860 target?"
    ON "CAHUTE_CLI" OFF)

set(CAHUTE_FEATURE_LIB_HEADERS ON CACHE BOOL
    "Enable Cahute headers?")

set(CAHUTE_FEATURE_LIB_PKGCONF ON CACHE BOOL
    "Enable Cahute pkgconf / pkg-config files?")

set(CAHUTE_FEATURE_LIB_STATIC ON CACHE BOOL
    "Enable Cahute (static library) target?")

cmake_dependent_option(CAHUTE_FEATURE_UDEV
    "Enable udev rules?"
    ON "CAHUTE_UDEV" OFF)

mark_as_advanced(
    CAHUTE_FEATURE_CLI_CAS
    CAHUTE_FEATURE_CLI_P7
    CAHUTE_FEATURE_CLI_P7OS
    CAHUTE_FEATURE_CLI_P7SCREEN
    CAHUTE_FEATURE_CLI_XFER9860
    CAHUTE_FEATURE_LIB_HEADERS
    CAHUTE_FEATURE_LIB_PKGCONF
    CAHUTE_FEATURE_LIB_STATIC
    CAHUTE_FEATURE_UDEV)

# ---
# Packaging options.
# ---

set(CPACK_PACKAGE_VENDOR "Cahute project")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}")
set(CPACK_PACKAGE_ICON "misc/cahute.bmp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_VERBATIM_VARIABLES true)

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(
    CPACK_SOURCE_IGNORE_FILES
    "\\.git" "/build/" ".*~$" "\\.envrc" "\\.pre-commit-config\\.yaml"
    "\\.tool-versions" "\\.clang-format" "__pycache__" "\\.gitlab-ci.yml"
    "_CPack" "/docs/"
)

include(CPack)

# ---
# Common definitions.
# ---
# Cahute is developed against ANSI C / ISO C90, we want to enforce this as
# much as possible to ensure that we stay compatible.
set(CMAKE_C_STANDARD 90)

# We want to add as much warnings as we can to all targets.
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # CMake defines "-O3" by default for the Release target for some platforms,
    # including Windows. This is very unsafe, so we force "-O2" instead.
    set(CMAKE_C_FLAGS_RELEASE -O2)

    add_compile_options(
        -Wall -Wextra -Wno-c99-extensions -Wc23-extensions
    )
elseif (CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom")
else()
    # CMake defines "-O3" by default for the Release target for multiple
    # compilers with the GNU command-line interface.
    # This is very unsafe, so we force "-O2" instead.
    set(CMAKE_C_FLAGS_RELEASE -O2)

    add_compile_options(
        -Wall -Wextra -Wno-unused-macros
        -Wshadow -Wwrite-strings -Wredundant-decls -Wformat -Wformat-nonliteral
        -Wformat-security -Wimplicit-function-declaration
        -Wdate-time -Wmissing-prototypes -Wreturn-type -Wpointer-arith
        -Wstack-protector -Wno-unused-parameter
    )
endif()

# LIB_INCLUDE_DIRS represents the directories to include in order to compile
# the library.
set(LIB_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_BINARY_DIR}/common"
)

# LIB_COMPILE_DEFS represents the additional defines to pass to the compiler
# while compiling library, as compiler options ("-DXYZ=ABC").
string(TOUPPER ${CAHUTE_DEFAULT_LOGLEVEL} CAHUTE_DEFAULT_LOGLEVEL_UPPER)
set(LIB_COMPILE_DEFS "-DCAHUTE_DEFAULT_LOGLEVEL=CAHUTE_LOGLEVEL_${CAHUTE_DEFAULT_LOGLEVEL_UPPER}")

# CLI_INCLUDE_DIRS represent the directories to include in order to compile
# any command-line utility.
set(CLI_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_BINARY_DIR}/common"
)

# CLI_LIBRARIES represent the libraries against which to link for any
# command-line utility.
set(CLI_LIBRARIES
    cahute
)

# PKGCONFIG_DEPS represent the additional dependencies to place in the
# pkgconf / pkg-config configuration file when installing the library.
set(PKGCONFIG_DEPS "")

# PKGCONFIG_LIBS represent the additional includes to place in the
# pkgconf / pkg-config configuration file when installing the library.
set(PKGCONFIG_LIBS "")

# USE_LIBUSB represents whether to include and use libusb.
# It is disabled on Win32 because we manage some drivers libusb does not,
# that are more important in our case (CESG502, mostly).
if(CAHUTE_LIBUSB AND NOT WIN32)
    set(USE_LIBUSB ON)
else()
    set(USE_LIBUSB OFF)
endif()

include(GNUInstallDirs)
include(CheckTypeSize)

if(USE_LIBUSB)
    find_package(PkgConfig REQUIRED)
endif()

find_package(Python3 REQUIRED)

if(WIN32 OR MINGW)
    set(SDL2_NO_MWINDOWS 1)
endif()

if(CAHUTE_FEATURE_CLI_P7SCREEN)
    find_package(SDL2 REQUIRED)
endif()

if(USE_LIBUSB)
    pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)

    list(APPEND LIB_INCLUDE_DIRS
        "${libusb_INCLUDE_DIRS}"
    )
    list(APPEND CLI_LIBRARIES
        PkgConfig::libusb
    )
    list(APPEND PKGCONFIG_DEPS
        "libusb-1.0"
    )
endif()

check_type_size("ssize_t" "SIZEOF_SIZE_T")
if(SIZEOF_SIZE_T STREQUAL "")
    check_type_size("size_t" "SIZEOF_SIZE_T" BUILTIN_TYPES_ONLY)
endif()
check_type_size("void*" "SIZEOF_UINTPTR_T")

check_include_file("linux/serial.h" "HAVE_LINUX_SERIAL_H")
if(WIN32)
    try_compile("HAVE_DDK_NTDDSCSI_H"
        SOURCE_FROM_FILE "main.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/misc/ntddscsi_example.c")
endif()

add_compile_definitions(
    SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
    SIZEOF_UINTPTR_T=${SIZEOF_UINTPTR_T}
    HAVE_LINUX_SERIAL_H=${HAVE_LINUX_SERIAL_H}
    HAVE_DDK_NTDDSCSI_H=${HAVE_DDK_NTDDSCSI_H}
)

# ---
# Feature definitions.
# ---

if(CAHUTE_FEATURE_LIB_HEADERS)
    FILE(GLOB_RECURSE CAHUTE_HEADERS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "*.h")

    add_custom_target(
        cahute-config-header
        COMMAND ${CMAKE_COMMAND}
            -DCAHUTE_CONFIG_BUILD_PATH="${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
            -DPROJECT_VERSION="${PROJECT_VERSION}"
            -DPROJECT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
            -DPROJECT_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
            -DPROJECT_HOMEPAGE_URL="${PROJECT_HOMEPAGE_URL}"
            -P misc/CahuteConfig.cmake
        DEPENDS include/cahute/config.h.in
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_custom_target(cahute-headers
        SOURCES
            include/cahute/cdefs.h
            "${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
            include/cahute/context.h
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cahute-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(CAHUTE_FEATURE_LIB_STATIC)
    add_custom_target(
        cahute-version-data-header
        COMMAND ${CMAKE_COMMAND}
            -DCAHUTE_GIT="${CAHUTE_GIT}"
            -DCAHUTE_GIT_CACHE_PATH="${CMAKE_CURRENT_BINARY_DIR}/git-hash.txt"
            -DCAHUTE_VERSION_DATA_BUILD_PATH="${CMAKE_CURRENT_BINARY_DIR}/common/version_data.h"
            -DPROJECT_VERSION="${PROJECT_VERSION}"
            -DPROJECT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
            -DPROJECT_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
            -DPROJECT_HOMEPAGE_URL="${PROJECT_HOMEPAGE_URL}"
            -DCAHUTE_REPORT_URL="${CAHUTE_REPORT_URL}"
            -P misc/CahuteVersionData.cmake
        DEPENDS common/version_data.h.in
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/common/version_data.h"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/chars/process_chars.py
            "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        DEPENDS chars/process_chars.py chars/chars.toml
        VERBATIM
    )

    set(LIB_SOURCES
        lib/file/checksum.c
        lib/file/close.c
        lib/file/create.c
        lib/file/data.c
        lib/file/examine.c
        lib/file/open.c
        lib/file/open_memory.c
        lib/file/open_stdout.c
        lib/file/read.c
        lib/file/size.c
        lib/file/write.c
        lib/link/open/init.c
        lib/link/open/serial.c
        lib/link/open/simple_usb.c
        lib/link/open/usb.c
        lib/link/transport/receive.c
        lib/link/transport/scsi_request_from.c
        lib/link/transport/scsi_request_to.c
        lib/link/transport/send.c
        lib/link/transport/set_serial_params.c
        lib/link/backup_rom.c
        lib/link/check.c
        lib/link/close.c
        lib/link/copy_file_on_storage.c
        lib/link/delete_file_from_storage.c
        lib/link/flash_system_using_fxremote_method.c
        lib/link/get_device_info.c
        lib/link/list_storage_entries.c
        lib/link/negotiate_serial_params.c
        lib/link/optimize_storage.c
        lib/link/receive.c
        lib/link/receive_data.c
        lib/link/receive_screen.c
        lib/link/request_file_from_storage.c
        lib/link/request_storage_capacity.c
        lib/link/reset_storage.c
        lib/link/send.c
        lib/link/send_file_to_storage.c
        lib/link/set_serial_params.c
        lib/link/upload_and_run_program.c
        lib/cas40.c
        lib/cas50.c
        lib/cas100.c
        lib/cas300.c
        lib/casiolink.c
        lib/cdefs.c
        "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        lib/context.c
        lib/data.c
        lib/detect_serial.c
        lib/detect_usb.c
        lib/info.c
        lib/logging.c
        lib/mcs.c
        lib/misc.c
        lib/path.c
        lib/picture.c
        lib/seven.c
        lib/seven_ohp.c
        lib/text.c
    )

    if(WIN32)
        list(APPEND LIB_COMPILE_DEFS
            CAHUTE_PLATFORM_WIN32=1)
        list(APPEND LIB_SOURCES
            lib/platform/win32/file/close.c
            lib/platform/win32/file/create.c
            lib/platform/win32/file/move.c
            lib/platform/win32/file/open.c
            lib/platform/win32/file/read.c
            lib/platform/win32/file/stdout.c
            lib/platform/win32/file/write.c
            lib/platform/win32/misc/cfgmgr32.c
            lib/platform/win32/misc/devices.c
            lib/platform/win32/misc/dll.c
            lib/platform/win32/misc/guid.c
            lib/platform/win32/misc/guids.c
            lib/platform/win32/misc/logging.c
            lib/platform/win32/misc/monotonic.c
            lib/platform/win32/misc/sleep.c
            lib/platform/win32/misc/version.c
            lib/platform/win32/misc/winusb.c
            lib/platform/win32/serial/detection.c
            lib/platform/win32/serial/link.c
            lib/platform/win32/usb/cesg.c
            lib/platform/win32/usb/detection.c
            lib/platform/win32/usb/devices.c
            lib/platform/win32/usb/open.c
            lib/platform/win32/usb/ums.c
            lib/platform/win32/usb/winusb_bulk.c)
    elseif(DEFINED AMIGA)
        list(APPEND LIB_COMPILE_DEFS
            CAHUTE_PLATFORM_AMIGAOS=1)
        list(APPEND LIB_SOURCES
            lib/platform/amigaos/misc/device_path.c
            lib/platform/amigaos/misc/monotonic.c
            lib/platform/amigaos/misc/sleep.c
            lib/platform/amigaos/misc/timer.c
            lib/platform/amigaos/serial/detection.c
            lib/platform/amigaos/serial/link.c)
    elseif(UNIX)
        if(LINUX)
            list(APPEND LIB_COMPILE_DEFS
                CAHUTE_PLATFORM_LINUX=1)
            list(APPEND LIB_SOURCES
                lib/platform/linux/serial_detection.c)
        endif()

        list(APPEND LIB_COMPILE_DEFS
            CAHUTE_PLATFORM_POSIX=1)
        list(APPEND LIB_SOURCES
            lib/platform/posix/file.c
            lib/platform/posix/file_create.c
            lib/platform/posix/file_open.c
            lib/platform/posix/misc.c
            lib/platform/posix/path.c
            lib/platform/posix/serial.c
            lib/platform/posix/serial_detection.c
            lib/platform/posix/stdout.c)
    endif()

    if(USE_LIBUSB)
        list(APPEND LIB_COMPILE_DEFS
            CAHUTE_PLATFORM_LIBUSB=1)
        list(APPEND LIB_SOURCES
            lib/platform/libusb/bulk.c
            lib/platform/libusb/close.c
            lib/platform/libusb/context.c
            lib/platform/libusb/detection.c
            lib/platform/libusb/open.c
            lib/platform/libusb/ums.c)
    endif()

    add_library(cahute STATIC ${LIB_SOURCES})

    add_dependencies(cahute cahute-headers cahute-version-data-header)
    target_include_directories(cahute PRIVATE
        ${LIB_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/lib")
    target_compile_definitions(cahute PRIVATE ${LIB_COMPILE_DEFS})

    install(TARGETS cahute ARCHIVE)
endif()

if(CAHUTE_FEATURE_LIB_PKGCONF)
    configure_file(misc/cahute.pc.in misc/cahute.pc ESCAPE_QUOTES @ONLY)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute.pc
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    )
endif()

if(CAHUTE_FEATURE_UDEV)
    configure_file(
        misc/60-casio-calculators.rules.in
        misc/60-casio-calculators.rules
        ESCAPE_QUOTES
        @ONLY)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/60-casio-calculators.rules
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/udev/rules.d"
    )
endif()

if(CAHUTE_FEATURE_CLI_CAS)
    add_executable(CaS
        cli/cas.c
        cli/cas_args.c
        cli/casrc.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(CaS PRIVATE ${CLI_LIBRARIES})
    target_include_directories(CaS PRIVATE ${CLI_INCLUDE_DIRS})

    install(TARGETS CaS RUNTIME)
endif()

if(CAHUTE_FEATURE_CLI_P7)
    add_executable(p7
        cli/p7.c
        cli/p7_args.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(p7 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7 PRIVATE ${CLI_INCLUDE_DIRS})

    install(TARGETS p7 RUNTIME)
endif()

if(CAHUTE_FEATURE_CLI_P7OS)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/misc/process_bin.py
            "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
            "${CMAKE_SOURCE_DIR}/cli/fxremote-update.exe.bin"
        DEPENDS cli/fxremote-update.exe.bin misc/process_bin.py
        VERBATIM
    )

    add_executable(p7os
        cli/p7os.c
        cli/p7os_args.c
        cli/common.c
        cli/options.c
        "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
    )
    target_link_libraries(p7os PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7os PRIVATE ${CLI_INCLUDE_DIRS})

    install(TARGETS p7os RUNTIME)
endif()

if(CAHUTE_FEATURE_CLI_P7SCREEN)
    add_executable(p7screen
        cli/p7screen.c
        cli/p7screen_args.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(p7screen PRIVATE
        ${CLI_LIBRARIES}
        ${SDL2_LIBRARIES}
    )
    target_include_directories(p7screen PRIVATE
        ${CLI_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )

    install(TARGETS p7screen RUNTIME)
endif()

if(CAHUTE_FEATURE_CLI_XFER9860)
    add_executable(xfer9860
        cli/xfer9860.c
        cli/xfer9860_args.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(xfer9860 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(xfer9860 PRIVATE ${CLI_INCLUDE_DIRS})

    install(TARGETS xfer9860 RUNTIME)
endif()
