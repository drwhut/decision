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

#include <dcfg.h>
#include <decision.h>
#include <dsheet.h>
#include <dtype.h>
#include <dvm.h>

#include "assert.h"

#include <stdio.h>

int test_sheet(Sheet *sheet) {
    DVM vm = d_vm_create();

    char *from = "drwhut";

    // d_vm_push_ptr
    d_vm_push_ptr(&vm, from);

    // d_run_function
    START_CAPTURE_STDOUT()
    d_run_function(&vm, sheet, "SayHi");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT("Hi! From,\ndrwhut\n")

    // d_vm_reset
    d_vm_reset(&vm);

    // d_vm_push
    d_vm_push(&vm, 1377);
    d_vm_push(&vm, 51);

    // d_run_function
    d_run_function(&vm, sheet, "FactorOf");

    // d_vm_pop
    dint answer = d_vm_pop(&vm);
    ASSERT_EQUAL(answer, 1)

    // d_vm_reset
    d_vm_reset(&vm);

    // d_vm_push_float
    d_vm_push_float(&vm, 4.75);

    // d_run_function
    d_run_function(&vm, sheet, "Double");

    // d_vm_pop_float
    dfloat fAnswer = d_vm_pop_float(&vm);
    ASSERT_EQUAL(fAnswer, 9.5)

    d_vm_free(&vm);

    return 0;
}

int main() {

    const char *src = "[Function(FactorOf)]\n"
                      "[FunctionInput(FactorOf, dividend, Integer, 1)]\n"
                      "[FunctionInput(FactorOf, divisor, Integer, 1)]\n"
                      "[FunctionOutput(FactorOf, isDivisor, Boolean)]\n"
                      "Define(FactorOf)~#1, #2\n"
                      "Mod(#1, #2)~#3\n"
                      "Equal(#3, 0)~#4\n"
                      "Return(FactorOf, #4)\n"
                      "[Subroutine(SayHi)]\n"
                      "[FunctionInput(SayHi, from, String, 'Bob')]\n"
                      "Define(SayHi)~#10, #11\n"
                      "Print(#10, 'Hi! From,')~#12\n"
                      "Print(#12, #11)\n"
                      "[Function(Double)]\n"
                      "[FunctionInput(Double, num, Float, 1.0)]\n"
                      "[FunctionOutput(Double, doubled, Float)]\n"
                      "Define(Double)~#20\n"
                      "Multiply(#20, 2)~#21\n"
                      "Return(Double, #21)\n";

    // d_load_string
    Sheet *sheet = d_load_string(src, NULL, NULL);

    int result = test_sheet(sheet);
    ASSERT_EQUAL(result, 0)

    // d_sheet_free
    d_sheet_free(sheet);

    FILE *file = fopen("main.dc", "w");
    fprintf(file, "%s", src);
    fclose(file);

    // d_load_file
    sheet = d_load_file("main.dc", NULL);

    result = test_sheet(sheet);
    ASSERT_EQUAL(result, 0)

    // d_sheet_free
    d_sheet_free(sheet);

    // d_compile_string
    d_compile_string(src, "main.dco", NULL);

    // d_load_file
    sheet = d_load_file("main.dco", NULL);

    result = test_sheet(sheet);
    ASSERT_EQUAL(result, 0)

    // d_sheet_free
    d_sheet_free(sheet);

    return 0;
}