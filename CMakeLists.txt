if(WIN32)
    # CMake 3.24 is required to read the Windows registry.
    cmake_minimum_required(VERSION 3.24)
else()
    cmake_minimum_required(VERSION 3.16)
endif()

project(
    cahute
    VERSION 0.5
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
if(DEFINED FXSDK_PLATFORM OR DEFINED AMIGA)
    set_property(CACHE CAHUTE_LIBUSB PROPERTY VALUE OFF)
endif()

set(CAHUTE_PKGCONF ON CACHE BOOL
    "Enable installation of pkgconf files?")

set(CAHUTE_REPORT_URL "https://cahuteproject.org/guides/report.html"
    CACHE STRING "Bug report guide URL?")

set(CAHUTE_SDL ON CACHE BOOL
    "Enable usage of SDL?")
if(DEFINED FXSDK_PLATFORM OR DEFINED AMIGA)
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

# CMake defines "-O3" by default for the Release target.
# This is very unsafe, so we force "-O2" instead.
set(CMAKE_C_FLAGS_RELEASE -O2)

# We want to add as much warnings as we can to all targets.
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Wno-c99-extensions -Wc23-extensions
    )
else()
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

include(GNUInstallDirs)
include(CheckTypeSize)

if(CAHUTE_LIBUSB)
    find_package(PkgConfig REQUIRED)
endif()

find_package(Python3 REQUIRED)

if(WIN32 OR MINGW)
    if(
        CMAKE_HOST_WIN32
        AND CMAKE_GENERATOR MATCHES "Visual Studio"
        AND "${CMAKE_GENERATOR_TOOLSET}" MATCHES ".*_xp"
    )
        # In case of VS toolset v141_xp for Visual Studio, an incorrect
        # Windows SDK is loaded that may not include <wdmguid.h> or
        # cfgmgr32.lib, so we add them manually.
        string(REGEX REPLACE "-.*" "" WIN_SDK_ARCH "${VCPKG_TARGET_TRIPLET}")

        # Start by trying to match the Windows 8.1 SDK.
        cmake_host_system_information(
            RESULT WIN_SDK_PATH
            QUERY WINDOWS_REGISTRY "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1"
            VALUE "InstallationFolder"
        )
        string(REGEX REPLACE "[\\/]$" "" WIN_SDK_PATH "${WIN_SDK_PATH}")

        if(NOT WIN_SDK_PATH STREQUAL "" AND EXISTS "${WIN_SDK_PATH}/Include/shared/wdmguid.h")
            set(WIN_SDK_INCLUDE_DIR "${WIN_SDK_PATH}/Include/shared")
            set(WIN_SDK_LIBRARY_DIR "${WIN_SDK_PATH}/Lib/winv6.3/um/${WIN_SDK_ARCH}")
            set(WIN_SDK_CFGMGR32_LIB "cfgmgr32")
        endif()

        # Otherwise, try to match the Windows 7 SDK.
        if(NOT DEFINED WIN_SDK_INCLUDE_DIR)
            foreach(WIN_SDK_MAJOR IN ITEMS "V7.1A\\XPSupport" "V7.1A")
                cmake_host_system_information(
                    RESULT WIN_SDK_PATH
                    QUERY WINDOWS_REGISTRY "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\${CAHUTE_WIN_SDK_MAJOR}"
                    VALUE "InstallationFolder"
                )
                string(REGEX REPLACE "[\\/]$" "" WIN_SDK_PATH "${WIN_SDK_PATH}")

                if(NOT WIN_SDK_PATH STREQUAL "" AND EXISTS "${WIN_SDK_PATH}/Include/wdmguid.h")
                    set(WIN_SDK_INCLUDE_DIR "${WIN_SDK_PATH}/Include")
                    if("${WIN_SDK_ARCH}" EQUALS "x64")
                        set(WIN_SDK_LIBRARY_DIR "${WIN_SDK_PATH}/Lib/x64")
                    else()
                        set(WIN_SDK_LIBRARY_DIR "${WIN_SDK_PATH}/Lib")
                    endif()
                    set(WIN_SDK_CFGMGR32_LIB "setupapi")
                endif()
            endforeach()
        endif()

        # If Windows 8.1 SDK was not found, we try to match the Windows 10 SDK.
        if(NOT DEFINED WIN_SDK_INCLUDE_DIR)
            cmake_host_system_information(
                RESULT WIN_SDK_PATH
                QUERY WINDOWS_REGISTRY "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0"
                VALUE "InstallationFolder"
            )
            string(REGEX REPLACE "[\\/]$" "" WIN_SDK_PATH "${WIN_SDK_PATH}")

            # Both "Include/" and "Lib/" are subdivided with the SDK version, so we want to look
            # for the first available version that actually works and go with that.
            file(GLOB WIN_SDK_VERSIONS LIST_DIRECTORIES TRUE RELATIVE "${WIN_SDK_PATH}/Include" "${WIN_SDK_PATH}/Include/*")
            foreach(WIN_SDK_VERSION IN LISTS WIN_SDK_VERSIONS)
                if(
                    EXISTS "${WIN_SDK_PATH}/Include/${WIN_SDK_VERSION}/shared/wdmguid.h"
                    AND EXISTS "${WIN_SDK_PATH}/Lib/${WIN_SDK_VERSION}/um/${WIN_SDK_ARCH}/cfgmgr32.lib"
                )
                    set(WIN_SDK_INCLUDE_DIR "${WIN_SDK_PATH}/Include/${WIN_SDK_VERSION}/shared")
                    set(WIN_SDK_LIBRARY_DIR "${WIN_SDK_PATH}/Lib/${WIN_SDK_VERSION}/um/${WIN_SDK_ARCH}")
                    break()
                endif()
            endforeach()

            set(WIN_SDK_CFGMGR32_LIB "cfgmgr32")
        endif()

        if(NOT DEFINED WIN_SDK_INCLUDE_DIR)
            message(FATAL_ERROR "Windows SDK could not be found.")
        endif()

        set(WIN_SDK_CFGMGR32_LIB "${WIN_SDK_LIBRARY_DIR}/${WIN_SDK_CFGMGR32_LIB}.lib")

        message("Using the following overrides for Windows SDK:")
        message("* cfgmgr32 library: ${WIN_SDK_CFGMGR32_LIB}")
        message("* include directory: ${WIN_SDK_INCLUDE_DIR}")

        list(APPEND CLI_LIBRARIES "${WIN_SDK_CFGMGR32_LIB}")
        list(APPEND LIB_INCLUDE_DIRS "${WIN_SDK_INCLUDE_DIR}")
    else()
        list(APPEND CLI_LIBRARIES cfgmgr32)
    endif()

    list(APPEND CLI_LIBRARIES uuid)
    set(SDL2_NO_MWINDOWS 1)
endif()

if(CAHUTE_FEATURE_CLI_P7SCREEN)
    find_package(SDL2 REQUIRED)
endif()

if(CAHUTE_LIBUSB)
    if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
        # With vcpkg, libusb <=1.0.23 does not include a pkgconf file,
        # so we need to add the options manually, hoping it will be enough.
        pkg_check_modules(libusb libusb-1.0 IMPORTED_TARGET)
        if(NOT TARGET PkgConfig::libusb)
            list(APPEND LIB_INCLUDE_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/libusb-1.0")
            list(APPEND CLI_LIBRARIES "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libusb-1.0.lib")
            list(APPEND PKGCONFIG_LIBS "-lusb-1.0")
        endif()
    else()
        pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)
    endif()

    if(TARGET PkgConfig::libusb)
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

    list(APPEND LIB_COMPILE_DEFS
        "-DLIBUSB_ENABLED=1"
    )
else()
    list(APPEND LIB_COMPILE_DEFS
        "-DLIBUSB_ENABLED=0"
    )
endif()

check_type_size("ssize_t" "SIZEOF_SIZE_T")
if(SIZEOF_SIZE_T STREQUAL "")
    check_type_size("size_t" "SIZEOF_SIZE_T" BUILTIN_TYPES_ONLY)
endif()
check_type_size("void*" "SIZEOF_UINTPTR_T")

check_include_file("linux/serial.h" "HAVE_LINUX_SERIAL_H")

add_compile_definitions(
    SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
    SIZEOF_UINTPTR_T=${SIZEOF_UINTPTR_T}
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
            -DCAHUTE_GIT="${CAHUTE_GIT}"
            -DCAHUTE_CONFIG_CHECK=1
            -DCAHUTE_GIT_CACHE_PATH="${CMAKE_CURRENT_BINARY_DIR}/git-hash.txt"
            -DCAHUTE_CONFIG_BUILD_PATH="${CMAKE_CURRENT_BINARY_DIR}/include/cahute/config.h"
            -DPROJECT_VERSION="${PROJECT_VERSION}"
            -DPROJECT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
            -DPROJECT_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
            -DPROJECT_HOMEPAGE_URL="${PROJECT_HOMEPAGE_URL}"
            -DCAHUTE_REPORT_URL="${CAHUTE_REPORT_URL}"
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
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/chars/process_chars.py
            "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        DEPENDS chars/process_chars.py chars/chars.toml
        VERBATIM
    )

    add_library(cahute STATIC
        lib/cas40.c
        lib/cas50.c
        lib/cas100.c
        lib/cas300.c
        lib/casiolink.c
        lib/cdefs.c
        "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
        lib/context.c
        lib/data.c
        lib/detection.c
        lib/file.c
        lib/filemedium.c
        lib/fileopen.c
        lib/link.c
        lib/linkmedium.c
        lib/linkopen.c
        lib/logging.c
        lib/mcs.c
        lib/misc.c
        lib/path.c
        lib/picture.c
        lib/seven.c
        lib/seven_ohp.c
        lib/text.c
    )
    add_dependencies(cahute cahute-headers)
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
