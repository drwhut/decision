/*
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
*/

#include <decision.h>
#include <dsheet.h>

#include "assert.h"

int main() {
    // d_compile_string
    d_compile_string("Start~#1; Print(#1, 'Hello, world!');", "main.dco", NULL);

    // d_load_object_file
    Sheet *sheet = d_load_object_file("main.dco", NULL);

    // d_run_sheet
    START_CAPTURE_STDOUT()
    d_run_sheet(sheet);
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")

    // d_sheet_free
    d_sheet_free(sheet);

    // d_run_object_file
    START_CAPTURE_STDOUT()
    d_run_object_file("main.dco", NULL);
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hello, world!\n")
    
    return 0;
}