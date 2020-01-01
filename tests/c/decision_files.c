/*
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
*/

#include <dcfg.h>
#include <decision.h>
#include <dsheet.h>

#include "assert.h"

#include <stdio.h>

void write_source_file() {
    FILE *file;
#ifdef DECISION_SAFE_FUNCTIONS
    fopen_s(&file, "main.dc", "w");
#else
    file = fopen("main.dc", "w");
#endif // DECISION_SAFE_FUNCTIONS

    fprintf(file, "Start~#1\nPrint(#1, 'Hello, world!')");
    fclose(file);
}

int main() {
    write_source_file();

    // d_load_source_file
    Sheet *sheet = d_load_source_file("main.dc");

    // d_run_sheet
    START_CAPTURE_STDOUT()
    d_run_sheet(sheet);
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    // d_sheet_free
    d_sheet_free(sheet);

    // d_run_source_file
    START_CAPTURE_STDOUT()
    d_run_source_file("main.dc");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    // d_compile_file
    d_compile_file("main.dc", "main.dco");

    // d_run_object_file
    START_CAPTURE_STDOUT()
    d_run_object_file("main.dco");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    // d_is_object_file
    short isObj = d_is_object_file("main.dc");
    ASSERT_EQUAL(isObj, 0)
    isObj = d_is_object_file("main.dco");
    ASSERT_EQUAL(isObj, 1)

    // d_load_file
    sheet = d_load_file("main.dc");

    // d_run_sheet
    START_CAPTURE_STDOUT()
    d_run_sheet(sheet);
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    // d_sheet_free
    d_sheet_free(sheet);

    // d_run_file
    START_CAPTURE_STDOUT()
    d_run_file("main.dco");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    return 0;
}