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
 * \file dbool.h
 * \brief Since C doesn't have in-built booleans, I'm defining them here.
 */

#ifndef DBOOL_H
#define DBOOL_H

#ifndef __cplusplus

/**
 * \enum bool
 * \brief A boolean.
 */
typedef enum {
    false, ///< `false`
    true   ///< `true`
} bool;

#endif // __cplusplus

#endif // DBOOL_H
