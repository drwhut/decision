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

#include "dsheet.h"

#include <string.h>

// clang-format off

static const SocketMeta CORE_FUNC_SOCKETS[NUM_CORE_FUNCTIONS][7] = {
    { // CORE_ADD
        {"number", "A number to be used in the addition.", TYPE_NUMBER, 0},
        {"number", "A number to be used in the addition.", TYPE_NUMBER, 0},
        {"output", "The addition of all the inputs.", TYPE_NUMBER, 0}
    },
    { // CORE_AND
        {"input1", "The first integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"input2", "The second integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"output", "The bitwise AND of the two inputs.", TYPE_INT | TYPE_BOOL, 0}
    },
    { // CORE_DIV
        {"dividend", "The dividend of the division.", TYPE_NUMBER, 0},
        {"divisor", "The divisor of the division.", TYPE_NUMBER, 1},
        {"output", "The dividend divided by the divisor, truncated.", TYPE_INT, 0}
    },
    { // CORE_DIVIDE
        {"dividend", "The dividend of the division.", TYPE_NUMBER, 0},
        {"divisor", "The divisor of the division.", TYPE_NUMBER, 1},
        {"output", "The dividend divided by the divisor.", TYPE_FLOAT, 0}
    },
    { // CORE_EQUAL
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the two inputs are equal, false otherwise.", TYPE_BOOL, 0}
    },
    { // CORE_FOR
        {"before", "The for loop will start when this input is activated.", TYPE_EXECUTION, 0},
        {"start", "The starting value of the for loop.", TYPE_NUMBER, 1},
        {"end", "The ending value of the for loop.", TYPE_NUMBER, 10},
        {"step", "The number added to the value of the for loop at the end of every loop.", TYPE_NUMBER, 1},
        {"loop", "This output will be activated at the start of every loop.", TYPE_EXECUTION, 0},
        {"value", "The value of the for loop.", TYPE_NUMBER, 0},
        {"after", "This output will activate when the for loop is over.", TYPE_EXECUTION, 0}
    },
    { // CORE_IF_THEN
        {"before", "The node will check the boolean input when this input is activated.", TYPE_EXECUTION, 0},
        {"if", "The condition boolean.", TYPE_BOOL, 0},
        {"then", "This output is only activated if the condition is true.", TYPE_EXECUTION, 0},
        {"after", "This output will activate after the condition has been checked.", TYPE_EXECUTION, 0}
    },
    { // CORE_IF_THEN_ELSE
        {"before", "The node will check the boolean input when this input is activated.", TYPE_EXECUTION, 0},
        {"if", "The condition boolean.", TYPE_BOOL, 0},
        {"then", "This output is only activated if the condition is true.", TYPE_EXECUTION, 0},
        {"else", "This output is only activated if the condition is false.", TYPE_EXECUTION, 0},
        {"after", "This output will activate after the condition has been checked.", TYPE_EXECUTION, 0}
    },
    { // CORE_LENGTH
        {"string", "The string to get the length of.", TYPE_STRING, NULL},
        {"length", "The length of the input string.", TYPE_INT, 0}
    },
    { // CORE_LESS_THAN
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the first input is less than the second input, false otherwise.", TYPE_BOOL, 0}
    },
    { // CORE_LESS_THAN_OR_EQUAL
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the first input is less than or equal to the second input, false otherwise.", TYPE_BOOL, 0}
    },
    { // CORE_MOD
        {"dividend", "The dividend of the division.", TYPE_INT, 0},
        {"divisor", "The divisor of the division.", TYPE_INT, 1},
        {"output", "The remainder after division of the dividend by the divisor.", TYPE_INT, 0}
    },
    { // CORE_MORE_THAN
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the first input is more than the second input, false otherwise.", TYPE_BOOL, 0}
    },
    { // CORE_MORE_THAN_OR_EQUAL
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the first input is more than or equal to the second input, false otherwise.", TYPE_BOOL, 0}
    },
    { // CORE_MULTIPLY
        {"number", "A number to be used in the multiplication.", TYPE_NUMBER, 0},
        {"number", "A number to be used in the multiplication.", TYPE_NUMBER, 0},
        {"output", "The multiplication of all the inputs.", TYPE_NUMBER, 0}
    },
    { // CORE_NOT
        {"input", "The integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"output", "The bitwise NOT of the input.", TYPE_INT | TYPE_BOOL, 0}
    },
    { // CORE_NOT_EQUAL
        {"input1", "The first input.", TYPE_VAR_ANY, 0},
        {"input2", "The second input.", TYPE_VAR_ANY, 0},
        {"output", "True if the two inputs are not equal, false otherwise.", TYPE_BOOL, 0}   
    },
    { // CORE_OR
        {"input1", "The first integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"input2", "The second integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"output", "The bitwise OR of the two inputs.", TYPE_INT | TYPE_BOOL, 0}   
    },
    { // CORE_PRINT
        {"before", "The node will print the value when this input is activated.", TYPE_EXECUTION, 0},
        {"value", "The value to print to the screen.", TYPE_VAR_ANY, 0},
        {"after", "This output will activate after the value has been printed.", TYPE_EXECUTION, 0}
    },
    { // CORE_SET
        {"variable", "The variable whose value to set.", TYPE_NAME, 0},
        {"before", "The node will set the value of the variable when this input is activated.", TYPE_EXECUTION, 0},
        {"value", "The value to set the variable to. It must be the same data type as the variable.", TYPE_VAR_ANY, 0},
        {"after", "This output is activated after the variable's value has been set.", TYPE_EXECUTION, 0}
    },
    { // CORE_SUBTRACT
        {"from", "The number to subtract from.", TYPE_NUMBER, 0},
        {"subtract", "How much to subtract.", TYPE_NUMBER, 1},
        {"output", "The subtraction of the two inputs.", TYPE_NUMBER, 0}
    },
    { // CORE_TERNARY
        {"if", "The condition boolean.", TYPE_BOOL, 0},
        {"then", "The input to output if the condition is true.", TYPE_VAR_ANY, 0},
        {"else", "The input to output if the condition is false.", TYPE_VAR_ANY, 0},
        {"output", "The selected output.", TYPE_VAR_ANY, 0}
    },
    { // CORE_WHILE
        {"before", "The while loop will start when this input is activated.", TYPE_EXECUTION, 0},
        {"condition", "The condition that needs to be met for the while loop to continue looping.", TYPE_BOOL, 0},
        {"loop", "This output will be activated at the start of every loop.", TYPE_EXECUTION, 0},
        {"after", "This output will activate when the while loop is over.", TYPE_EXECUTION, 0}
    },
    { // CORE_XOR
        {"input1", "The first integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"input2", "The second integer or boolean input.", TYPE_INT | TYPE_BOOL, 0},
        {"output", "The bitwise XOR of the two inputs.", TYPE_INT | TYPE_BOOL, 0}  
    }
};

static const NodeDefinition CORE_FUNC_DEFINITIONS[NUM_CORE_FUNCTIONS] = {
    {"Add", "Calculate the addition of two or more numbers.", CORE_FUNC_SOCKETS[CORE_ADD], 3, 2, true},
    {"And", "Calculate the bitwise AND of two integers or booleans.", CORE_FUNC_SOCKETS[CORE_AND], 3, 2, false},
    {"Div", "Calculate the truncated division of two numbers.", CORE_FUNC_SOCKETS[CORE_DIV], 3, 2, false},
    {"Divide", "Calculate the division of two numbers.", CORE_FUNC_SOCKETS[CORE_DIVIDE], 3, 2, false},
    {"Equal", "Check if two values are equal.", CORE_FUNC_SOCKETS[CORE_EQUAL], 3, 2, false},
    {"For", "For each iteration of a numerical value, activate an execution path.", CORE_FUNC_SOCKETS[CORE_FOR], 7, 4, false},
    {"IfThen", "Activate an execution path if a condition is true.", CORE_FUNC_SOCKETS[CORE_IF_THEN], 4, 2, false},
    {"IfThenElse", "Activate an execution path if a condition is true, or another if the condition is false.", CORE_FUNC_SOCKETS[CORE_IF_THEN_ELSE], 5, 2, false},
    {"Length", "Output the number of characters in a string.", CORE_FUNC_SOCKETS[CORE_LENGTH], 2, 1, false},
    {"LessThan", "Check if one value is less than another.", CORE_FUNC_SOCKETS[CORE_LESS_THAN], 3, 2, false},
    {"LessThanOrEqual", "Check if one value is less than or equal to another.", CORE_FUNC_SOCKETS[CORE_LESS_THAN_OR_EQUAL], 3, 2, false},
    {"Mod", "Calculate the remainder after division of two integers.", CORE_FUNC_SOCKETS[CORE_MOD], 3, 2, false},
    {"Multiply", "Calculate the multiplication of two or more numbers.", CORE_FUNC_SOCKETS[CORE_MULTIPLY], 3, 2, true},
    {"Not", "Calculate the bitwise NOT of an integer or boolean.", CORE_FUNC_SOCKETS[CORE_NOT], 2, 1, false},
    {"NotEqual", "Check if two values are not equal.", CORE_FUNC_SOCKETS[CORE_NOT_EQUAL], 3, 2, false},
    {"Or", "Calculate the bitwise OR of two integers or booleans.", CORE_FUNC_SOCKETS[CORE_OR], 3, 2, false},
    {"Print", "Print a value to the standard output.", CORE_FUNC_SOCKETS[CORE_PRINT], 3, 2, false},
    {"Set", "Set the value of a variable.", CORE_FUNC_SOCKETS[CORE_SET], 4, 3, false},
    {"Ternary", "Output one input or another, depending on a condition.", CORE_FUNC_SOCKETS[CORE_TERNARY], 4, 3, false},
    {"While", "Keep activating an execution path while a condition is true.", CORE_FUNC_SOCKETS[CORE_WHILE], 4, 2, false},
    {"Xor", "Calculate the bitwise XOR of two integers or booleans.", CORE_FUNC_SOCKETS[CORE_XOR], 3, 2, false}
};

// clang-format on

/**
 * \fn const NodeDefinition *d_core_get_definition(const CoreFunction core)
 * \brief Get the definition of a core function.
 *
 * \return The definition of the core function.
 *
 * \param core The core function to get the definition of.
 */
const NodeDefinition *d_core_get_definition(const CoreFunction core) {
    if (core < 0 || core >= NUM_CORE_FUNCTIONS) {
        return NULL;
    }

    return CORE_FUNC_DEFINITIONS + core;
}

/**
 * \fn const CoreFunction d_core_find_name(const char *name)
 * \brief Given the name of the core function, get the CoreFunction.
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

        NodeDefinition *def = d_core_get_definition(middle);

        short cmp = strcmp(name, def->name);

        if (cmp < 0) {
            right = middle - 1;
        } else if (cmp > 0) {
            left = middle + 1;
        } else
            return middle;
    }

    return -1;
}
