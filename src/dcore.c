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

#include "dcore.h"

#include <string.h>

/* A static array of the names of the core functions. */
static const char *CORE_FUNC_NAMES[NUM_CORE_FUNCTIONS] = {
    "Add",
    "And",
    "Div",
    "Divide",
    "Equal",
    "For",
    "IfThen",
    "IfThenElse",
    "Length",
    "LessThan",
    "LessThanOrEqual",
    "Mod",
    "MoreThan",
    "MoreThanOrEqual",
    "Multiply",
    "Not",
    "NotEqual",
    "Or",
    "Print",
    "Set",
    "Subtract",
    "Ternary",
    "While",
    "Xor",
};

/*
    A static array for the number of input sockets for each
    of the core functions.
*/
static const short CORE_FUNC_NUM_INPUTS[NUM_CORE_FUNCTIONS] = {
    -1, // CORE_ADD
    2,  // CORE_AND
    2,  // CORE_DIV
    2,  // CORE_DIVIDE
    2,  // CORE_EQUAL
    4,  // CORE_FOR
    2,  // CORE_IF_THEN
    2,  // CORE_IF_THEN_ELSE
    1,  // CORE_LENGTH
    2,  // CORE_LESS_THAN
    2,  // CORE_LESS_THAN_OR_EQUAL
    2,  // CORE_MOD
    2,  // CORE_MORE_THAN
    2,  // CORE_MORE_THAN_OR_EQUAL
    -1, // CORE_MULTIPLY
    1,  // CORE_NOT
    2,  // CORE_NOT_EQUAL
    2,  // CORE_OR
    2,  // CORE_PRINT
    3,  // CORE_SET
    2,  // CORE_SUBTRACT
    3,  // CORE_TERNARY
    2,  // CORE_WHILE
    2,  // CORE_XOR
};

/*
    A static array for the number of output sockets for each
    of the core functions.
*/
static const short CORE_FUNC_NUM_OUTPUTS[NUM_CORE_FUNCTIONS] = {
    1, // CORE_ADD
    1, // CORE_AND
    1, // CORE_DIV
    1, // CORE_DIVIDE
    1, // CORE_EQUAL
    3, // CORE_FOR
    2, // CORE_IF_THEN
    3, // CORE_IF_THEN_ELSE
    1, // CORE_LENGTH
    1, // CORE_LESS_THAN
    1, // CORE_LESS_THAN_OR_EQUAL
    1, // CORE_MOD
    1, // CORE_MORE_THAN
    1, // CORE_MORE_THAN_OR_EQUAL
    1, // CORE_MULTIPLY
    1, // CORE_NOT
    1, // CORE_NOT_EQUAL
    1, // CORE_OR
    1, // CORE_PRINT
    1, // CORE_SET
    1, // CORE_SUBTRACT
    1, // CORE_TERNARY
    2, // CORE_WHILE
    1, // CORE_XOR
};

/* A static array of the core function's input data types. */
static const DType CORE_FUNC_INPUT_TYPES[NUM_CORE_FUNCTIONS][5] = {
    {TYPE_NUMBER, 0},                                           // CORE_ADD
    {TYPE_INT | TYPE_BOOL, TYPE_INT | TYPE_BOOL, 0},            // CORE_AND
    {TYPE_NUMBER, TYPE_NUMBER, 0},                              // CORE_DIV
    {TYPE_NUMBER, TYPE_NUMBER, 0},                              // CORE_DIVIDE
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING, 0},  // CORE_EQUAL
    {TYPE_EXECUTION, TYPE_NUMBER, TYPE_NUMBER, TYPE_NUMBER, 0}, // CORE_FOR
    {TYPE_EXECUTION, TYPE_BOOL, 0},                             // CORE_IF_THEN
    {TYPE_EXECUTION, TYPE_BOOL, 0}, // CORE_IF_THEN_ELSE
    {TYPE_STRING, 0},               // CORE_LENGTH
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING, 0}, // CORE_LESS_THAN
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING,
     0},                     // CORE_LESS_THAN_OR_EQUAL
    {TYPE_INT, TYPE_INT, 0}, // CORE_MOD
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING, 0}, // CORE_MORE_THAN
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING,
     0},                       // CORE_MORE_THAN_OR_EQUAL
    {TYPE_NUMBER, 0},          // CORE_MULTIPLY
    {TYPE_INT | TYPE_BOOL, 0}, // CORE_NOT
    {TYPE_NUMBER | TYPE_STRING, TYPE_NUMBER | TYPE_STRING, 0}, // CORE_NOT_EQUAL
    {TYPE_INT | TYPE_BOOL, TYPE_INT | TYPE_BOOL, 0},           // CORE_OR
    {TYPE_EXECUTION, TYPE_VAR_ANY, 0},                         // CORE_PRINT
    {TYPE_NAME, TYPE_EXECUTION, TYPE_VAR_ANY, 0},              // CORE_SET
    {TYPE_NUMBER, TYPE_NUMBER, 0},                             // CORE_SUBTRACT
    {TYPE_BOOL, TYPE_VAR_ANY, TYPE_VAR_ANY, 0},                // CORE_TERNARY
    {TYPE_EXECUTION, TYPE_BOOL, 0},                            // CORE_WHILE
    {TYPE_INT | TYPE_BOOL, TYPE_INT | TYPE_BOOL, 0},           // CORE_XOR
};

/* A static array of the core function's output data types. */
static const DType CORE_FUNC_OUTPUT_TYPES[NUM_CORE_FUNCTIONS][4] = {
    {TYPE_NUMBER, 0},                                    // CORE_ADD
    {TYPE_INT | TYPE_BOOL, 0},                           // CORE_AND
    {TYPE_INT, 0},                                       // CORE_DIV
    {TYPE_FLOAT, 0},                                     // CORE_DIVIDE
    {TYPE_BOOL, 0},                                      // CORE_EQUAL
    {TYPE_EXECUTION, TYPE_NUMBER, TYPE_EXECUTION, 0},    // CORE_FOR
    {TYPE_EXECUTION, TYPE_EXECUTION, 0},                 // CORE_IF_THEN
    {TYPE_EXECUTION, TYPE_EXECUTION, TYPE_EXECUTION, 0}, // CORE_IF_THEN_ELSE
    {TYPE_INT, 0},                                       // CORE_LENGTH
    {TYPE_BOOL, 0},                                      // CORE_LESS_THAN
    {TYPE_BOOL, 0},                      // CORE_LESS_THAN_OR_EQUAL
    {TYPE_INT, 0},                       // CORE_MOD
    {TYPE_BOOL, 0},                      // CORE_MORE_THAN
    {TYPE_BOOL, 0},                      // CORE_MORE_THAN_OR_EQUAL
    {TYPE_NUMBER, 0},                    // CORE_MULTIPLY
    {TYPE_INT | TYPE_BOOL, 0},           // CORE_NOT
    {TYPE_BOOL, 0},                      // CORE_NOT_EQUAL
    {TYPE_INT | TYPE_BOOL, 0},           // CORE_OR
    {TYPE_EXECUTION, 0},                 // CORE_PRINT
    {TYPE_EXECUTION, 0},                 // CORE_SET
    {TYPE_NUMBER, 0},                    // CORE_SUBTRACT
    {TYPE_VAR_ANY, 0},                   // CORE_TERNARY
    {TYPE_EXECUTION, TYPE_EXECUTION, 0}, // CORE_WHILE
    {TYPE_INT | TYPE_BOOL, 0},           // CORE_XOR
};

/**
 * \fn const char *d_core_get_name(CoreFunction func)
 * \brief Get the name of a core function as a string.
 *
 * \return The name of the core function.
 *
 * \param func The core function to get the name of.
 */
const char *d_core_get_name(CoreFunction func) {
    return CORE_FUNC_NAMES[func];
}

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
const CoreFunction d_core_find_name(const char *name) {
    // Since the core functions are in alphabetical order,
    // we can use binary search!

    CoreFunction left  = 0;
    CoreFunction right = NUM_CORE_FUNCTIONS - 1;
    CoreFunction middle;

    while (left <= right) {
        middle = (left + right) / 2;

        int cmp = strcmp(name, CORE_FUNC_NAMES[middle]);

        if (cmp < 0) {
            right = middle - 1;
        } else if (cmp > 0) {
            left = middle + 1;
        } else
            return middle;
    }

    return -1;
}

/**
 * \fn const short d_core_num_input_sockets(CoreFunction func)
 * \brief Get the number of input sockets the core function has.
 *
 * \return The number of input sockets the function is defined to have.
 * If -1, then any number is allowed.
 *
 * \param func The function to query.
 */
const short d_core_num_input_sockets(CoreFunction func) {
    return CORE_FUNC_NUM_INPUTS[func];
}

/**
 * \fn const short d_core_num_output_sockets(CoreFunction func)
 * \brief Get the number of output sockets the core function has.
 *
 * \return The number of output sockets the function is defined to have.
 * If -1, then any number is allowed.
 *
 * \param func The function to query.
 */
const short d_core_num_output_sockets(CoreFunction func) {
    return CORE_FUNC_NUM_OUTPUTS[func];
}

/**
 * \fn const DType *d_core_get_input_types(CoreFunction func)
 * \brief Get the core functions input data types.
 *
 * \return The core functions input data types. It is a 0-terminating array.
 *
 * \param func The function to query.
 */
const DType *d_core_get_input_types(CoreFunction func) {
    return CORE_FUNC_INPUT_TYPES[func];
}

/**
 * \fn const DType *d_core_get_output_types(CoreFunction func)
 * \brief Get the core functions output data types.
 *
 * \return The core functions output data types. It is a 0-terminating array.
 *
 * \param func The function to query.
 */
const DType *d_core_get_output_types(CoreFunction func) {
    return CORE_FUNC_OUTPUT_TYPES[func];
}
