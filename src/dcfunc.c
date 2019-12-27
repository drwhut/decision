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

#include "dcfunc.h"

#include "dmalloc.h"

#include <stdlib.h>
#include <string.h>

/* A dynamic list of all defined C functions. */
static CFunction *cFunctionList = NULL;
static size_t numCFunctions     = 0;

/**
 * \fn void d_create_c_function(const char *name, DecisionCFunction function,
 *                              DType *inputs, DType *outputs)
 * \brief Create a function that calls a C function. Any sheets that are loaded
 * after this call can use this new function.
 *
 * \param name The name of the function.
 * \param function The C function to call when the node is activated.
 * \param inputs A `TYPE_NONE`-terminated array of the input data types.
 * \param outputs A `TYPE_NONE`-terminated array of the output data types.
 */
void d_create_c_function(const char *name, DecisionCFunction function,
                         DType *inputs, DType *outputs) {
    size_t numInputs = 0, numOutputs = 0; // NOT including the TYPE_NONE!
    DType *inputPtr, *outputPtr;

    for (inputPtr = inputs; *inputPtr != TYPE_NONE; inputPtr++) {
        numInputs++;
    }

    for (outputPtr = outputs; *outputPtr != TYPE_NONE; outputPtr++) {
        numOutputs++;
    }

    // Create a new array of the types, which does not include the terminating
    // TYPE_NONE. These arrays will need to be free'd.
    DType *newInputs  = (DType *)d_malloc((numInputs + 1) * sizeof(DType));
    DType *newOutputs = (DType *)d_malloc((numOutputs + 1) * sizeof(DType));

    memcpy(newInputs, inputs, numInputs * sizeof(DType));
    memcpy(newOutputs, outputs, numOutputs * sizeof(DType));

    // Copy the name of the function. This will need to be free'd also.
    size_t nameSize = strlen(name) + 1;
    char *newName   = (char *)d_malloc(nameSize);
    memcpy(newName, name, nameSize);

    // Build the CFunction struct.
    CFunction func;
    func.name     = (const char *)newName;
    func.function = function;

    func.inputs    = newInputs;
    func.numInputs = numInputs;

    func.outputs    = newOutputs;
    func.numOutputs = numOutputs;

    // Add the struct to the global list.
    if (numCFunctions == 0) {
        cFunctionList    = (CFunction *)d_malloc(sizeof(CFunction));
        cFunctionList[0] = func;
    } else {
        cFunctionList = (CFunction *)d_realloc(
            cFunctionList, (numCFunctions + 1) * sizeof(CFunction));
        cFunctionList[numCFunctions] = func;
    }

    numCFunctions++;
}

/**
 * \fn void d_create_c_subroutine(const char *name, DecisionCFunction function,
 *                                DType *inputs, DType *outputs)
 * \brief Create a subroutine that calls a C function. Any sheets that are
 * loaded after this call can use this new subroutine.
 *
 * \param name The name of the subroutine.
 * \param function The C function to call when the node is activated.
 * \param inputs A `TYPE_NONE`-terminated array of the input data types. Note
 * that this array should not include a `TYPE_EXECUTION` type.
 * \param outputs A `TYPE_NONE`-terminated array of the output data types. Note
 * that this array should not include a `TYPE_EXECUTION` type.
 */
void d_create_c_subroutine(const char *name, DecisionCFunction function,
                           DType *inputs, DType *outputs) {
    // For this function, we're just going to prepend TYPE_EXECUTION to each
    // of the input and output arrays, and pass that onto d_create_c_function.

    size_t numInputs = 0, numOutputs = 1; // Including the TYPE_NONE!
    DType *inputPtr, *outputPtr;

    for (inputPtr = inputs; *inputPtr != TYPE_NONE; inputPtr++) {
        numInputs++;
    }

    for (outputPtr = outputs; *outputPtr != TYPE_NONE; outputPtr++) {
        numOutputs++;
    }

    DType *newInputs  = (DType *)d_malloc((numInputs + 1) * sizeof(DType));
    DType *newOutputs = (DType *)d_malloc((numOutputs + 1) * sizeof(DType));

    newInputs[0]  = TYPE_EXECUTION;
    newOutputs[0] = TYPE_EXECUTION;

    memcpy(newInputs + 1, inputs, numInputs * sizeof(DType));
    memcpy(newOutputs + 1, outputs, numOutputs * sizeof(DType));

    d_create_c_function(name, function, newInputs, newOutputs);

    free(newInputs);
    free(newOutputs);
}

/**
 * \fn size_t d_get_num_c_functions()
 * \brief Get the current number of C functions.
 *
 * **NOTE:** Use `d_get_c_functions` to get the items in the list.
 *
 * \return The number of C functions added.
 */
size_t d_get_num_c_functions() {
    return numCFunctions;
}

/**
 * \fn const CFunction *d_get_c_functions()
 * \brief Get the current list of C functions.
 *
 * **NOTE:** Use `d_get_num_c_functions` to get the number of items in this
 * list.
 *
 * \return The current list of C functions.
 */
const CFunction *d_get_c_functions() {
    return (const CFunction *)cFunctionList;
}

/**
 * \fn void d_free_c_functions()
 * \brief If any functions or subroutines have been created that call C
 * functions, free them.
 */
void d_free_c_functions() {
    for (size_t i = 0; i < numCFunctions; i++) {
        CFunction func = cFunctionList[i];

        if (func.name != NULL) {
            free((char *)func.name);
        }

        if (func.inputs != NULL) {
            free((DType *)func.inputs);
        }

        if (func.outputs != NULL) {
            free((DType *)func.outputs);
        }
    }

    free(cFunctionList);

    cFunctionList = NULL;
    numCFunctions = 0;
}