#
# Unit test module
#
# Authors: Manel Jimeno <manel.jimeno@gmail.com>
#
# License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
#

include(GoogleTest)
enable_testing()

# Create a unit test target
function(add_cpp_test)
    set(oneValueArgs TARGET)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(CONFIG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    config_target(
        TARGET
        ${CONFIG_TARGET}
        SOURCES
        ${CONFIG_TARGET}.cpp
        LIBRARIES
        ${CONFIG_LIBRARIES}
        GTest::gtest
        GTest::gtest_main
        ${PROJECT_TEST_DEPENDENCIES}
        GTEST
        CPP)
    set_target_properties(${test_name} PROPERTIES FOLDER "test")
    if(WIN32)
        set_tests_properties(
            ${test_name}
            PROPERTIES
                ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin;${CMAKE_BINARY_DIR}/lib;$ENV{PATH}"
        )
    endif()
    gtest_discover_tests(${CONFIG_TARGET})
endfunction()
