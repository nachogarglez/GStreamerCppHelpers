#
# Configure conan with modules-cmake.
#
# Authors: Manel Jimeno <manel.jimeno@gmail.com>
#
# License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
#

set(CONAN_CMAKE_VERSION
    "0.18.1"
    CACHE STRING "Conan cmake script version")
option(CONAN_BUILD_MISSING "Automatically build all missing dependencies" OFF)

# Conan's libraries setting helper
function(conan_configure)
    set(multiValueArgs REQUIRES OPTIONS FIND_PACKAGES EXPORT_VARIABLES)
    cmake_parse_arguments(CONFIG "" "" "${multiValueArgs}" ${ARGN})

    # Install conan
    find_package(Python3 3.8 QUIET REQUIRED)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pip install --user --upgrade "conan>=2.0,<3"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE COMMAND_RESULT
        OUTPUT_QUIET ERROR_QUIET)
    if(NOT COMMAND_RESULT EQUAL "0")
        message(FATAL_ERROR "Cannot install conan")
    endif()

    # Ensure default conan profile exists
    execute_process(
        COMMAND conan profile list
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RESULT_VARIABLE COMMAND_RESULT
        OUTPUT_VARIABLE PROFILE_LIST
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT COMMAND_RESULT EQUAL "0" OR NOT PROFILE_LIST MATCHES "default")
        execute_process(
            COMMAND conan profile detect --force
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            RESULT_VARIABLE COMMAND_RESULT
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT COMMAND_RESULT EQUAL "0")
            message(FATAL_ERROR "Cannot detect default conan profile")
        endif()
    endif()

    # Download conan module
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/${CONAN_CMAKE_VERSION}/conan.cmake"
             "${CMAKE_BINARY_DIR}/conan/conan.cmake" TLS_VERIFY ON)
    endif()
    include(${CMAKE_BINARY_DIR}/conan/conan.cmake)

    # Work around missing MSVC 19.4x detection in cmake-conan
    function(_get_msvc_ide_version result)
        set(${result} "" PARENT_SCOPE)
        if(NOT MSVC_VERSION VERSION_LESS 1400 AND MSVC_VERSION VERSION_LESS 1500)
            set(${result} 8 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1500 AND MSVC_VERSION VERSION_LESS 1600)
            set(${result} 9 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1600 AND MSVC_VERSION VERSION_LESS 1700)
            set(${result} 10 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1700 AND MSVC_VERSION VERSION_LESS 1800)
            set(${result} 11 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1800 AND MSVC_VERSION VERSION_LESS 1900)
            set(${result} 12 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1900 AND MSVC_VERSION VERSION_LESS 1910)
            set(${result} 14 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1910 AND MSVC_VERSION VERSION_LESS 1920)
            set(${result} 15 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1920 AND MSVC_VERSION VERSION_LESS 1930)
            set(${result} 16 PARENT_SCOPE)
        elseif(NOT MSVC_VERSION VERSION_LESS 1930)
            set(${result} 17 PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Conan: Unknown MSVC compiler version [${MSVC_VERSION}]")
        endif()
    endfunction()

    # Set config values
    if(CONAN_BUILD_MISSING)
        set(build_option missing)
    else()
        set(build_option never)
    endif()
    set(generators_option CMakeDeps CMakeToolchain VirtualBuildEnv VirtualRunEnv)

    # Create conanfile.txt
    conan_cmake_configure(REQUIRES ${CONFIG_REQUIRES} GENERATORS ${generators_option} OPTIONS ${CONFIG_OPTIONS})

    # Load settings from environment
    conan_cmake_autodetect(settings)

    # Remove cppstd option
    string(REPLACE ";compiler.cppstd=20" "" settings "${settings}")

    # Adjust settings for Conan 2.x MSVC
    if(MSVC)
        string(REPLACE "compiler=Visual Studio" "compiler=msvc" settings "${settings}")
        string(SUBSTRING "${MSVC_VERSION}" 0 3 msvc_major)
        string(REGEX REPLACE "compiler.version=[^;]+" "compiler.version=${msvc_major}" settings "${settings}")
        string(REGEX MATCH "compiler.runtime=([A-Za-z]+)" _runtime_match "${settings}")
        if(_runtime_match)
            set(_runtime "${CMAKE_MATCH_1}")
            if(_runtime MATCHES "^MD")
                set(_runtime_val "dynamic")
            else()
                set(_runtime_val "static")
            endif()
            if(_runtime MATCHES "d$")
                set(_runtime_type "Debug")
            else()
                set(_runtime_type "Release")
            endif()
            string(REGEX REPLACE "compiler.runtime=[A-Za-z]+" "compiler.runtime=${_runtime_val}" settings "${settings}")
            string(APPEND settings ";compiler.runtime_type=${_runtime_type}")
        endif()
    endif()

    # Install conan packages
    conan_cmake_install(
        PATH_OR_REFERENCE
        .
        BUILD
        ${build_option}
        REMOTE
        conancenter
        SETTINGS
        ${settings})

    # Load the generated toolchain so that package paths are available
    if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
        include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    endif()

    # Include generated dependencies
    if(EXISTS ${CMAKE_BINARY_DIR}/conan_deps.cmake)
        include(${CMAKE_BINARY_DIR}/conan_deps.cmake)
    endif()

    message(STATUS "Conan's libraries setting ${settings}")

    # Load packages
    foreach(package IN LISTS CONFIG_FIND_PACKAGES)
        find_package(${package} CONFIG REQUIRED)
        message(STATUS "\tpackage ${package} loaded")
    endforeach()

    # Load to path
    foreach(to_export IN LISTS CONFIG_EXPORT_VARIABLES)
        set(GLOBAL_${to_export}
            ${${to_export}}
            PARENT_SCOPE)
        message(STATUS "exporting ${to_export} to GLOBAL_${to_export}")
    endforeach()
endfunction()
