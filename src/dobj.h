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
 * \file dobj.h
 * \brief This header deals with Decision object files, i.e. compiled files.
 */

#ifndef DOBJ_H
#define DOBJ_H

#include "dcfg.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn const char *d_obj_generate(Sheet *sheet, size_t *size)
 * \brief Given a sheet has been compiled, create the contents of the sheet's
 * object file.
 *
 * This function is essentially the reverse of `d_obj_load`.
 * 
 * **NOTE:** You cannot compile the sheet if it has any C functions defined in
 * it!
 *
 * \return A malloc'd string of the contents of the future object file.
 *
 * \param sheet The sheet to use to create the object.
 * \param size A pointer to a size that is overwritten with the size of the
 * generated string.
 */
DECISION_API const char *d_obj_generate(struct _sheet *sheet, size_t *size);

/**
 * \fn Sheet *d_obj_load(const char *obj, size_t size,
 *                              const char *filePath)
 * \brief Given a binary object string, create a malloc'd Sheet structure from
 * it.
 *
 * This function is essentially the reverse of `d_obj_generate`.
 *
 * \return The malloc'd sheet generated from the object string.
 *
 * \param obj The object string.
 * \param size The size of the object string.
 * \param filePath Where the object file the object string came from is located.
 */
DECISION_API struct _sheet *d_obj_load(const char *obj, size_t size,
                                       const char *filePath);

#endif // DOBJ_H
