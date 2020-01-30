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

#include "doptimize.h"

#include "dcodegen.h"
#include "decision.h"
#include "dlink.h"
#include "dsheet.h"
#include "dvm.h"

#include <stdlib.h>
#include <string.h>

/**
 * \fn void d_optimize_remove_ins_to_link(Sheet *sheet, size_t index)
 * \brief Remove one instruction-to-link record from a sheet.
 *
 * \param sheet The sheet to remove the record from.
 * \param index The index of the record to remove.
 */
void d_optimize_remove_ins_to_link(Sheet *sheet, size_t index) {
    if (index < sheet->_insLinkListSize) {
        InstructionToLink *moveLinkTo   = sheet->_insLinkList + index;
        InstructionToLink *moveLinkFrom = moveLinkTo + 1;
        memmove(moveLinkTo, moveLinkFrom,
                (sheet->_insLinkListSize - index - 1) *
                    sizeof(InstructionToLink));

        sheet->_insLinkListSize--;
    }
}

/**
 * \fn void d_optimize_remove_bytecode(Sheet *sheet, size_t start, size_t len)
 * \brief Remove a section of bytecode, and make any adjustments to the data
 * that is nessesary.
 *
 * \param sheet The sheet containing the bytecode to remove from.
 * \param start The starting index of the bytecode to remove.
 * \param len How many bytes to remove, starting from `start`.
 */
void d_optimize_remove_bytecode(Sheet *sheet, size_t start, size_t len) {
    if (len > 0) {
        // Shift all instructions after the deleted region to cover up the
        // deleted region using memmove (not memcpy, since the memory overlaps)
        char *moveTo   = sheet->_text + start;
        char *moveFrom = moveTo + len;
        memmove(moveTo, moveFrom, sheet->_textSize - start - len);

        // Set the new size of the bytecode.
        sheet->_textSize = sheet->_textSize - len;

        // NOTE: There will be leftover instructions at the very end of the
        // array that the size won't account for, but it should still be free'd.
        // TODO: Is it worth it to realloc to save memory?

        // By doing this we may have made some instructions that use relative
        // addressing overshoot or undershoot. We need to fix those
        // instructions.
        for (size_t i = 0; i < sheet->_textSize;) {
            DIns opcode                 = sheet->_text[i];
            const unsigned char insSize = d_vm_ins_size(opcode);

            if (i + insSize >= sheet->_textSize)
                break;

            if (opcode == OP_CALLRF || opcode == OP_JRFI ||
                opcode == OP_JRCONFI) {

                fimmediate_t *jmpPtr = (fimmediate_t *)(sheet->_text + i + 1);
                fimmediate_t jmpAmt  = *jmpPtr;

                // TODO: Deal with the case that it jumped INTO the deleted
                // region.

                // If the instruction was going backwards, and it was after the
                // deleted region, we may have a problem.
                if (i >= start && jmpAmt < 0) {
                    // Did it jump over the deleted region?
                    if (i + len + jmpAmt < start) {
                        // Then fix the jump amount.
                        jmpAmt = jmpAmt + (fimmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }

                // If the instruction was going forwards, and it was before the
                // deleted region, we may have a problem.
                else if (i < start && jmpAmt > 0) {
                    // Did it jump over the deleted region?
                    if (i + jmpAmt >= start + len) {
                        // Then fix the jump amount.
                        jmpAmt = jmpAmt - (fimmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }
            }

            // If we've gone wrong somewhere, error and exit.
            if (insSize == 0) {
                printf(
                    "Fatal: (internal:d_optimize_remove_bytecode) Byte %zu of "
                    "part-optimized bytecode for sheet %s is not a valid "
                    "opcode",
                    i, sheet->filePath);
                exit(1);
            }

            i += insSize;
        }

        // Next, we need to edit the LinkMetaList, since the indicies of
        // instructions might have changed.
        for (size_t i = 0; i < sheet->_insLinkListSize; i++) {
            size_t insIndex = sheet->_insLinkList[i].ins;

            if (insIndex >= start && insIndex < start + len) {
                // If instructions that were going to get linked are no longer
                // going to get linked... we need to remove the link. We do
                // this the same way we remove the instructions, but on a
                // different list.
                d_optimize_remove_ins_to_link(sheet, i);

                // Keep the index on the same spot, since the new item has been
                // moved here.
                i--;
            } else if (insIndex >= start + len) {
                sheet->_insLinkList[i].ins = insIndex - len;
            }
        }

        // Next we need to change link metadata that points to instructions in
        // the text section.
        for (size_t i = 0; i < sheet->_link.size; i++) {
            if (sheet->_link.list[i].type == LINK_FUNCTION) {
                char *ptr = sheet->_link.list[i]._ptr;

                // If the function is not in this castle, don't change the
                // pointer!
                if ((intptr_t)ptr != -1) {
                    size_t funcStartIndex = (size_t)ptr;

                    if (funcStartIndex >= start &&
                        funcStartIndex < start + len) {
                        sheet->_link.list[i]._ptr = (char *)start;
                    } else if (funcStartIndex >= start + len) {
                        sheet->_link.list[i]._ptr =
                            (char *)(funcStartIndex - len);
                    }
                }
            }
        }

        // Next, we may need to move the .main integer.
        // Since main points to the first instruction rather than the RET
        // before, we will need to fix the index if the first instruction gets
        // deleted.
        if (sheet->_main >= start && sheet->_main < start + len) {
            sheet->_main = start;
        } else if (sheet->_main >= start + len) {
            sheet->_main = sheet->_main - len;
        }
    }
}

/**
 * \fn void d_optimize_all(Sheet *sheet)
 * \brief Try and optimise all possible senarios.
 *
 * \param sheet The sheet containing the bytecode to optimise.
 */
void d_optimize_all(Sheet *sheet) {
    // Could we try and optimize further?
    bool repeat = false;

    // d_optimize_not_consecutive
    VERBOSE(5, "-- Checking for cancelling NOT operations... ");
    repeat = d_optimize_not_consecutive(sheet);
    if (repeat) {
        while (repeat) {
            repeat = d_optimize_not_consecutive(sheet);
        }
    }
    VERBOSE(5, "done.\n");

    // d_optimize_call_func_relative
    VERBOSE(
        5, "-- Checking for absolute calls to functions on the same sheet... ");
    d_optimize_call_func_relative(sheet);
    VERBOSE(5, "done.\n");

    // TODO: Add function to compress immediate instructions.
}

/**
 * \fn bool d_optimize_not_consecutive(Sheet *sheet)
 * \brief Try and find consecutive NOT instructions that NOT the same register.
 *
 * \return If we were able to optimise.
 *
 * \param sheet The sheet containing the bytecode to optimize.
 */
bool d_optimize_not_consecutive(Sheet *sheet) {
    bool optimized = false;

    for (size_t i = 0; i < sheet->_textSize;) {
        DIns firstOpcode              = sheet->_text[i];
        const unsigned char firstSize = d_vm_ins_size(firstOpcode);
        bool deleted                  = false;

        if (i + firstSize >= sheet->_textSize)
            break;

        if (firstOpcode == OP_NOT) {
            DIns secondOpcode = (sheet->_text[i + firstSize]);

            // Is it a NOT followed by a NOT?
            if (secondOpcode == OP_NOT) {
                // Are the registers the same?
                if (sheet->_text[i + 1] == sheet->_text[i + firstSize + 1]) {
                    // We can get rid of these 2 instructions, since they
                    // cancel each other out.
                    d_optimize_remove_bytecode(
                        sheet, i, 2 * (size_t)d_vm_ins_size(OP_NOT));
                    optimized = true;
                    deleted   = true;
                }
            }
        }

        // If we've gone wrong somewhere, error and exit.
        if (firstSize == 0) {
            printf("Fatal: (internal:d_optimize_not_consecutive) Byte %zu of "
                   "part-optimized bytecode for sheet %s is not a valid opcode",
                   i, sheet->filePath);
            exit(1);
        }

        if (!deleted)
            i += firstSize;
    }

    return optimized;
}

/**
 * \fn bool d_optimize_call_func_relative(Sheet *sheet)
 * \brief Try and find instructions that link to functions that are defined in
 * the same sheet, and if possible, just replace with a relative call rather
 * than an absolute one.
 *
 * \return If we were able to optimise.
 *
 * \param sheet The sheet containing the bytecode to optimize.
 */
bool d_optimize_call_func_relative(Sheet *sheet) {
    bool optimized = false;

    for (size_t i = 0; i < sheet->_insLinkListSize; i++) {
        InstructionToLink itl = sheet->_insLinkList[i];
        LinkMeta linkMeta     = sheet->_link.list[itl.link];

        // Is the link to a function?
        if (linkMeta.type == LINK_FUNCTION && linkMeta.meta != NULL) {
            SheetFunction *func = (SheetFunction *)linkMeta.meta;

            // Check if the function is defined in this sheet.
            if (func->sheet == sheet) {
                // Calculate the jump.
                dint jmp = (size_t)linkMeta._ptr - itl.ins;

                // The idea is to replace the CALLI instruction with a
                // CALLRF one.
                sheet->_text[itl.ins] = OP_CALLRF;

                // Replace the full immediate.
                linkMeta = sheet->_link.list[itl.link];
                jmp      = (size_t)linkMeta._ptr - itl.ins;
                *(fimmediate_t *)(sheet->_text + itl.ins + 1) = jmp;

                // We also remove the link record here since we don't
                // want the linker to overwrite what beautiful art
                // this is.
                d_optimize_remove_ins_to_link(sheet, i);

                // Keep the index on the same spot in the array, since
                // the next InstructionToLink will have moved to this
                // spot.
                i--;

                optimized = true;
            }
        }
    }

    return optimized;
}
