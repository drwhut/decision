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

set (SRCS
dasm.c
dcfunc.c
dcodegen.c
dcore.c
ddebug.c
decision.c
derror.c
dgraph.c
dlex.c
dlink.c
dmalloc.c
dname.c
dobj.c
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
dcfunc.h
dcodegen.h
dcore.h
ddebug.h
decision.h
derror.h
dgraph.h
dlex.h
dlink.h
dmalloc.h
dname.h
dobj.h
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

# Do we want to compile a shared library rather than a static one?
option(COMPILER_SHARED "Build a shared library rather than a static one?" OFF)

if(COMPILER_32)
    add_definitions(-DDECISION_32)
endif(COMPILER_32)

if(COMPILER_SHARED)
    if (MSVC)
        add_definitions(-DDECISION_BUILD_DLL)
    endif(MSVC)
endif(COMPILER_SHARED)

# Disable warnings about not using "safe" functions in MSVC.
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

# The executable.
add_executable(decision dmain.c)
install(TARGETS decision DESTINATION bin)

# Set where the header files should be installed.
install(FILES ${HDRS} DESTINATION include/decision)

# The library.
if(COMPILER_SHARED)
    add_library(decisionLibShared SHARED ${SRCS})

    # Link the executable with the library.
    target_link_libraries(decision PUBLIC decisionLibShared)

    # Set the install target.
    install(TARGETS decisionLibShared DESTINATION lib)

    # Set the name of the library.
    if(MSVC)
        set_target_properties(decisionLibShared PROPERTIES OUTPUT_NAME libdecision)
    else(MSVC)
        set_target_properties(decisionLibShared PROPERTIES OUTPUT_NAME decision)
    endif(MSVC)
else(COMPILER_SHARED)
    add_library(decisionLibStatic STATIC ${SRCS})

    # Link the executable with the library.
    target_link_libraries(decision PUBLIC decisionLibStatic)

    # Set the install target.
    install(TARGETS decisionLibStatic DESTINATION lib)

    # Set the name of the library.
    if(MSVC)
        set_target_properties(decisionLibStatic PROPERTIES OUTPUT_NAME libdecision)
    else(MSVC)
        set_target_properties(decisionLibStatic PROPERTIES OUTPUT_NAME decision)
    endif(MSVC)
endif(COMPILER_SHARED)

# Export symbols for debugging on Linux.
# target_compile_options(decision PRIVATE "-g")

# Show warnings!
if(MSVC)
    target_compile_options(decision PRIVATE "-W4")
else(MSVC)
    target_compile_options(decision PRIVATE "-Wall")
endif(MSVC)
