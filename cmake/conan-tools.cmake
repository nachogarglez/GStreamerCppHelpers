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
    find_package(Python3 3.6 QUIET REQUIRED)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade conan
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE COMMAND_RESULT
        OUTPUT_QUIET ERROR_QUIET)
    if(NOT COMMAND_RESULT EQUAL "0")
        message(FATAL_ERROR "Cannot install conan")
    endif()

    # Donwload conan module
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
             "${CMAKE_BINARY_DIR}/conan/conan.cmake" TLS_VERIFY ON)
    endif()
    include(${CMAKE_BINARY_DIR}/conan/conan.cmake)

    # Set config values
    if(CONAN_BUILD_MISSING)
        set(build_option missing)
    else()
        set(build_option never)
    endif()
    set(generators_option cmake cmake_find_package cmake_paths virtualenv virtualbuildenv virtualrunenv)

    # Create conanfile.txt
    conan_cmake_configure(REQUIRES ${CONFIG_REQUIRES} GENERATORS ${generators_option} OPTIONS ${CONFIG_OPTIONS})

    # Load settings from environment
    conan_cmake_autodetect(settings)

    # Remove cppstd option
    string(REPLACE ";compiler.cppstd=20" "" settings "${settings}")

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

    # Include install packages
    if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
        include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
    else()
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    endif()

    # Config project
    conan_basic_setup(NO_OUTPUT_DIRS TARGETS)

    # Include cmake module paths
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

    message(STATUS "Conan's libraries setting ${settings}")

    # Load packages
    foreach(package IN LISTS CONFIG_FIND_PACKAGES)
        find_package(${package} REQUIRED)
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
