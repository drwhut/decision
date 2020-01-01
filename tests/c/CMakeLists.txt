#[[
    Decision
    Copyright (C) 2019  Benjamin Beddows

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
]]

# Only create the tests if the user wants to.
option(COMPILER_C_TESTS "Do you want to enable C API tests?" OFF)

if(COMPILER_C_TESTS)

    # A function to link the test executables to either the static or dynamic
    # library, depending on which ones were created.
    # We should use the dynamic library where possible, as there will be a lot
    # of executables made here.
    function(link_with_decision testTarget)
        if(MSVC)
            if(COMPILER_DLL)
                target_link_libraries(${testTarget} decisionLibShared)
            else(COMPILER_DLL)
                target_link_libraries(${testTarget} decisionLibStatic)
            endif(COMPILER_DLL)
        else(MSVC)
            target_link_libraries(${testTarget} decisionLibShared)
        endif(MSVC)
    endfunction(link_with_decision)

    # Add the include directory for the header files.
    include_directories(${PROJECT_SOURCE_DIR}/src)

    # Defining the test executables and linking them to the desired Decision
    # library.
    add_executable(TestDecisionStrings decision_strings/decision_strings.c)
    link_with_decision(TestDecisionStrings)

    # Defining the CMake tests.
    add_test(NAME TestDecisionStrings COMMAND TestDecisionStrings)

endif(COMPILER_C_TESTS)
