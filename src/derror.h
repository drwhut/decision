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
 * \file derror.h
 * \brief This header deals with errors in Decision during runtime or compile
 * time.
 */

#ifndef DERROR_H
#define DERROR_H

#include <stdbool.h>
#include "dcfg.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/**
 * \def MAX_ERROR_SIZE
 * \brief A macro constant stating the maximum size of formatted error messages.
 */
#define MAX_ERROR_SIZE 256

/**
 * \def ERROR_COMPILER(filePath, lineNum, isError, ...)
 * \brief A macro function to be able to print formatted error messages.
 *
 * If you don't need the error to be formatted, it is more efficient to
 * directly use `d_error_compiler_push`.
 */
#ifdef DECISION_SAFE_FUNCTIONS
#define ERROR_COMPILER(filePath, lineNum, isError, ...)                  \
    {                                                                    \
        char errMsg[MAX_ERROR_SIZE];                                     \
        sprintf_s(errMsg, MAX_ERROR_SIZE, __VA_ARGS__);                  \
        d_error_compiler_push(errMsg, (filePath), (lineNum), (isError)); \
    }
#else
#define ERROR_COMPILER(filePath, lineNum, isError, ...)                  \
    {                                                                    \
        char errMsg[MAX_ERROR_SIZE];                                     \
        sprintf(errMsg, __VA_ARGS__);                                    \
        d_error_compiler_push(errMsg, (filePath), (lineNum), (isError)); \
    }
#endif // DECISION_SAFE_FUNCTIONS

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_error_compiler_push(const char *message, const char *filePath,
 *                                size_t lineNum, bool isError)
 * \brief Push a compile-time error message to the error list.
 *
 * \param message The error message itself.
 * \param filePath The file where the error occured.
 * \param lineNum The line number of the error.
 * \param isError If `true`, show ERROR, otherwise, show WARNING.
 */
DECISION_API void d_error_compiler_push(const char *message,
                                        const char *filePath, size_t lineNum,
                                        bool isError);

/**
 * \fn bool d_error_report()
 * \brief Report if there were any compile-time errors or warnings to `stdout`.
 *
 * \return `true` if there were errors (warnings do not count as errors),
 * `false` otherwise.
 */
DECISION_API bool d_error_report();

/**
 * \fn void d_error_free()
 * \brief Free any compile-time error messages saved.
 *
 * Should be called at the end of compilation.
 */
DECISION_API void d_error_free();

#endif // DERROR_H
