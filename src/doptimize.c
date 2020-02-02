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

// TODO: The fact this runs every time bytecode is removed is REALLY
// inefficient. Maybe instead "mark" which bits to remove and do it all in one
// go at the end?

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

            // TODO: Shrink this code.
            // TODO: Deal with the case that it jumped INTO the deleted
            // region.
            if (opcode == OP_CALLRF || opcode == OP_JRFI ||
                opcode == OP_JRCONFI) {

                fimmediate_t *jmpPtr = (fimmediate_t *)(sheet->_text + i + 1);
                fimmediate_t jmpAmt  = *jmpPtr;

                // If the instruction was going backwards, and it was after the
                // deleted region, we may have a problem.
                if (i >= start && jmpAmt < 0) {
                    // Did it jump over the deleted region?
                    if (i + len + jmpAmt < start) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt + (fimmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }

                // If the instruction was going forwards, and it was before the
                // deleted region, we may have a problem.
                else if (i < start && jmpAmt > 0) {
                    // Did it jump over the deleted region?
                    if (i + jmpAmt >= start + len) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt - (fimmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }
            } else if (opcode == OP_CALLRH || opcode == OP_JRHI ||
                       opcode == OP_JRCONHI) {

                himmediate_t *jmpPtr = (himmediate_t *)(sheet->_text + i + 1);
                himmediate_t jmpAmt  = *jmpPtr;

                // If the instruction was going backwards, and it was after the
                // deleted region, we may have a problem.
                if (i >= start && jmpAmt < 0) {
                    // Did it jump over the deleted region?
                    if (i + len + jmpAmt < start) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt + (himmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }

                // If the instruction was going forwards, and it was before the
                // deleted region, we may have a problem.
                else if (i < start && jmpAmt > 0) {
                    // Did it jump over the deleted region?
                    if (i + jmpAmt >= start + len) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt - (himmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }
            } else if (opcode == OP_CALLRB || opcode == OP_JRBI ||
                       opcode == OP_JRCONBI) {

                bimmediate_t *jmpPtr = (bimmediate_t *)(sheet->_text + i + 1);
                bimmediate_t jmpAmt  = *jmpPtr;

                // If the instruction was going backwards, and it was after the
                // deleted region, we may have a problem.
                if (i >= start && jmpAmt < 0) {
                    // Did it jump over the deleted region?
                    if (i + len + jmpAmt < start) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt + (bimmediate_t)len;
                        *jmpPtr = jmpAmt;
                    }
                }

                // If the instruction was going forwards, and it was before the
                // deleted region, we may have a problem.
                else if (i < start && jmpAmt > 0) {
                    // Did it jump over the deleted region?
                    if (i + jmpAmt >= start + len) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt - (bimmediate_t)len;
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
    bool repeat = true;

    while (repeat) {
        repeat = false;

        VERBOSE(5, "-- Starting an optimisation pass...\n");

        // d_optimize_not_consecutive
        VERBOSE(5, "- Checking for cancelling NOT instructions... ");
        bool consecNot = d_optimize_not_consecutive(sheet);
        if (consecNot) {
            repeat = true;
        }
        VERBOSE(5, "done.\n");

        // d_optimize_useless
        VERBOSE(5, "- Checking for useless instructions... ");
        bool useless = d_optimize_useless(sheet);
        if (useless) {
            repeat = true;
        }
        VERBOSE(5, "done.\n");

        if (repeat) {
            VERBOSE(5,
                    "-- Optimisations were found, starting another pass...\n");
        } else {
            VERBOSE(5, "-- No optimisations were found.\n");
        }
    }

    // These optimisations should only be run once:
    VERBOSE(5, "-- Checking for further optimisations...\n");

    // d_optimize_call_func_relative
    VERBOSE(5,
            "- Checking for absolute calls to functions on the same sheet... ");
    d_optimize_call_func_relative(sheet);
    VERBOSE(5, "done.\n");

    // d_optimize_simplify
    VERBOSE(5, "- Checking if we can simplify instructions... ");
    d_optimize_simplify(sheet);
    VERBOSE(5, "done.\n");

    // d_optimize_shrink_fimmediate
    // NOTE: This should be the last thing to optimise!
    VERBOSE(5, "- Checking if we can shrink instruction operands... ");
    d_optimize_shrink_fimmediate(sheet);
    VERBOSE(5, "done.\n");
}

/**
 * \fn bool d_optimize_not_consecutive(Sheet *sheet)
 * \brief Try and find consecutive NOT instructions..
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
                // We can get rid of these 2 instructions, since they
                // cancel each other out.
                d_optimize_remove_bytecode(sheet, i,
                                           2 * (size_t)d_vm_ins_size(OP_NOT));
                optimized = true;
                deleted   = true;
            }
        }

        // If we've gone wrong somewhere, error and exit.
        if (firstSize == 0) {
            printf("Fatal: (internal:d_optimize_not_consecutive) Byte %zu of "
                   "part-optimized bytecode for sheet %s is not a valid opcode",
                   i, sheet->filePath);
            exit(1);
        }

        if (!deleted) {
            i += firstSize;
        }
    }

    return optimized;
}

/**
 * \fn d_optimize_useless(Sheet *sheet)
 * \brief Try and find useless instructions in the bytecode, e.g. poping 0
 * items.
 *
 * \return If we were able to optimise.
 *
 * \param sheet The sheet containing the bytecode to optimise.
 */
bool d_optimize_useless(Sheet *sheet) {
    bool optimised = false;

    for (size_t i = 0; i < sheet->_textSize;) {
        DIns opcode = sheet->_text[i];

        bool removed = false;

        if (opcode == OP_POPB) {
            bimmediate_t numPop = *(bimmediate_t *)(sheet->_text + i + 1);

            // POPB 0
            if (numPop == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        } else if (opcode == OP_POPH) {
            himmediate_t numPop = *(himmediate_t *)(sheet->_text + i + 1);

            // POPH 0
            if (numPop == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        } else if (opcode == OP_POPF) {
            fimmediate_t numPop = *(fimmediate_t *)(sheet->_text + i + 1);

            // POPF 0
            if (numPop == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        } else if (opcode == OP_PUSHNB) {
            bimmediate_t numPush = *(bimmediate_t *)(sheet->_text + i + 1);

            // PUSHNB 0
            if (numPush == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        } else if (opcode == OP_PUSHNH) {
            himmediate_t numPush = *(himmediate_t *)(sheet->_text + i + 1);

            // PUSHNH 0
            if (numPush == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        } else if (opcode == OP_PUSHNF) {
            fimmediate_t numPush = *(fimmediate_t *)(sheet->_text + i + 1);

            // PUSHNF 0
            if (numPush == 0) {
                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i, d_vm_ins_size(opcode));

                optimised = true;
                removed   = true;
            }
        }

        if (!removed) {
            i += d_vm_ins_size(opcode);
        }
    }

    return optimised;
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
                dint jmp = (fimmediate_t)linkMeta._ptr - itl.ins;

                // The idea is to replace the CALLI instruction with a
                // CALLRF one.
                sheet->_text[itl.ins] = OP_CALLRF;

                // Replace the full immediate.
                linkMeta = sheet->_link.list[itl.link];
                jmp      = (fimmediate_t)linkMeta._ptr - itl.ins;
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

/**
 * \fn bool d_optimize_simplify(Sheet *sheet)
 * \brief Try and find instructions that can be simplified, i.e. POPB 1 = POP.
 *
 * \return If we were able to optimise.
 *
 * \param sheet The sheet containing the bytecode to optimise.
 */
bool d_optimize_simplify(Sheet *sheet) {
    bool optimised = false;

    for (size_t i = 0; i < sheet->_textSize;) {
        DIns opcode = sheet->_text[i];

        if (opcode == OP_RETN) {
            bimmediate_t numRets = *(bimmediate_t *)(sheet->_text + i + 1);

            // RETN 0 == RET
            if (numRets == 0) {
                opcode = OP_RET;

                // Replace the opcode.
                *(sheet->_text + i) = opcode;

                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i + 1, BIMMEDIATE_SIZE);

                optimised = true;
            }
        } else if (opcode == OP_POPB) {
            bimmediate_t numPop = *(bimmediate_t *)(sheet->_text + i + 1);

            // POPB 1 == POP
            if (numPop == 1) {
                opcode = OP_POP;

                // Replace the opcode.
                *(sheet->_text + i) = opcode;

                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i + 1, BIMMEDIATE_SIZE);

                optimised = true;
            }
        } else if (opcode == OP_POPH) {
            himmediate_t numPop = *(himmediate_t *)(sheet->_text + i + 1);

            // POPH 1 == POP
            if (numPop == 1) {
                opcode = OP_POP;

                // Replace the opcode.
                *(sheet->_text + i) = opcode;

                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i + 1, HIMMEDIATE_SIZE);

                optimised = true;
            }
        } else if (opcode == OP_POPF) {
            fimmediate_t numPop = *(fimmediate_t *)(sheet->_text + i + 1);

            // POPF 1 == POP
            if (numPop == 1) {
                opcode = OP_POP;

                // Replace the opcode.
                *(sheet->_text + i) = opcode;

                // Remove the unnessesary bytecode.
                d_optimize_remove_bytecode(sheet, i + 1, FIMMEDIATE_SIZE);

                optimised = true;
            }
        }

        i += d_vm_ins_size(opcode);
    }

    return optimised;
}

// An array of arrays where the first element is the opcode that has the full
// immediate, and the second and third elements are the opcodes that have the
// half and byte immediates, respectively.
#define NUM_SHRINK_FIMMEDIATE_OPS 15
static const DIns SHRINK_FIMMEDIATE_OPS[NUM_SHRINK_FIMMEDIATE_OPS][3] = {
    {OP_ADDFI, OP_ADDHI, OP_ADDBI},       {OP_ANDFI, OP_ANDHI, OP_ANDBI},
    {OP_CALLRF, OP_CALLRH, OP_CALLRB},    {OP_DIVFI, OP_DIVHI, OP_DIVBI},
    {OP_GETFI, OP_GETHI, OP_GETBI},       {OP_JRFI, OP_JRHI, OP_JRBI},
    {OP_JRCONFI, OP_JRCONHI, OP_JRCONBI}, {OP_MODFI, OP_MODHI, OP_MODBI},
    {OP_MULFI, OP_MULHI, OP_MULBI},       {OP_ORFI, OP_ORHI, OP_ORBI},
    {OP_POPF, OP_POPH, OP_POPB},          {OP_PUSHF, OP_PUSHH, OP_PUSHB},
    {OP_PUSHNF, OP_PUSHNH, OP_PUSHNB},    {OP_SUBFI, OP_SUBHI, OP_SUBBI},
    {OP_XORFI, OP_XORHI, OP_XORBI}};

/**
 * \fn bool d_optimize_shrink_fimmediate(Sheet *sheet)
 * \brief For instructions that have full immediate operands, try and replace
 * them with equivalent instructions that use immediates that are smaller, i.e.
 * half and byte immediates.
 *
 * \return If we were able to optimise.
 *
 * \param sheet The sheet containing the bytecode to optimise.
 */
bool d_optimize_shrink_fimmediate(Sheet *sheet) {
    bool optimised = false;

    for (size_t i = 0; i < sheet->_textSize;) {
        DIns opcode = sheet->_text[i];

        // Check first if we should even reduce the immediate here.
        // If this instruction should be linked, then we CANNOT REDUCE IT.
        // Linking relies on the fact that it will be a full immediate.
        bool canOptimise = true;

        for (size_t j = 0; j < sheet->_insLinkListSize; j++) {
            InstructionToLink itl = sheet->_insLinkList[j];

            if (i == itl.ins) {
                canOptimise = false;
                break;
            }
        }

        if (canOptimise) {
            // TODO: Optimise with binary search, or a lookup table?
            int shrinkOpsIndex = -1;
            for (int j = 0; j < NUM_SHRINK_FIMMEDIATE_OPS; j++) {
                if (opcode == SHRINK_FIMMEDIATE_OPS[j][0]) {
                    shrinkOpsIndex = j;
                    break;
                }
            }

            // Is our opcode in the above list?
            if (shrinkOpsIndex >= 0) {
                // Then attempt to shrink it.
                fimmediate_t immediate =
                    *(fimmediate_t *)(sheet->_text + i + 1);

                if (immediate >= BIMMEDIATE_MIN &&
                    immediate <= BIMMEDIATE_MAX) {
                    // The immediate needs to be adjusted for certain
                    // instructions.
                    if (opcode == OP_CALLRF || opcode == OP_JRFI ||
                        opcode == OP_JRCONFI) {
                        if (immediate > 0) {
                            immediate -= (FIMMEDIATE_SIZE - BIMMEDIATE_SIZE);
                        }
                    }

                    opcode = SHRINK_FIMMEDIATE_OPS[shrinkOpsIndex][2];

                    // Replace the opcode.
                    // This needs to be done before removing the bytecode.
                    *(sheet->_text + i) = opcode;

                    // Remove from the bytecode.
                    d_optimize_remove_bytecode(
                        sheet, i + 1, FIMMEDIATE_SIZE - BIMMEDIATE_SIZE);

                    bimmediate_t bImmediate = (bimmediate_t)immediate;
                    *(bimmediate_t *)(sheet->_text + i + 1) = bImmediate;

                    optimised = true;
                } else if (immediate >= HIMMEDIATE_MIN &&
                           immediate <= HIMMEDIATE_MAX) {
                    // The immediate needs to be adjusted for certain
                    // instructions.
                    if (opcode == OP_CALLRF || opcode == OP_JRFI ||
                        opcode == OP_JRCONFI) {
                        if (immediate > 0) {
                            immediate -= (FIMMEDIATE_SIZE - HIMMEDIATE_SIZE);
                        }
                    }

                    opcode = SHRINK_FIMMEDIATE_OPS[shrinkOpsIndex][1];

                    // Replace the opcode.
                    // This needs to be done before removing the bytecode.
                    *(sheet->_text + i) = opcode;

                    // Remove from the bytecode.
                    d_optimize_remove_bytecode(
                        sheet, i + 1, FIMMEDIATE_SIZE - HIMMEDIATE_SIZE);

                    himmediate_t hImmediate = (himmediate_t)immediate;
                    *(himmediate_t *)(sheet->_text + i + 1) = hImmediate;

                    optimised = true;
                }
            }
        }

        i += d_vm_ins_size(opcode);
    }

    return optimised;
}
