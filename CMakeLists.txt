#
# Main cmake file.
#
# Authors: Manel Jimeno <manel.jimeno@gmail.com>
#
# License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
#

cmake_minimum_required(VERSION 3.22.3-3.23)
find_package(Python3 3.6 QUIET REQUIRED)

project(
    GstStreamerCppHelpers
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Include modules
include(settings)
include(clang-tools)
include(pre-commit)
include(conan-tools)

# Config conan
set(CONAN_OPTIONS "*:shared=True" )

list(APPEND CONAN_REQUIRES "gtest/cci.20210126")
list(APPEND CONAN_FIND GTest)
list(APPEND CONAN_OPTIONS "libelf:shared=False")
include(unit-tests)

conan_configure(REQUIRES ${CONAN_REQUIRES} OPTIONS ${CONAN_OPTIONS} FIND_PACKAGES ${CONAN_FIND})

# Add subdirs
add_subdirectory(GstPtr/test)
