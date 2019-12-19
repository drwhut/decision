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

#include "dtype.h"

/**
 * \fn bool d_type_is_vague(DType vague)
 * \brief Given a possible vague data type, return if it is actually vague.
 *
 * **NOTE:** Vague means more than one variable data type, e.g. `Integer |
 * Float`
 *
 * \return If the data type is vague.
 *
 * \param vague The possible vague data type.
 */
bool d_type_is_vague(DType vague) {
    bool found   = false;
    bool isVague = false;

    for (DType test = TYPE_VAR_MIN; test <= TYPE_VAR_MAX; test = test << 1) {
        if ((vague & test) == test) {
            if (found) {
                isVague = true;
                break;
            } else {
                found = true;
            }
        }
    }

    return isVague;
}

/**
 * \fn const char *d_type_name(DType type)
 * \brief Get a string representation for the name of a data type.
 *
 * \return The string representation of the data type.
 *
 * \param type The type to get the name of.
 */
const char *d_type_name(DType type) {
    switch ((int)type) {
        // Non-vague types:
        case TYPE_EXECUTION:
            return "Execution";
        case TYPE_INT:
            return "Integer";
        case TYPE_FLOAT:
            return "Float";
        case TYPE_STRING:
            return "String";
        case TYPE_BOOL:
            return "Boolean";
        case TYPE_NAME:
            return "Name";
        // Vague types:
        case TYPE_NUMBER:
            return "Number";
        case TYPE_VAR_ANY:
            return "Variable";
        default:
            return NULL;
    }
}
