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
 * \file dtype.h
 * \brief This header deals with discrete data types in Decision.
 */

#ifndef DTYPE_H
#define DTYPE_H

#include <stdbool.h>
#include "dcfg.h"
#include "dlex.h"

/*
=== HEADER DEFINITIONS ====================================
*/

/**
 * \enum _dType
 * \brief An enum for the data types.
 *
 * The values go up in powers of 2. This is so that we can combine data types
 * to make *vague* data types, e.g. `Multiply` should be able to take
 * `Integer`s *or* `Float`s.
 *
 * \typedef enum _dType DType
 */
typedef enum _dType {
    TYPE_NONE      = 0,
    TYPE_EXECUTION = 1,
    TYPE_INT       = 2,
    TYPE_FLOAT     = 4,
    TYPE_STRING    = 8,
    TYPE_BOOL      = 16,
    TYPE_NAME      = 32,
} DType;

/**
 * \def TYPE_VAR_MIN
 * \brief The least-valued data type that is a variable type.
 */
#define TYPE_VAR_MIN TYPE_INT

/**
 * \def TYPE_VAR_MAX
 * \brief The highest-valued data type that is a variable type.
 */
#define TYPE_VAR_MAX TYPE_BOOL

/**
 * \def TYPE_NUMBER
 * \brief A vague type representing all numbers.
 */
#define TYPE_NUMBER (TYPE_INT | TYPE_FLOAT)

/**
 * \def TYPE_VAR_ANY
 * \brief A vague type representing all variable types.
 */
#define TYPE_VAR_ANY (TYPE_INT | TYPE_FLOAT | TYPE_STRING | TYPE_BOOL)

/**
 * \def TYPE_FROM_LEX(x)
 * \brief A macro to convert from lexical token types (`LexType`) to `DType`.
 */
#define TYPE_FROM_LEX(x) (1 << ((x)-LEX_DATATYPE_START))

/**
 * \def TYPE_FROM_LEX_LITERAL(x)
 * \brief A macro to convert from lexical token literals to `DType`.
 */
#define TYPE_FROM_LEX_LITERAL(x) (2 << ((x)-LEX_LITERAL_START))

/*
=== FUNCTIONS =============================================
*/

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
DECISION_API bool d_type_is_vague(DType vague);

/**
 * \fn const char *d_type_name(DType type)
 * \brief Get a string representation for the name of a data type.
 *
 * \return The string representation of the data type.
 *
 * \param type The type to get the name of.
 */
DECISION_API const char *d_type_name(DType type);

#endif // DTYPE_H
