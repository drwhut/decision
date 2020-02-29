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

cmake_minimum_required(VERSION 2.6)
project(Decision)

set (Decision_VERSION_MAJOR 0)
set (Decision_VERSION_MINOR 3)
set (Decision_VERSION_PATCH 0)

# Since the built files are added in the src/ directory,
# the files by default would go into a src/ directory.
# This statement stops that.
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)

# Only create the tests if the user wants to.
option(COMPILER_C_TESTS "Do you want to enable C API tests?" OFF)

if(COMPILER_C_TESTS)
    enable_testing()
    add_subdirectory(tests/c)
endif(COMPILER_C_TESTS)