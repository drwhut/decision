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
 * \file dcfg.h
 * \brief This header configures the source code, depending on macro
 * definitions and the environment.
 */

#ifndef DCFG_H
#define DCFG_H

#include <stdint.h>

/**
 * \def dint
 * \brief An integer.
 *
 * If `DECISION_32` is defined, `dint` will be equivalent to `int32_t`.
 * Otherwise, it will be equivalent to `int64_t`.
 *
 * \def duint
 * \brief An unsigned integer.
 *
 * If `DECISION_32` is defined, `duint` will be equivalent to `uint32_t`.
 * Otherwise, it will be equivalent to `uint64_t`.
 *
 * \def dfloat
 * \brief A floating point number.
 *
 * If `DECISION_32` is defined, `dfloat` will be equivalent to `float`.
 * Otherwise, it will be equivalent to `double`.
 */
#ifdef DECISION_32
#define dint   int32_t
#define duint  uint32_t
#define dfloat float
#else
#define dint   int64_t
#define duint  uint64_t
#define dfloat double
#endif // DECISION_32

/*
    Helper definitions for when we want to print the integer types in printf().
    These don't include %f, since it prints as a double anyway.
*/
/**
 * \def DINT_PRINTF_d
 * \brief Helper definition for when we want to print a signed integer type
 * in `printf`.
 *
 * \def DINT_PRINTF_u
 * \brief Helper definition for when we want to print an unsigned integer type
 * in `printf`.
 *
 * \def DINT_PRINTF_x
 * \brief Helper definition for when we want to print an hexadecimal integer
 * in `printf`.
 */
#if defined(DECISION_32) && !defined(WIN32)
#define DINT_PRINTF_d "d"
#define DINT_PRINTF_u "u"
#define DINT_PRINTF_x "x"
#elif (!defined(DECISION_32) && defined(WIN32))
#define DINT_PRINTF_d "lld"
#define DINT_PRINTF_u "llu"
#define DINT_PRINTF_x "llx"
#else // (defined(DECISION_32) && defined(WIN32)) || (!defined(DECISION_32) &&
      // !defined(WIN32))
#define DINT_PRINTF_d "ld"
#define DINT_PRINTF_u "lu"
#define DINT_PRINTF_x "lx"
#endif

/**
 * \def DECISION_API
 * \brief Goes in front of all functions and variables that we want programs
 * to see.
 *
 * By default, it is just `extern`, but since Windows likes being the special
 * one, on Windows (if you build a DLL) it is `__declspec(dllexport)`.
 */
#ifdef DECISION_BUILD_DLL
#define DECISION_API __declspec(dllexport)
#else
#define DECISION_API extern
#endif // DECISION_BUILD_DLL

/* Stop warnings relating to not using "safe" functions. */
#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif // _WIN32

#endif // DCFG_H
