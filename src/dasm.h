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
 * \file dasm.h
 * \brief Header for reading / manipulating Decision machine code.
 */

#ifndef DASM_H
#define DASM_H

#include "dcfg.h"
#include "dlink.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* Forward declaration of the InstructionToLink struct from dsheet.h */
struct _insToLink;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_asm_text_dump(char *code, size_t size)
 * \brief De-assemble Decision machine code, and print it to stdout.
 *
 * \param code The machine code array to print.
 * \param size The size of the machine code array.
 */
DECISION_API void d_asm_text_dump(char *code, size_t size);

/**
 * \fn void d_asm_data_dump(char *data, size_t size)
 * \brief Print the data section in hex format.
 *
 * \param data Pointer to the beginning of the data section.
 * \param size The size of the data section.
 */
DECISION_API void d_asm_data_dump(char *data, size_t size);

/**
 * \fn void d_asm_lmeta_dump(LinkMetaList meta)
 * \brief Print the lmeta section.
 *
 * \param meta The link meta list to print.
 */
DECISION_API void d_asm_lmeta_dump(LinkMetaList meta);

/**
 * \fn void d_asm_link_dump(InstructionToLink *list, size_t size)
 * \brief Print the link section.
 *
 * \param list The list of relational records to print.
 * \param size The size of the list.
 */
DECISION_API void d_asm_link_dump(struct _insToLink *list, size_t size);

/**
 * \fn void d_asm_incl_dump(Sheet **includes, size_t size)
 * \brief Print the incl section.
 *
 * \param includes The list of sheets that are included.
 * \param size The size of the list.
 */
DECISION_API void d_asm_incl_dump(struct _sheet **includes, size_t size);

/**
 * \fn void d_asm_dump_all(Sheet *sheet)
 * \brief Dump all of the sections of a sheet object.
 *
 * \param sheet The sheet object to dump.
 */
DECISION_API void d_asm_dump_all(struct _sheet *sheet);

/**
 * \fn const char *d_asm_generate_object(Sheet *sheet, size_t *size)
 * \brief Given a sheet has been compiled, create the contents of the sheet's
 * object file.
 *
 * This function is essentially the reverse of `d_asm_load_object`.
 *
 * \return A malloc'd string of the contents of the future object file.
 *
 * \param sheet The sheet to use to create the object.
 * \param size A pointer to a size that is overwritten with the size of the
 * generated string.
 */
DECISION_API const char *d_asm_generate_object(struct _sheet *sheet,
                                               size_t *size);

/**
 * \fn Sheet *d_asm_load_object(const char *obj, size_t size,
 *                              const char *filePath)
 * \brief Given a binary object string, create a malloc'd Sheet structure from
 * it.
 *
 * This function is essentially the reverse of `d_asm_generate_object`.
 *
 * \return The malloc'd sheet generated from the object string.
 *
 * \param obj The object string.
 * \param size The size of the object string.
 * \param filePath Where the object file the object string came from is located.
 */
DECISION_API struct _sheet *d_asm_load_object(const char *obj, size_t size,
                                              const char *filePath);

#endif // DASM_H