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

/**
 * \file dcfunc.h
 * \brief This header file deals with C functions that Decision code can call.
 */

#ifndef DCFUNC_H
#define DCFUNC_H

#include "dcfg.h"
#include "dtype.h"

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the DVM struct from dvm.h */
struct _DVM;

/**
 * \typedef void (*DecisionCFunction)(DVM *vm)
 * \brief This function prototype describes what C functions that are called
 * by Decision should look like.
 */
typedef void (*DecisionCFunction)(struct _DVM *vm);

/**
 * \struct _cFunction
 * \brief A structure describing the semantics of a C function that Decision
 * can call.
 *
 * \typedef struct _cFunction CFunction
 */
typedef struct _cFunction {
    const char *name;
    DecisionCFunction function;

    const DType *inputs;
    size_t numInputs;

    const DType *outputs;
    size_t numOutputs;
} CFunction;

/*
=== FUNCTIONS =============================================
*/

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
DECISION_API void d_create_c_function(const char *name,
                                      DecisionCFunction function, DType *inputs,
                                      DType *outputs);

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
DECISION_API void d_create_c_subroutine(const char *name,
                                        DecisionCFunction function,
                                        DType *inputs, DType *outputs);

/**
 * \fn size_t d_get_num_c_functions()
 * \brief Get the current number of C functions.
 *
 * **NOTE:** Use `d_get_c_functions` to get the items in the list.
 *
 * \return The number of C functions added.
 */
DECISION_API size_t d_get_num_c_functions();

/**
 * \fn const CFunction *d_get_c_functions()
 * \brief Get the current list of C functions.
 *
 * **NOTE:** Use `d_get_num_c_functions` to get the number of items in this
 * list.
 *
 * \return The current list of C functions.
 */
DECISION_API const CFunction *d_get_c_functions();

/**
 * \fn void d_free_c_functions()
 * \brief If any functions or subroutines have been created that call C
 * functions, free them.
 */
DECISION_API void d_free_c_functions();

#endif // DCFUNC_H