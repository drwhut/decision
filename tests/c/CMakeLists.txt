#[[
    Decision
    Copyright (C) 2019-2020  Benjamin Beddows

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

# A function to link the test executables to either the static or dynamic
# library, depending on which ones were created.
# We should use the dynamic library where possible, as there will be a lot
# of executables made here.
function(link_with_decision testTarget)
    if(MSVC)
        if(COMPILER_SHARED)
            target_link_libraries(${testTarget} decisionLibShared)
        else(COMPILER_SHARED)
            target_link_libraries(${testTarget} decisionLibStatic)
        endif(COMPILER_SHARED)
    else(MSVC)
        target_link_libraries(${testTarget} decisionLibShared)
    endif(MSVC)
endfunction(link_with_decision)

# If the compiler is storing data in 32 bits, we need to here as well.
if(COMPILER_32)
    add_definitions(-DDECISION_32)
endif(COMPILER_32)

# Disable warnings about not using "safe" functions here as well.
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

# Add the include directory for the header files.
include_directories(${PROJECT_SOURCE_DIR}/src)

# Defining the test executables and linking them to the desired Decision
# library.
add_executable(TestCFromDecision c_from_decision.c)
link_with_decision(TestCFromDecision)

add_executable(TestDecisionFiles decision_files.c)
link_with_decision(TestDecisionFiles)

add_executable(TestDecisionFromC decision_from_c.c)
link_with_decision(TestDecisionFromC)

add_executable(TestDecisionObjects decision_objects.c)
link_with_decision(TestDecisionObjects)

add_executable(TestDecisionStrings decision_strings.c)
link_with_decision(TestDecisionStrings)

# Defining the CMake tests.
add_test(NAME TestCFromDecision COMMAND TestCFromDecision)
add_test(NAME TestDecisionFiles COMMAND TestDecisionFiles)
add_test(NAME TestDecisionFromC COMMAND TestDecisionFromC)
add_test(NAME TestDecisionObjects COMMAND TestDecisionObjects)
add_test(NAME TestDecisionStrings COMMAND TestDecisionStrings)
