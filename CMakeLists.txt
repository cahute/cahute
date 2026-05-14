cmake_minimum_required(VERSION 3.21)
project(
    cahute
    VERSION 0.6
    DESCRIPTION "Communication and file format handling tools for CASIO calculators"
    HOMEPAGE_URL "https://cahute.org/"
    LANGUAGES C
)
set(PROJECT_CONTACT "Thomas Touhey <thomas@touhey.fr>")

# PROJECT_VERSION_PUBLIC represents the version namespace in which Cahute will
# be placed when installed:
# * If we are in 0.x, the public version will be "0.x", e.g. "0.6".
# * If we are in 1.0 or above, the public version will only be the major
#   version, e.g. "1" for "1.4" or "3" for "3.0".
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(PROJECT_VERSION_PUBLIC ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
else()
    set(PROJECT_VERSION_PUBLIC ${PROJECT_VERSION_MAJOR})
endif()

# ---
# Options.
# ---

include(GNUInstallDirs)
include(CMakeDependentOption)

set(CAHUTE_APPSTREAM OFF CACHE BOOL
    "Include AppStream metadata?")

set(CAHUTE_CLI ON CACHE BOOL
    "Enable command-line utilities?")

set(CAHUTE_CLI_EXPERIMENTAL OFF CACHE BOOL
    "Enable experimental command-line utilities?")

set(CAHUTE_CLI_LIBRARY_TYPE "auto" CACHE STRING
    "Which library type should the CLI be built against?")
set_property(CACHE CAHUTE_CLI_LIBRARY_TYPE PROPERTY STRINGS "auto" "shared" "static")

set(CAHUTE_CLI_PIE OFF CACHE BOOL
    "Enable compiling the CLI utilities as position-independent executables (PIE)?")

set(CAHUTE_CLI_RUNTIME_DEPS OFF CACHE BOOL
    "Enable installation of runtime dependencies with command-line utilities?")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_DEFAULT_LOGLEVEL "debug")
else()
    set(_DEFAULT_LOGLEVEL "warning")
endif()

set(CAHUTE_DEFAULT_LOGLEVEL "${_DEFAULT_LOGLEVEL}" CACHE STRING
    "Default logging level used when creating a context?")
set_property(CACHE CAHUTE_DEFAULT_LOGLEVEL PROPERTY STRINGS
    "debug" "info" "warning" "error" "fatal" "none")

set(CAHUTE_GIT ON CACHE BOOL
    "Include git-related information if available?")

set(CAHUTE_INSTALL_UDEVDIR "${CMAKE_INSTALL_LIBDIR}/udev" CACHE STRING
    "Path in which to install udev rules?")

if("${CMAKE_SYSTEM_NAME}" MATCHES "Android|Darwin|Haiku|iOS|Linux|NetBSD|OpenBSD|SunOS")
    set(_DEFAULT_LIBUSB ON)
else()
    set(_DEFAULT_LIBUSB OFF)
endif()

set(CAHUTE_LIBUSB ${_DEFAULT_LIBUSB} CACHE BOOL
    "Enable usage of libusb?")

set(CAHUTE_PKGCONF ON CACHE BOOL
    "Enable installation of pkgconf files?")

set(CAHUTE_REPORT_URL "https://cahute.org/guides/report.html"
    CACHE STRING "Bug report guide URL?")

if("${CMAKE_SYSTEM_NAME}" MATCHES "Android|Darwin|iOS|Linux|Windows$")
    set(_DEFAULT_SDL ON)
else()
    set(_DEFAULT_SDL OFF)
endif()

set(CAHUTE_SDL ${_DEFAULT_SDL} CACHE BOOL
    "Enable usage of SDL?")

if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows$")
    set(_DEFAULT_SHARED ON)
else()
    set(_DEFAULT_SHARED OFF)
endif()

set(CAHUTE_SHARED ${_DEFAULT_SHARED} CACHE BOOL
    "Enable compiling the library as shared?")

set(CAHUTE_STATIC ON CACHE BOOL
    "Enable compiling the library as position-dependent static?")

set(CAHUTE_STATIC_PIC OFF CACHE BOOL
    "Enable compiling the library as position-independent static?")

if(LINUX)
    set(_DEFAULT_UDEV ON)
else()
    set(_DEFAULT_UDEV OFF)
endif()

set(CAHUTE_UDEV ${_DEFAULT_UDEV} CACHE BOOL
    "Enable installation of udev rule?")

set(CAHUTE_UDEV_GROUP "uucp" CACHE STRING
    "Name of the group inserted into the udev rule?")

# ---
# Command-line utilities.
# ---

cmake_dependent_option(CAHUTE_CLI_CAS
    "Enable CaS command-line utility?"
    ON "CAHUTE_CLI AND CAHUTE_CLI_EXPERIMENTAL" OFF)

cmake_dependent_option(CAHUTE_CLI_P7
    "Enable p7 command-line utility?"
    ON "CAHUTE_CLI" OFF)

cmake_dependent_option(CAHUTE_CLI_P7OS
    "Enable p7os command-line-utility?"
    ON "CAHUTE_CLI" OFF)

cmake_dependent_option(CAHUTE_CLI_P7SCREEN
    "Enable p7screen command-line-utility?"
    ON "CAHUTE_CLI AND CAHUTE_SDL" OFF)

cmake_dependent_option(CAHUTE_CLI_XFER9860
    "Enable xfer9860 command-line-utility?"
    ON "CAHUTE_CLI" OFF)

mark_as_advanced(
    CAHUTE_CLI_CAS
    CAHUTE_CLI_P7
    CAHUTE_CLI_P7OS
    CAHUTE_CLI_P7SCREEN
    CAHUTE_CLI_XFER9860)

# ---
# Common definitions.
# ---
# USE_LIBUSB represents whether to include and use libusb.
# It is disabled on Win32 because we manage some drivers libusb does not,
# that are more important in our case (CESG502, mostly).
if(CAHUTE_LIBUSB AND NOT WIN32)
    set(USE_LIBUSB ON)
else()
    set(USE_LIBUSB OFF)
endif()

if(CAHUTE_CLI_P7SCREEN)
    set(USE_SDL ON)
else()
    set(USE_SDL OFF)
endif()

include(CheckTypeSize)

if(CAHUTE_CLI_PIE OR CAHUTE_STATIC_PIC OR CAHUTE_SHARED)
    include(CheckPIESupported)

    check_pie_supported()
endif()

if(USE_LIBUSB)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)
endif()

find_package(Python3 REQUIRED)

# GET_RUNTIME_DEPENDENCIES uses system introspection for determining the
# dependencies, which does not work well with cross-compiling.
# We set the platform manually.
if(WIN32)
    set(SDL2_NO_MWINDOWS 1)
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
elseif(APPLE OR IOS)
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "macos+macho")
elseif(LINUX)
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "linux+elf")
endif()

if(CAHUTE_CLI_RUNTIME_DEPS AND DEFINED CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM)
    set(INSTALL_RUNTIME_DEPS ON)
else()
    set(INSTALL_RUNTIME_DEPS OFF)
endif()

if(USE_SDL)
    find_package(SDL2 REQUIRED)
endif()

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

# CLI_CAHUTE_DEP_VAR represents the name of the variable we need to check.
# CLI_CAHUTE_DEP represents the name of the target used by the CLI utilities.
if(CAHUTE_CLI_LIBRARY_TYPE STREQUAL "auto")
    if(CAHUTE_SHARED)
        set(CLI_CAHUTE_DEP_VAR "CAHUTE_SHARED")
        set(CLI_CAHUTE_DEP "cahute-shared")
    else()
        if(CAHUTE_CLI_PIE)
            set(CLI_CAHUTE_DEP_VAR "CAHUTE_STATIC_PIC")
            set(CLI_CAHUTE_DEP "cahute-static-pic")
        else()
            set(CLI_CAHUTE_DEP_VAR "CAHUTE_STATIC")
            set(CLI_CAHUTE_DEP "cahute-static")
        endif()

        if(NOT ${${CLI_CAHUTE_DEP_VAR}})
            message(FATAL_ERROR
                "CLI utilities' library type \"${CAHUTE_CLI_LIBRARY_TYPE}\" "
                "require that you either set CAHUTE_SHARED or "
                "${CLI_CAHUTE_DEP_VAR} to ON.")
        endif()
    endif()
elseif(CAHUTE_CLI_LIBRARY_TYPE STREQUAL "shared")
    set(CLI_CAHUTE_DEP_VAR "CAHUTE_SHARED")
    set(CLI_CAHUTE_DEP "cahute-shared")
elseif(CAHUTE_CLI_PIE)
    set(CLI_CAHUTE_DEP_VAR "CAHUTE_STATIC_PIC")
    set(CLI_CAHUTE_DEP "cahute-static-pic")
else()
    set(CLI_CAHUTE_DEP_VAR "CAHUTE_STATIC")
    set(CLI_CAHUTE_DEP "cahute-static")
endif()

if(NOT ${${CLI_CAHUTE_DEP_VAR}})
    message(FATAL_ERROR
        "CLI utilities' library type \"${CAHUTE_CLI_LIBRARY_TYPE}\" require "
        "that you set ${CLI_CAHUTE_DEP_VAR} to ON.")
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
set(LIB_COMPILE_DEFS
    CAHUTE_DEFAULT_LOGLEVEL=CAHUTE_LOGLEVEL_${CAHUTE_DEFAULT_LOGLEVEL_UPPER}
    CAHUTE_IS_INTERNAL=1)

# LIB_SOURCES represents the names of the sources used to compile the library.
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
    lib/link/cas40/decode_data.c
    lib/link/cas40/decode_data_using_description.c
    lib/link/cas40/determine_data_description.c
    lib/link/cas40/receive_data.c
    lib/link/cas40/receive_raw_data.c
    lib/link/cas40/receive_screen.c
    lib/link/cas40/terminate.c
    lib/link/cas50/decode_data.c
    lib/link/cas50/decode_data_using_description.c
    lib/link/cas50/determine_data_description.c
    lib/link/cas50/receive_data.c
    lib/link/cas50/receive_raw_data.c
    lib/link/cas50/terminate.c
    lib/link/cas100/decode_data_using_description.c
    lib/link/cas100/determine_data_description.c
    lib/link/cas100/exchange_model_information.c
    lib/link/cas100/handle_mdl1.c
    lib/link/cas100/obtain_device_info.c
    lib/link/cas100/receive_data.c
    lib/link/cas100/receive_raw_data.c
    lib/link/cas100/send_model_information.c
    lib/link/cas100/terminate.c
    lib/link/cas300/discover.c
    lib/link/cas300/initiate_as_receiver.c
    lib/link/cas300/initiate_as_sender.c
    lib/link/cas300/obtain_device_info.c
    lib/link/cas300/receive_data.c
    lib/link/cas300/receive_packet.c
    lib/link/cas300/send_command.c
    lib/link/cas300/send_data_packet.c
    lib/link/cas300/terminate.c
    lib/link/casiolink/check_file_data.c
    lib/link/casiolink/decode_data.c
    lib/link/casiolink/determine_header_variant.c
    lib/link/casiolink/initiate_as_receiver.c
    lib/link/casiolink/initiate_as_sender.c
    lib/link/casiolink/log_data_description.c
    lib/link/casiolink/receive_data.c
    lib/link/casiolink/receive_first_byte.c
    lib/link/casiolink/receive_packet.c
    lib/link/casiolink/receive_raw_data.c
    lib/link/seven/backup_rom.c
    lib/link/seven/copy_file_on_storage.c
    lib/link/seven/decode_command.c
    lib/link/seven/delete_file_from_storage.c
    lib/link/seven/discover.c
    lib/link/seven/flash_system_using_fxremote_method.c
    lib/link/seven/initialize.c
    lib/link/seven/list_storage_entries.c
    lib/link/seven/negotiate_serial_params.c
    lib/link/seven/obtain_device_info.c
    lib/link/seven/optimize_storage.c
    lib/link/seven/receive_bytes.c
    lib/link/seven/receive_bytes_into_stream.c
    lib/link/seven/receive_data.c
    lib/link/seven/receive_packet.c
    lib/link/seven/request_file_from_storage.c
    lib/link/seven/request_file_type.c
    lib/link/seven/request_storage_capacity.c
    lib/link/seven/reset_storage.c
    lib/link/seven/send_basic_packet.c
    lib/link/seven/send_bytes.c
    lib/link/seven/send_bytes_from_stream.c
    lib/link/seven/send_command.c
    lib/link/seven/send_extended_packet.c
    lib/link/seven/send_file_to_storage.c
    lib/link/seven/send_packet.c
    lib/link/seven/terminate.c
    lib/link/seven/upload_and_run_program.c
    lib/link/seven_ohp/receive_packet.c
    lib/link/seven_ohp/receive_screen.c
    lib/link/seven_ohp/send_basic.c
    lib/link/backup_rom.c
    lib/link/check.c
    lib/link/close.c
    lib/link/copy_file_on_storage.c
    lib/link/delete_file_from_storage.c
    lib/link/flash_system_using_fxremote_method.c
    lib/link/get_device_property.c
    lib/link/get_protocol_name.c
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
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows3x")
    list(APPEND LIB_COMPILE_DEFS
        CAHUTE_PLATFORM_WIN16=1)
    list(APPEND LIB_SOURCES
        lib/platform/win16/serial/detection.c
        lib/platform/win16/serial/link.c
        lib/platform/win16/monotonic.c
        lib/platform/win16/sleep.c)
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
        lib/platform/posix/stderr_tty.c
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
    ${CLI_CAHUTE_DEP})

# CLI_COMPILE_DEFS defines the compile definitions.
set(CLI_COMPILE_DEFS
    CAHUTE_STATIC=0)
if(CLI_CAHUTE_DEP STREQUAL "cahute-static")
    set(CLI_COMPILE_DEFS
        CAHUTE_STATIC=1)
endif()

# PKGCONFIG_DEPS represent the additional dependencies to place in the
# pkgconf / pkg-config configuration file when installing the library.
set(PKGCONFIG_DEPS "")

# PKGCONFIG_LIBS represent the additional includes to place in the
# pkgconf / pkg-config configuration file when installing the library.
set(PKGCONFIG_LIBS "")

if(USE_LIBUSB)
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
if(WIN32 AND WATCOM)
    # The DDK is not included by default with OpenWatcom; we want to add it.
    include_directories("$ENV{WATCOM}/h/nt/ddk")
endif()

add_compile_definitions(
    SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
    SIZEOF_UINTPTR_T=${SIZEOF_UINTPTR_T}
    HAVE_LINUX_SERIAL_H=${HAVE_LINUX_SERIAL_H}
    HAVE_DDK_NTDDSCSI_H=${HAVE_DDK_NTDDSCSI_H}
)

# ---
# "cahute-headers" target.
# ---

FILE(GLOB_RECURSE CAHUTE_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "*.h")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
    COMMAND ${CMAKE_COMMAND}
        -DCAHUTE_CONFIG_BUILD_PATH="${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
        -DPROJECT_VERSION="${PROJECT_VERSION}"
        -DPROJECT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
        -DPROJECT_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
        -DPROJECT_HOMEPAGE_URL="${PROJECT_HOMEPAGE_URL}"
        -P misc/CahuteConfig.cmake
    DEPENDS include/cahute/config.h.in misc/CahuteConfig.cmake
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(cahute-headers
    SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cahute-${PROJECT_VERSION_PUBLIC}"
    COMPONENT lib_dev
    FILES_MATCHING PATTERN "*.h"
)

# ---
# "cahute-static" and "cahute-shared" (library) targets.
# ---

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/common/version_data.h"
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
    DEPENDS common/version_data.h.in misc/CahuteVersionData.cmake
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
list(APPEND LIB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/common/version_data.h")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
    COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/chars/process_chars.py
        "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
    DEPENDS chars/process_chars.py chars/chars.toml
    VERBATIM
)

if(CAHUTE_SHARED OR CAHUTE_STATIC_PIC)
    add_library(cahute-pic OBJECT ${LIB_SOURCES})
    set_property(TARGET cahute-pic PROPERTY POSITION_INDEPENDENT_CODE ON)
    add_dependencies(cahute-pic cahute-headers)
    target_include_directories(cahute-pic PRIVATE
        ${LIB_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/lib")
    target_compile_definitions(cahute-pic PRIVATE
        ${LIB_COMPILE_DEFS})
endif()

if(CAHUTE_STATIC)
    add_library(cahute-pdc OBJECT ${LIB_SOURCES})
    add_dependencies(cahute-pdc cahute-headers)
    target_include_directories(cahute-pdc PRIVATE
        ${LIB_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/lib")
    target_compile_definitions(cahute-pdc PRIVATE
        ${LIB_COMPILE_DEFS})
endif()

if(CAHUTE_STATIC)
    add_library(cahute-static STATIC $<TARGET_OBJECTS:cahute-pdc>)
    set_target_properties(cahute-static PROPERTIES OUTPUT_NAME "cahute-static")

    if(CAHUTE_PKGCONF)
        configure_file(misc/cahute-static.pc.in misc/cahute-static.pc ESCAPE_QUOTES @ONLY)

        install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute-static.pc
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
            COMPONENT lib_dev
            RENAME "cahute-${PROJECT_VERSION_PUBLIC}-static.pc"
        )
    endif()

    install(TARGETS cahute-static
        ARCHIVE
            COMPONENT lib_dev)
endif()

if(CAHUTE_STATIC_PIC)
    add_library(cahute-static-pic STATIC $<TARGET_OBJECTS:cahute-pic>)
    set_target_properties(cahute-static-pic PROPERTIES OUTPUT_NAME "cahute-static-pic")

    if(CAHUTE_PKGCONF)
        configure_file(misc/cahute-static-pic.pc.in misc/cahute-static-pic.pc ESCAPE_QUOTES @ONLY)

        install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute-static-pic.pc
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
            COMPONENT lib_dev
            RENAME "cahute-${PROJECT_VERSION_PUBLIC}-static-pic.pc"
        )
    endif()

    install(TARGETS cahute-static-pic
        ARCHIVE
            COMPONENT lib_dev)
endif()

if(CAHUTE_SHARED)
    add_library(cahute-shared SHARED $<TARGET_OBJECTS:cahute-pic>)
    set_target_properties(cahute-shared PROPERTIES OUTPUT_NAME "cahute")
    set_target_properties(cahute-shared PROPERTIES SOVERSION
        "${PROJECT_VERSION_PUBLIC}")

    if(CAHUTE_PKGCONF)
        configure_file(misc/cahute-shared.pc.in misc/cahute-shared.pc ESCAPE_QUOTES @ONLY)

        install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute-shared.pc
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
            COMPONENT lib_dev
            RENAME "cahute-${PROJECT_VERSION_PUBLIC}.pc"
        )
    endif()

    install(TARGETS cahute-shared
        PERMISSIONS
            OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        RUNTIME
            COMPONENT lib_runtime
        LIBRARY
            COMPONENT lib_runtime
            NAMELINK_COMPONENT lib_dev
        ARCHIVE
            COMPONENT lib_dev)
endif()

# ---
# Additional configuration files.
# ---

if(CAHUTE_UDEV)
    configure_file(
        misc/60-casio-calculators.rules.in
        misc/60-casio-calculators.rules
        ESCAPE_QUOTES
        @ONLY)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/60-casio-calculators.rules
        DESTINATION "${CAHUTE_INSTALL_UDEVDIR}/rules.d"
        COMPONENT udev
    )
    if(CAHUTE_APPSTREAM)
        install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/org.cahute.cahute.metainfo.xml
            DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
            COMPONENT udev
        )
    endif()
endif()

# ---
# Command-line utilities.
# ---

if(CAHUTE_CLI_CAS)
    add_executable(CaS
        cli/cas.c
        cli/cas_args.c
        cli/casrc.c
        cli/common.c
        cli/options.c
    )
    if(CAHUTE_CLI_PIE)
        set_property(TARGET CaS PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(CaS PRIVATE ${CLI_LIBRARIES})
    target_include_directories(CaS PRIVATE ${CLI_INCLUDE_DIRS})
    target_compile_definitions(CaS PRIVATE ${CLI_COMPILE_DEFS})

    install(TARGETS CaS
        RUNTIME_DEPENDENCY_SET cli
        RUNTIME
            COMPONENT cli_cas)
endif()

if(CAHUTE_CLI_P7)
    add_executable(p7
        cli/p7.c
        cli/p7_args.c
        cli/common.c
        cli/options.c
    )
    if(CAHUTE_CLI_PIE)
        set_property(TARGET p7 PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(p7 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7 PRIVATE ${CLI_INCLUDE_DIRS})
    target_compile_definitions(p7 PRIVATE ${CLI_COMPILE_DEFS})

    install(TARGETS p7
        RUNTIME_DEPENDENCY_SET cli
        RUNTIME
            COMPONENT cli_p7)
endif()

if(CAHUTE_CLI_P7OS)
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
    if(CAHUTE_CLI_PIE)
        set_property(TARGET p7os PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(p7os PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7os PRIVATE ${CLI_INCLUDE_DIRS})
    target_compile_definitions(p7os PRIVATE ${CLI_COMPILE_DEFS})

    install(TARGETS p7os
        RUNTIME_DEPENDENCY_SET cli
        RUNTIME
            COMPONENT cli_p7os)
endif()

if(CAHUTE_CLI_P7SCREEN)
    add_executable(p7screen
        cli/p7screen.c
        cli/p7screen_args.c
        cli/common.c
        cli/options.c
    )
    if(CAHUTE_CLI_PIE)
        set_property(TARGET p7screen PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(p7screen PRIVATE
        ${CLI_LIBRARIES}
        ${SDL2_LIBRARIES}
    )
    target_include_directories(p7screen PRIVATE
        ${CLI_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )
    target_compile_definitions(p7screen PRIVATE ${CLI_COMPILE_DEFS})

    install(TARGETS p7screen
        RUNTIME_DEPENDENCY_SET cli
        RUNTIME
            COMPONENT cli_p7screen)
endif()

if(CAHUTE_CLI_XFER9860)
    add_executable(xfer9860
        cli/xfer9860.c
        cli/xfer9860_args.c
        cli/common.c
        cli/options.c
    )
    if(CAHUTE_CLI_PIE)
        set_property(TARGET xfer9860 PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(xfer9860 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(xfer9860 PRIVATE ${CLI_INCLUDE_DIRS})
    target_compile_definitions(xfer9860 PRIVATE ${CLI_COMPILE_DEFS})

    install(TARGETS xfer9860
        RUNTIME_DEPENDENCY_SET cli
        RUNTIME
            COMPONENT cli_xfer9860)
endif()

# ---
# Runtime dependencies.
# ---

if(INSTALL_RUNTIME_DEPS)
    # CMake's GET_RUNTIME_DEPENDENCIES for Windows+PE lowercases the DLL name
    # before looking for it in the provided directories. This works when
    # building natively on Windows, since the filesystem is case-insensitive
    # by default, but doesn't work if the actual DLL name is not lowercase
    # when cross-compiling for Windows on a system with case-sensitive paths.
    #
    # This is the case for SDL, where the DLL name is "SDL2.dll" instead of
    # "sdl2.dll"; so, instead of trying to make CMake behave correctly, we
    # will simply copy it manually if need be.
    #
    # This is documented here in CMake's Gitlab repository:
    # https://gitlab.kitware.com/cmake/cmake/-/issues/26359

    install(RUNTIME_DEPENDENCY_SET cli
        DIRECTORIES
            ${CMAKE_SYSTEM_LIBRARY_PATH}
        PRE_EXCLUDE_REGEXES
            # Win32.
            "^(advapi|kernel|shell)32\.dll$"
            "^sdl2\.dll$"
            "^(api|ext)-ms-"
            # Linux.
            "^ld-"
            "^lib(c|cap)\.so"
        COMPONENT lib_runtime)

    if(USE_SDL AND WIN32)
        get_target_property(_SDL_DLL SDL2::SDL2 IMPORTED_LOCATION)
        install(FILES "${_SDL_DLL}"
            TYPE BIN
            COMPONENT lib_runtime)
    endif()
endif()

# ---
# Packaging options.
# ---

include(CPackComponent)

set(CPACK_PACKAGE_VENDOR "Cahute project")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_CONTACT "${PROJECT_CONTACT}")
# DESCRIPTION_SUMMARY is automatically placed at the top of Debian packages'
# descriptions, and there is no way to disable it, which is annoying,
# especially for packages related to command-line utilities.
# For simplicy, we set it to blank, so it doesn't come and parasite our
# components' descriptions.
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/misc/cahute.bmp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_STRIP_FILES YES)
set(CPACK_VERBATIM_VARIABLES true)

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(
    CPACK_SOURCE_IGNORE_FILES
    "\\.git" "/build/" ".*~$" "\\.envrc" "\\.pre-commit-config\\.yaml"
    "\\.tool-versions" "\\.clang-format" "__pycache__" "\\.gitlab-ci.yml"
    "_CPack" "/docs/"
)

# By default, CPACK_PACKAGING_INSTALL_PREFIX may differ from
# CMAKE_INSTALL_PREFIX (e.g. "/usr" versus "/usr/local"), which may cause
# the pkgconf files to not reference the correct paths.
# The following set instruction fixes this problem.
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_COMPONENTS_GROUPING "IGNORE")

if(WIN32)
    set(CPACK_GENERATOR NSIS ZIP)
endif()

cpack_add_install_type(full
    DISPLAY_NAME "Full install, including the software development kit")
cpack_add_install_type(dev
    DISPLAY_NAME "Development install, only including the software development kit")
cpack_add_install_type(min
    DISPLAY_NAME "Minimal install, only including the runtime components")

set(COMP_UDEV_DEP "")
if(CAHUTE_UDEV)
    set(COMP_UDEV_DEP "udev")
    cpack_add_component(udev
        DISPLAY_NAME "libcahute device access"
        DESCRIPTION "udev rules for interacting with CASIO calculators.\nThis package provides udev rules for CASIO calculators over USB."
        INSTALL_TYPES full dev min)
endif()
cpack_add_component(lib_runtime
    DISPLAY_NAME "libcahute runtime files"
    DESCRIPTION "Library for interacting with CASIO calculators and related data\nThis package contains what you need to run programs that use this library."
    REQUIRED
    DEPENDS ${COMP_UDEV_DEP}
    INSTALL_TYPES full dev min)
cpack_add_component(lib_dev
    DISPLAY_NAME "libcahute development files"
    DESCRIPTION "Library for interacting with CASIO calculators and related data\nThis package contains what you need for compiling sources that use this library\nin your own code."
    DEPENDS lib_runtime
    INSTALL_TYPES full dev)

cpack_add_component_group(cli
    DISPLAY_NAME "Cahute command-line utilities"
    DESCRIPTION "Command-line utilities for CASIO calculators.")

cpack_add_component(cli_cas
    DISPLAY_NAME "CaS command-line utility"
    DESCRIPTION "Main memory transfer utility for CASIO calculators\nThis utility allows one to send and receive main memory data to and from a\nCASIO calculator of any generation, such as programs, lists, matrixes,\nand so on, over USB or serial cables."
    GROUP cli
    DEPENDS lib_runtime
    INSTALL_TYPES full min)
cpack_add_component(cli_p7
    DISPLAY_NAME "p7 command-line utility"
    DESCRIPTION "Storage memory transfer utility for CASIO calculators\nThis utility allows one to send and receive files to and from a CASIO\ncalculator's storage (flash or SD card) memory, over USB or serial cables."
    GROUP cli
    DEPENDS lib_runtime
    INSTALL_TYPES full min)
cpack_add_component(cli_p7os
    DISPLAY_NAME "p7os command-line utility"
    DESCRIPTION "System management utility for CASIO calculators\nThis utility allows one to flash or backup a CASIO calculator's system, over\nUSB or serial cables."
    GROUP cli
    DEPENDS lib_runtime
    INSTALL_TYPES full min)
cpack_add_component(cli_p7screen
    DISPLAY_NAME "p7screen command-line utility"
    DESCRIPTION "Virtual projector for CASIO calculators\nThis utility allows one to display the screen of a CASIO calculator in\n'Projector' or similar mode as a window, over USB cables."
    GROUP cli
    DEPENDS lib_runtime
    INSTALL_TYPES full min)
cpack_add_component(cli_xfer9860
    DISPLAY_NAME "xfer9860 command-line utility"
    DESCRIPTION "Storage memory transfer utility for CASIO calculators\nThis utility allows one to send and receive files to and from a CASIO\ncalculator's storage (flash or SD card) memory, over USB or serial cables."
    GROUP cli
    DEPENDS lib_runtime
    INSTALL_TYPES full min)

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SOURCE "cahute")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS OFF)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
if(DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    # No need to redefine it.
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(CPACK_DEBIAN_UDEV_PACKAGE_NAME "libcahute-udev")
# Section inspired by bilibop-udev and hdmi2usb-udev.
set(CPACK_DEBIAN_UDEV_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_UDEV_PACKAGE_ARCHITECTURE all)
set(CPACK_DEBIAN_UDEV_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/udev/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/udev/postrm")

set(DEB_LIB_RUNTIME_DEPS "libc6 (>=2.31)")
if(CAHUTE_UDEV)
    list(APPEND DEB_LIB_RUNTIME_DEPS
        "libcahute-udev (>=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})")
endif()

if(USE_LIBUSB)
    list(APPEND DEB_LIB_RUNTIME_DEPS "libusb-1.0-0 (>=1.0.23)")
endif()

list(JOIN DEB_LIB_RUNTIME_DEPS ", " DEB_LIB_RUNTIME_DEPS_JOINED)

configure_file(debian/runtime/shlibs.in debian/runtime/shlibs ESCAPE_QUOTES @ONLY)

set(CPACK_DEBIAN_LIB_RUNTIME_PACKAGE_NAME
    "libcahute${PROJECT_VERSION_PUBLIC}")
set(CPACK_DEBIAN_LIB_RUNTIME_PACKAGE_DEPENDS "${DEB_LIB_RUNTIME_DEPS_JOINED}")
set(CPACK_DEBIAN_LIB_RUNTIME_PACKAGE_SECTION libs)
set(CPACK_DEBIAN_LIB_RUNTIME_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/runtime/triggers;${CMAKE_CURRENT_BINARY_DIR}/debian/runtime/shlibs")

set(DEB_RUNTIME_DEP "\
libc6 (>=2.31), \
${CPACK_DEBIAN_LIB_RUNTIME_PACKAGE_NAME} \
(>=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})")

set(CPACK_DEBIAN_LIB_DEV_PACKAGE_NAME libcahute-dev)
set(CPACK_DEBIAN_LIB_DEV_PACKAGE_DEPENDS ${DEB_RUNTIME_DEP})
set(CPACK_DEBIAN_LIB_DEV_PACKAGE_SECTION libdevel)

set(CPACK_DEBIAN_CLI_CAS_PACKAGE_NAME cas)
set(CPACK_DEBIAN_CLI_CAS_PACKAGE_DEPENDS ${DEB_RUNTIME_DEP})
set(CPACK_DEBIAN_CLI_CAS_PACKAGE_SECTION comm)

set(CPACK_DEBIAN_CLI_P7_PACKAGE_NAME p7)
set(CPACK_DEBIAN_CLI_P7_PACKAGE_DEPENDS ${DEB_RUNTIME_DEP})
set(CPACK_DEBIAN_CLI_P7_PACKAGE_SECTION comm)

set(CPACK_DEBIAN_CLI_P7OS_PACKAGE_NAME p7os)
set(CPACK_DEBIAN_CLI_P7OS_PACKAGE_DEPENDS ${DEB_RUNTIME_DEP})
set(CPACK_DEBIAN_CLI_P7OS_PACKAGE_SECTION comm)

set(CPACK_DEBIAN_CLI_P7SCREEN_PACKAGE_NAME p7screen)
set(CPACK_DEBIAN_CLI_P7SCREEN_PACKAGE_DEPENDS "${DEB_RUNTIME_DEP}, libsdl2-2.0-0")
set(CPACK_DEBIAN_CLI_P7SCREEN_PACKAGE_SECTION comm)

set(CPACK_DEBIAN_CLI_XFER9860_PACKAGE_NAME xfer9860)
set(CPACK_DEBIAN_CLI_XFER9860_PACKAGE_DEPENDS ${DEB_RUNTIME_DEP})
set(CPACK_DEBIAN_CLI_XFER9860_PACKAGE_SECTION comm)

# Add copyright and changelog files for the Debian packages.
get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
foreach(COMP IN LISTS CPACK_COMPONENTS_ALL)
    string(TOUPPER "CPACK_DEBIAN_${COMP}_PACKAGE_NAME" COMP_PACKAGE_VAR)
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/${${COMP_PACKAGE_VAR}}"
        COMPONENT ${COMP}
        RENAME copyright
    )
endforeach()

set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/misc/cahute.ico")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP
    "${CMAKE_CURRENT_SOURCE_DIR}/misc/welcomefinishpage.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP
    "${CMAKE_CURRENT_SOURCE_DIR}/misc/welcomefinishpage.bmp")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_DISPLAY_NAME "Cahute")
set(CPACK_NSIS_PACKAGE_NAME "Cahute")
set(CPACK_NSIS_HELP_LINK "https://cahute.org/guides/install/windows.html")
set(CPACK_NSIS_URL_INFO_ABOUT "https://cahute.org/")

include(CPack)
