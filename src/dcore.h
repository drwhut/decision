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

/**
 * \file dcore.h
 * \brief This header contains the core Decision functions.
 */

#ifndef DCORE_H
#define DCORE_H

#include "dcfg.h"

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the NodeDefinition struct from dsheet.h */
struct _nodeDefinition;

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
 * \fn const NodeDefinition *d_core_get_definition(const CoreFunction core)
 * \brief Get the definition of a core function.
 *
 * \return The definition of the core function.
 *
 * \param core The core function to get the definition of.
 */
DECISION_API const struct _nodeDefinition *
d_core_get_definition(const CoreFunction core);

/**
 * \fn const CoreFunction d_core_find_name(const char *name)
 * \brief Given the name of the core function, get the CoreFunction.
 *
 * \return The corresponding CoreFunction. Value is -1 if the name doesn't
 * exist as a core function.
 *
 * \param name The name to query.
 */
DECISION_API const CoreFunction d_core_find_name(const char *name);

/**
 * \fn void d_core_dump_json()
 * \brief Dump the core functions and subroutines to `stdout` in JSON format.
 */
DECISION_API void d_core_dump_json();

#endif // DCORE_H
