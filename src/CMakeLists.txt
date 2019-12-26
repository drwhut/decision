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

set (SRCS
dasm.c
dcodegen.c
dcore.c
decision.c
derror.c
dlex.c
dlink.c
dmalloc.c
doptimize.c
dsemantic.c
dsheet.c
dsyntax.c
dtype.c
dvm.c
)

set (HDRS
dasm.h
dcfg.h
dcodegen.h
dcore.h
decision.h
derror.h
dlex.h
dlink.h
dmalloc.h
doptimize.h
dsemantic.h
dsheet.h
dsyntax.h
dtype.h
dvm.h
)

# Configure the main header file so we only need to change
# things like the version number once.
configure_file(decision.in.h ${PROJECT_SOURCE_DIR}/src/decision.h)

# Do we want to compile in 32-bit mode?
option(COMPILER_32 "Set the compiler to store data in 32-bit formats?" OFF)

# Do we want to compile into a DLL?
option(COMPILER_DLL "Build a DLL rather than an executable?" OFF)

if(COMPILER_32)
    add_definitions(-DDECISION_32)
endif(COMPILER_32)

if(COMPILER_DLL)
    add_definitions(-DDECISION_BUILD_DLL)
endif(COMPILER_DLL)

# The executable.
add_executable(decision ${SRCS})
install(TARGETS decision DESTINATION bin)

# Set where the header files should be installed.
install(FILES ${HDRS} DESTINATION include/decision)

# Add the libraries, but only one if on Windows.
if(MSVC)
    if(COMPILER_DLL)
        add_library(decisionLibShared SHARED ${SRCS})
    else(COMPILER_DLL)
        add_library(decisionLibStatic STATIC ${SRCS})
    endif(COMPILER_DLL)
else(MSVC)
    add_library(decisionLibStatic STATIC ${SRCS})
    add_library(decisionLibShared SHARED ${SRCS})
endif(MSVC)

# Change the output names of the libraries to be the same name as the executable,
# and make the library dependent on the executable, so that it is always built
# AFTER the executable.
# We also want to pass a macro definition to the libraries so the pre-processor
# knows we are building libraries.
# We also want to set the install destination.
if(TARGET decisionLibShared)
    set_target_properties(decisionLibShared PROPERTIES OUTPUT_NAME decision)
    set_target_properties(decisionLibShared PROPERTIES COMPILE_DEFINITIONS DECISION_LIBRARY)
    add_dependencies(decisionLibShared decision)
    install(TARGETS decisionLibShared DESTINATION lib)
endif(TARGET decisionLibShared)
if(TARGET decisionLibStatic)
    set_target_properties(decisionLibStatic PROPERTIES OUTPUT_NAME decision)
    set_target_properties(decisionLibStatic PROPERTIES COMPILE_DEFINITIONS DECISION_LIBRARY)
    add_dependencies(decisionLibStatic decision)
    install(TARGETS decisionLibStatic DESTINATION lib)
endif(TARGET decisionLibStatic)

# Export symbols for debugging on Linux.
# target_compile_options(decision PRIVATE "-g")

# Show warnings!
if(MSVC)
    target_compile_options(decision PRIVATE "-W4")
else(MSVC)
    target_compile_options(decision PRIVATE "-Wall")
endif(MSVC)