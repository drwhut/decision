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
 * \file dcore.h
 * \brief This header contains the core Decision functions.
 */

#ifndef DCORE_H
#define DCORE_H

#include "dcfg.h"
#include "dtype.h"

/*
=== HEADER DEFINITIONS ====================================
*/

/*
    An enum of the core functions.
    NOTE: Need to be in alphabetical order for binary search!
*/
/**
 * \enum _coreFunction
 * \brief An enum of the core functions.
 *
 * **NOTE:** They need to be in alphabetical order in order for binary search
 * to work!
 *
 * \typedef enum _coreFunction CoreFunction
 */
typedef enum _coreFunction {
    CORE_ADD,                // = 0
    CORE_AND,                // = 1
    CORE_DIV,                // = 2
    CORE_DIVIDE,             // = 3
    CORE_EQUAL,              // = 4
    CORE_FOR,                // = 5
    CORE_IF_THEN,            // = 6
    CORE_IF_THEN_ELSE,       // = 7
    CORE_LENGTH,             // = 8
    CORE_LESS_THAN,          // = 9
    CORE_LESS_THAN_OR_EQUAL, // = 10
    CORE_MOD,                // = 11
    CORE_MORE_THAN,          // = 12
    CORE_MORE_THAN_OR_EQUAL, // = 13
    CORE_MULTIPLY,           // = 14
    CORE_NOT,                // = 15
    CORE_NOT_EQUAL,          // = 16
    CORE_OR,                 // = 17
    CORE_PRINT,              // = 18
    CORE_SET,                // = 19
    CORE_SUBTRACT,           // = 20
    CORE_TERNARY,            // = 21
    CORE_WHILE,              // = 22
    CORE_XOR,                // = 23
} CoreFunction;

/**
 * \def NUM_CORE_FUNCTIONS
 * \brief Returns the number of core functions.
 */
#define NUM_CORE_FUNCTIONS (CORE_XOR + 1)

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn const char *d_core_get_name(CoreFunction func)
 * \brief Get the name of a core function as a string.
 *
 * \return The name of the core function.
 *
 * \param func The core function to get the name of.
 */
DECISION_API const char *d_core_get_name(CoreFunction func);

/**
 * \fn const CoreFunction d_core_find_name(const char *name)
 * \brief Opposite of `d_core_get_name`, get the CoreFunction from the
 * corresponding name.
 *
 * \return The corresponding CoreFunction. Value is -1 if the name doesn't
 * exist as a core function.
 *
 * \param name The name to query.
 */
DECISION_API const CoreFunction d_core_find_name(const char *name);

/**
 * \fn const short d_core_num_input_sockets(CoreFunction func)
 * \brief Get the number of input sockets the core function has.
 *
 * \return The number of input sockets the function is defined to have.
 * If -1, then any number is allowed.
 *
 * \param func The function to query.
 */
DECISION_API const short d_core_num_input_sockets(CoreFunction func);

/**
 * \fn const short d_core_num_output_sockets(CoreFunction func)
 * \brief Get the number of output sockets the core function has.
 *
 * \return The number of output sockets the function is defined to have.
 * If -1, then any number is allowed.
 *
 * \param func The function to query.
 */
DECISION_API const short d_core_num_output_sockets(CoreFunction func);

/**
 * \fn const DType *d_core_get_input_types(CoreFunction func)
 * \brief Get the core functions input data types.
 *
 * \return The core functions input data types. It is a 0-terminating array.
 *
 * \param func The function to query.
 */
DECISION_API const DType *d_core_get_input_types(CoreFunction func);

/**
 * \fn const DType *d_core_get_output_types(CoreFunction func)
 * \brief Get the core functions output data types.
 *
 * \return The core functions output data types. It is a 0-terminating array.
 *
 * \param func The function to query.
 */
DECISION_API const DType *d_core_get_output_types(CoreFunction func);

#endif // DCORE_H
