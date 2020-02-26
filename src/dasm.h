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
 * \file dasm.h
 * \brief Header for reading / manipulating Decision machine code.
 */

#ifndef DASM_H
#define DASM_H

#include "dcfg.h"
#include "ddebug.h"
#include "dlink.h"
#include "dvm.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* Forward declaration of the InstructionToLink struct from dsheet.h */
struct _insToLink;

/**
 * \struct _bcode
 * \brief A struct for generic Decision bytecode.
 *
 * \typedef struct _bcode BCode
 */
typedef struct _bcode {
    char *code;  ///< The bytecode as an array of bytes.
    size_t size; ///< The size of the bytecode in bytes.

    struct _insToLink *linkList; ///< An array of instructions that will need
                                 ///< to be linked.
    size_t linkListSize;         ///< The size of the `linkList` array.

    DebugInfo debugInfo;
} BCode;

/*
=== STRUCTURE FUNCTIONS ===================================
*/

/**
 * \fn BCode d_malloc_bytecode(size_t size)
 * \brief Create a malloc'd BCode object, with a set number of bytes.
 *
 * \return The BCode object with malloc'd elements.
 *
 * \param size The number of bytes.
 */
DECISION_API BCode d_malloc_bytecode(size_t size);

/**
 * \fn BCode d_bytecode_ins(DIns opcode)
 * \brief Quickly create bytecode that is the size of an opcode, which also has
 * its first byte set as the opcode itself.
 *
 * \return The opcode-initialised bytecode.
 *
 * \param opcode The opcode to initialise with.
 */
DECISION_API BCode d_bytecode_ins(DIns opcode);

/**
 * \fn void d_bytecode_set_byte(BCode bcode, size_t index, char byte)
 * \brief Given some bytecode, set a byte in the bytecode to a given value.
 *
 * \param bcode The bytecode to edit.
 * \param index The index of the byte in the bytecode to set.
 * \param byte The value to set.
 */
DECISION_API void d_bytecode_set_byte(BCode bcode, size_t index, char byte);

/**
 * \fn void d_bytecode_set_fimmediate(BCode bcode, size_t index,
 *                                   fimmediate_t fimmediate)
 * \brief Given some bytecode, set a full immediate value into the bytecode.
 *
 * **NOTE:** There are no functions to set byte or half immediates for a good
 * reason: Mixing immediate sizes during code generation is a bad idea, as
 * inserting bytecode in the middle of another bit of bytecode could make some
 * smaller immediates invalid, and they would have to increase in size, which
 * would be a pain. Instead, we only work with full immediates during code
 * generation, and reduce down the full immediate instructions to byte or
 * half immediate instructions in the optimisation stage.
 *
 * \param bcode The bytecode to edit.
 * \param index The starting index of the section of the bytecode to edit.
 * \param fimmediate The full immediate value to set.
 */
DECISION_API void d_bytecode_set_fimmediate(BCode bcode, size_t index,
                                            fimmediate_t fimmediate);

/**
 * \fn void d_free_bytecode(BCode *bcode)
 * \brief Free malloc'd elements of bytecode.
 *
 * \param bcode The bytecode to free.
 */
DECISION_API void d_free_bytecode(BCode *bcode);

/**
 * \fn void d_concat_bytecode(BCode *base, BCode *after)
 * \brief Append bytecode to the end of another set of bytecode.
 *
 * \param base The bytecode to be added to.
 * \param after The bytecode to append. Not changed.
 */
DECISION_API void d_concat_bytecode(BCode *base, BCode *after);

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

#endif // DASM_H
