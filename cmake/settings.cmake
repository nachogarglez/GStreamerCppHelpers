#
# Configure project and target default parameters.
#
# Authors: Manel Jimeno <manel.jimeno@gmail.com>
#
# License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
#

# Check the CMakeLists.txt location
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt")
    message(
        FATAL_ERROR
            "In-source builds are not allowed, please create a 'build' subfolder and use `cmake ..` inside it.\n"
            "NOTE: cmake will now create CMakeCache.txt and CMakeFiles/*.\n"
            "You must delete them, or cmake will refuse to work.")
endif()

# Check if the Generator is multi configuration
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    message(STATUS "Multi-config generator detected")
    set(CMAKE_CONFIGURATION_TYPES
        "Debug;Release"
        CACHE STRING "" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "Setting build type to Debug by default")
        set(CMAKE_BUILD_TYPE
            "Debug"
            CACHE STRING "" FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")
endif()

# Setting build type constraints
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to Debug by default")
    set(CMAKE_BUILD_TYPE
        "Debug"
        CACHE STRING "" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")

# Create the target specified
function(create_target)
    set(oneValueArgs TARGET LIBRARY STATIC)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(CONFIG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(CONFIG_LIBRARY)
        if(CONFIG_STATIC)
            message(STATUS "Creating ${CONFIG_TARGET}, which is a static library")
            add_library(${CONFIG_TARGET} STATIC ${CONFIG_SOURCES})
        else()
            message(STATUS "Creating ${CONFIG_TARGET}, which is a shared library")
            add_library(${CONFIG_TARGET} SHARED ${CONFIG_SOURCES})
        endif()
        set_target_properties(${CONFIG_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    else()
        message(STATUS "Creating ${CONFIG_TARGET}, which is an executable")
        add_executable(${CONFIG_TARGET} ${CONFIG_SOURCES})
        set_target_properties(${CONFIG_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()

    set_target_properties(${CONFIG_TARGET} PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 17 CXX_EXTENSIONS OFF)

endfunction()

# Setup the target
function(config_gtest_properties gtest_param cpp_param)
    if(gtest_param)
        if(MSVC)
            if(MSVC_VERSION AND (MSVC_VERSION EQUAL 1910 OR MSVC_VERSION GREATER 1910))
                add_definitions(-DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0 -DGTEST_LINKED_AS_SHARED_LIBRARY=1)
            endif()
        endif()
    endif()
endfunction()

# Setup the RPATH
function(config_rpath)
    set(oneValueArgs TARGET LIBRARY)
    cmake_parse_arguments(CONFIG "" "${oneValueArgs}" "" ${ARGN})
    message(STATUS "\tSetting rpath for ${CONFIG_TARGET}")

    if(CONFIG_LIBRARY)
        if(APPLE)
            set_target_properties(${CONFIG_TARGET} PROPERTIES MACOSX_RPATH 1)
        else()
            set_target_properties(${CONFIG_TARGET} PROPERTIES CMAKE_BUILD_RPATH "$ORIGIN/lib")
            set_target_properties(${CONFIG_TARGET} PROPERTIES CMAKE_INSTALL_RPATH "$ORIGIN/lib")
        endif()
    else()
        if(APPLE)
            set_target_properties(${CONFIG_TARGET} PROPERTIES BUILD_RPATH "@loader_path/lib")
        else()
            set_target_properties(${CONFIG_TARGET} PROPERTIES CMAKE_BUILD_RPATH "$ORIGIN/lib")
            set_target_properties(${CONFIG_TARGET} PROPERTIES CMAKE_INSTALL_RPATH "$ORIGIN/lib")
        endif()
    endif()
endfunction()

# Setup the target specified
function(config_target)
    set(options LIBRARY STATIC GTEST CPP)
    set(oneValueArgs TARGET)
    set(multiValueArgs
        AUTOGEN_SOURCES
        PUBLIC_HEADERS
        LIBRARIES
        PRIVATE_DEFINITIONS
        SOURCES
        PRIVATE_INCLUDE_DIRECTORIES
        PUBLIC_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(CONFIG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    create_target(
        TARGET
        ${CONFIG_TARGET}
        LIBRARY
        ${CONFIG_LIBRARY}
        STATIC
        ${CONFIG_STATIC}
        SOURCES
        ${CONFIG_SOURCES})

    config_rpath(TARGET ${CONFIG_TARGET} LIBRARY ${CONFIG_LIBRARY})

    if(DEFINED CONFIG_PRIVATE_DEFINITIONS)
        message(STATUS "\tPRIVATE_DEFINITIONS        : ${CONFIG_PRIVATE_DEFINITIONS}")
        target_compile_definitions(${CONFIG_TARGET} PRIVATE ${PRIVATE_DEFINITIONS})
    endif()

    if(DEFINED CONFIG_AUTOGEN_SOURCES)
        message(STATUS "\tAUTOGEN_SOURCES            : ${CONFIG_AUTOGEN_SOURCES}")
        foreach(item IN LISTS AUTOGEN_SOURCES)
            string(REGEX REPLACE "\\.[^.]*$" "" SRC ${item})
            configure_file(${item} ${SRC} @ONLY)
            target_sources(${CONFIG_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${SRC})
        endforeach()
    endif()

    if(DEFINED CONFIG_LIBRARIES)
        message(STATUS "\tLIBRARIES                  : ${CONFIG_LIBRARIES}")
        target_link_libraries(${CONFIG_TARGET} ${CONFIG_LIBRARIES})
    endif()

    if(DEFINED CONFIG_PUBLIC_HEADERS)
        message(STATUS "\tPUBLIC_HEADERS             : ${CONFIG_PUBLIC_HEADERS}")
        target_sources(${CONFIG_TARGET} PUBLIC FILE_SET HEADERS FILES ${CONFIG_PUBLIC_HEADERS})
    endif()

    if(DEFINED CONFIG_PRIVATE_INCLUDE_DIRECTORIES)
        message(STATUS "\tPRIVATE_INCLUDE_DIRECTORIES: ${CONFIG_PRIVATE_INCLUDE_DIRECTORIES}")
        target_include_directories(${CONFIG_TARGET} PRIVATE ${CONFIG_PRIVATE_INCLUDE_DIRECTORIES})
    endif()

    if(DEFINED CONFIG_PUBLIC_INCLUDE_DIRECTORIES)
        message(STATUS "\tPUBLIC_INCLUDE_DIRECTORIES : ${CONFIG_PUBLIC_INCLUDE_DIRECTORIES}")
        target_include_directories(${CONFIG_TARGET} PUBLIC ${CONFIG_PUBLIC_INCLUDE_DIRECTORIES})
    endif()

    config_gtest_properties(${CONFIG_GTEST} ${CONFIG_CPP})

    if(MSVC)
        set(custom_compile_options /W4 /WX)
        if(CONFIG_CPP)
            set(custom_compile_options ${custom_compile_options} /wd4996)
        endif()
    else()
        set(custom_compile_options -Wall -Wextra -Werror)
    endif()

    if(USE_CODE_WARNINGS_AS_ERRORS)
        target_compile_options(${CONFIG_TARGET} PRIVATE ${custom_compile_options})
    endif()

endfunction()

# Default policies
message(STATUS "Setting default policies")
if(APPLE)
    message(STATUS "\tCMP0042 NEW")
    cmake_policy(SET CMP0042 NEW)
    message(STATUS "\tCMP0068 NEW")
    cmake_policy(SET CMP0068 NEW)
endif()

# Output settings
message(STATUS "Output settings")
message(STATUS "\tSetting executables files to save to this path ${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
message(STATUS "\tSetting libraries files to save to this path ${CMAKE_BINARY_DIR}/bin/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/lib")
message(STATUS "\tSetting the other files to save to this path ${CMAKE_BINARY_DIR}/bin/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/lib")
