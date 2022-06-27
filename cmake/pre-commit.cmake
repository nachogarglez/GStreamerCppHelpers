#
# Install the pre-commit tool and set up the git hook.
#
# Authors: Manel Jimeno <manel.jimeno@gmail.com>
#
# License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
#

# Install pre-commit and formatting tools needed for it to work
function(configure_pre_commit)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git/hook/pre-commit")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade editorconfig-checker black cmakelang pre-commit
            WORKING_DIRECTORY "."
            RESULT_VARIABLE COMMAND_RESULT
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT COMMAND_RESULT EQUAL "0")
            message(FATAL_ERROR "Cannot install editorconfig-checker, black, pre-commit or cmakelang")
        endif()

        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.pre-commit-config.yaml")
            message(FATAL_ERROR "The file .pre-commit-config.yaml is mandatory to config pre-commit tool")
        endif()

        find_program(
            PRE_COMMIT_TOOL
            NAMES pre-commit
            DOC "Pre-commit tool")

        execute_process(COMMAND pre-commit install WORKING_DIRECTORY ".")
        # RESULT_VARIABLE COMMAND_RESULT             OUTPUT_QUIET ERROR_QUIET)

        if(NOT COMMAND_RESULT EQUAL "0")
            message(FATAL_ERROR "Cannot install pre-commit")
        else()
            message(STATUS "Pre-commit tools install successfully")
        endif()

    endif()
endfunction()

if(NOT PRE_COMMIT_TOOL)
    configure_pre_commit()
endif()
