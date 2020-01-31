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
#include <dcfunc.h>
#include <decision.h>
#include <dsheet.h>
#include <dtype.h>
#include <dvm.h>

#include "assert.h"

#include <stdio.h>

void myHalf(DVM *vm) {
    dfloat input  = d_vm_get_float(vm, 1);
    dfloat output = input / 2;
    d_vm_push_float(vm, output);
}

char *reasonStr;

void myCanDrive(DVM *vm) {
    dint age        = d_vm_get(vm, 1);
    dint hasLicense = d_vm_get(vm, 2);

    dint canDrive = 0;

    if (age >= 16) {
        if (hasLicense) {
            canDrive  = 1;
            reasonStr = "You can drive!";
        } else {
            reasonStr = "You do not have a license.";
        }
    } else {
        reasonStr = "You are not old enough to drive.";
    }

    d_vm_push_ptr(vm, reasonStr);
    d_vm_push(vm, canDrive);
}

dint recursive_factorial(dint n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * recursive_factorial(n - 1);
    }
}

void myFactorial(DVM *vm) {
    dint input  = d_vm_get(vm, 1);
    dint output = recursive_factorial(input);
    d_vm_push(vm, output);
}

int main() {
    DType halfInputs[]  = {TYPE_FLOAT, TYPE_NONE};
    DType halfOutputs[] = {TYPE_FLOAT, TYPE_NONE};

    // d_create_c_function
    d_create_c_function("Half", &myHalf, halfInputs, halfOutputs);

    DType canDriveInputs[]  = {TYPE_INT, TYPE_BOOL, TYPE_NONE};
    DType canDriveOutputs[] = {TYPE_BOOL, TYPE_STRING, TYPE_NONE};

    // d_create_c_function
    d_create_c_function("CanDrive", &myCanDrive, canDriveInputs,
                        canDriveOutputs);

    DType factorialInputs[]  = {TYPE_INT, TYPE_NONE};
    DType factorialOutputs[] = {TYPE_INT, TYPE_NONE};

    // d_create_c_subroutine
    d_create_c_subroutine("Factorial", &myFactorial, factorialInputs,
                          factorialOutputs);

    char *src = "Start~#1\n"
                "Half(100.125)~#2\n"
                "Print(#1, #2)~#3\n"
                "CanDrive(19, false)~#4, #5\n"
                "Print(#3, #4)~#6\n"
                "Print(#6, #5)~#7\n"
                "Factorial(#7, 10)~#8, #9\n"
                "Print(#8, #9)\n";

    char *answer = "50.0625\nfalse\nYou do not have a license.\n3628800\n";
    
    // d_run_string
    START_CAPTURE_STDOUT()
    d_run_string(src, NULL);
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT(answer)

    FILE *file;

#ifdef DECISION_SAFE_FUNCTIONS
    fopen_s(&file, "main.dc", "w");
#else
    file = fopen("main.dc", "w");
#endif // DECISION_SAFE_FUNCTIONS

    fprintf(file, "%s", src);
    fclose(file);

    // d_run_file
    START_CAPTURE_STDOUT()
    d_run_file("main.dc");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT(answer)

    // d_compile_string
    d_compile_string(src, "main.dco");

    // d_run_file
    START_CAPTURE_STDOUT()
    d_run_file("main.dco");
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT(answer)
    
    d_free_c_functions();

    return 0;
}