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

#include "dasm.h"

#include "dcfunc.h"
#include "decision.h"
#include "dlink.h"
#include "dmalloc.h"
#include "dsheet.h"
#include "dtype.h"
#include "dvm.h"

#include <stdlib.h>
#include <string.h>

/**
 * \fn BCode d_malloc_bytecode(size_t size)
 * \brief Create a malloc'd BCode object, with a set number of bytes.
 *
 * \return The BCode object with malloc'd elements.
 *
 * \param size The number of bytes.
 */
BCode d_malloc_bytecode(size_t size) {
    BCode out;

    if (size > 0) {
        out.code = d_calloc(size, sizeof(char));
    } else {
        out.code = NULL;
    }
    
    out.size = size;

    out.linkList     = NULL;
    out.linkListSize = 0;

    out.debugInfo = NO_DEBUG_INFO;

    return out;
}

/**
 * \fn BCode d_bytecode_ins(DIns opcode)
 * \brief Quickly create bytecode that is the size of an opcode, which also has
 * its first byte set as the opcode itself.
 *
 * \return The opcode-initialised bytecode.
 *
 * \param opcode The opcode to initialise with.
 */
BCode d_bytecode_ins(DIns opcode) {
    BCode out = d_malloc_bytecode(d_vm_ins_size(opcode));
    d_bytecode_set_byte(out, 0, (char)opcode);
    return out;
}

/**
 * \fn void d_bytecode_set_byte(BCode bcode, size_t index, char byte)
 * \brief Given some bytecode, set a byte in the bytecode to a given value.
 *
 * \param bcode The bytecode to edit.
 * \param index The index of the byte in the bytecode to set.
 * \param byte The value to set.
 */
void d_bytecode_set_byte(BCode bcode, size_t index, char byte) {
    if (index < bcode.size) {
        bcode.code[index] = byte;
    }
}

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
void d_bytecode_set_fimmediate(BCode bcode, size_t index,
                               fimmediate_t fimmediate) {
    if (index < bcode.size - FIMMEDIATE_SIZE + 1) {
        fimmediate_t *ptr = (fimmediate_t *)(bcode.code + index);
        *ptr              = fimmediate;
    }
}

/**
 * \fn void d_free_bytecode(BCode *bcode)
 * \brief Free malloc'd elements of bytecode.
 *
 * \param bcode The bytecode to free.
 */
void d_free_bytecode(BCode *bcode) {
    if (bcode->code != NULL) {
        free(bcode->code);
    }

    if (bcode->linkList != NULL) {
        free(bcode->linkList);
    }

    bcode->code         = NULL;
    bcode->size         = 0;
    bcode->linkList     = NULL;
    bcode->linkListSize = 0;

    d_debug_free_info(&(bcode->debugInfo));
}

/**
 * \fn void d_concat_bytecode(BCode *base, BCode *after)
 * \brief Append bytecode to the end of another set of bytecode.
 *
 * \param base The bytecode to be added to.
 * \param after The bytecode to append. Not changed.
 */
void d_concat_bytecode(BCode *base, BCode *after) {
    if (!(after->code == NULL || after->size == 0)) {
        size_t totalSize = base->size + after->size;

        if (base->code != NULL) {
            base->code = d_realloc(base->code, totalSize);
        } else {
            base->code = d_calloc(totalSize, sizeof(char));
        }

        // Pointer to the place we want to start adding "after".
        // We haven't changed base->size yet!
        char *addPtr = base->code + base->size;

        memcpy(addPtr, after->code, after->size);

        // Now we've concatenated the bytecode, we need to add the links as
        // well, but change the index of the instruction they point to, since
        // it's now changed.
        if (after->linkList != NULL && after->linkListSize > 0) {
            size_t totalLinkSize = base->linkListSize + after->linkListSize;

            if (base->linkList != NULL) {
                base->linkList = d_realloc(
                    base->linkList, totalLinkSize * sizeof(InstructionToLink));
            } else {
                base->linkList =
                    d_calloc(totalLinkSize, sizeof(InstructionToLink));
            }

            // i is the index in the base list.
            // j is the index in the after list.
            for (size_t i = base->linkListSize; i < totalLinkSize; i++) {
                size_t j = i - base->linkListSize;

                InstructionToLink newIns = after->linkList[j];
                newIns.ins += base->size;
                base->linkList[i] = newIns;
            }

            base->linkListSize = totalLinkSize;
        }

        // Add the after debug into to the before debug info, and correct the
        // instructions along the way.
        DebugInfo afterDebugInfo = after->debugInfo;

        if (afterDebugInfo.insValueInfoList != NULL && afterDebugInfo.insValueInfoSize > 0) {
            for (size_t i = 0; i < afterDebugInfo.insValueInfoSize; i++) {
                InsValueInfo valueInfo = afterDebugInfo.insValueInfoList[i];

                valueInfo.ins += base->size;

                d_debug_add_value_info(&(base->debugInfo), valueInfo);
            }
        }

        if (afterDebugInfo.insExecInfoList != NULL && afterDebugInfo.insExecInfoSize > 0) {
            for (size_t i = 0; i < afterDebugInfo.insExecInfoSize; i++) {
                InsExecInfo execInfo = afterDebugInfo.insExecInfoList[i];

                execInfo.ins += base->size;

                d_debug_add_exec_info(&(base->debugInfo), execInfo);
            }
        }

        if (afterDebugInfo.insNodeInfoList != NULL && afterDebugInfo.insNodeInfoSize > 0) {
            for (size_t i = 0; i < afterDebugInfo.insNodeInfoSize; i++) {
                InsNodeInfo nodeInfo = afterDebugInfo.insNodeInfoList[i];

                nodeInfo.ins += base->size;

                d_debug_add_node_info(&(base->debugInfo), nodeInfo);
            }
        }

        base->size = totalSize;
    }
}

/* A macro constant to state the number of columns d_asm_data_dump prints. */
#define DATA_DUMP_NUM_COLS 16

/* An array of mnemonics, where the index matches the opcode. */
static const char *MNEMONICS[NUM_OPCODES] = {
    "RET",     "RETN",   "ADD",    "ADDF",   "ADDBI",   "ADDHI",   "ADDFI",
    "AND",     "ANDBI",  "ANDHI",  "ANDFI",  "CALL",    "CALLC",   "CALLCI",
    "CALLI",   "CALLR",  "CALLRB", "CALLRH", "CALLRF",  "CEQ",     "CEQF",
    "CLEQ",    "CLEQF",  "CLT",    "CLTF",   "CMEQ",    "CMEQF",   "CMT",
    "CMTF",    "CVTF",   "CVTI",   "DEREF",  "DEREFI",  "DEREFB",  "DEREFBI",
    "DIV",     "DIVF",   "DIVBI",  "DIVHI",  "DIVFI",   "GET",     "GETBI",
    "GETHI",   "GETFI",  "INV",    "J",      "JCON",    "JCONI",   "JI",
    "JR",      "JRBI",   "JRHI",   "JRFI",   "JRCON",   "JRCONBI", "JRCONHI",
    "JRCONFI", "MOD",    "MODBI",  "MODHI",  "MODFI",   "MUL",     "MULF",
    "MULBI",   "MULHI",  "MULFI",  "NOT",    "OR",      "ORBI",    "ORHI",
    "ORFI",    "POP",    "POPB",   "POPH",   "POPF",    "PUSHB",   "PUSHH",
    "PUSHF",   "PUSHNB", "PUSHNH", "PUSHNF", "SETADR",  "SETADRB", "SUB",
    "SUBF",    "SUBBI",  "SUBHI",  "SUBFI",  "SYSCALL", "XOR",     "XORBI",
    "XORHI",   "XORFI"};

/**
 * \fn void d_asm_text_dump(char *code, size_t size)
 * \brief De-assemble Decision machine code, and print it to stdout.
 *
 * \param code The machine code array to print.
 * \param size The size of the machine code array.
 */
DECISION_API void d_asm_text_dump(char *code, size_t size) {
    for (size_t i = 0; i < size;) {
        char *ins   = code + i;
        DIns opcode = *ins;

        // Get the string representation of the opcode, i.e. the mnemonic.
        const char *mnemonic;

        if (opcode < NUM_OPCODES)
            mnemonic = MNEMONICS[opcode];
        else
            mnemonic = "UNDEFINED";

        const unsigned char insSize = d_vm_ins_size(opcode);

        const size_t maxInsSize = 1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE;

        // Print the index of the instruction, the actual instruction, and the
        // mnemonic.
        printf("%8zx\t", i);

        for (size_t j = 0; j < maxInsSize; j++) {
            if (j < insSize) {
                printf("%02hhx ", *(ins + j));
            } else {
                printf("   ");
            }
        }

        printf("\t%s ", mnemonic);

        bimmediate_t b2;

        // We group together instructions with the same "format" to simplify.
        switch (opcode) {
            // Byte Immediate.
            case OP_RETN:
            case OP_ADDBI:
            case OP_ANDBI:
            case OP_CALL:
            case OP_CALLC:
            case OP_CALLR:
            case OP_DIVBI:
            case OP_GETBI:
            case OP_JRBI:
            case OP_JRCONBI:
            case OP_MODBI:
            case OP_MULBI:
            case OP_ORBI:
            case OP_POPB:
            case OP_PUSHB:
            case OP_PUSHNB:
            case OP_SUBBI:
            case OP_SYSCALL:
            case OP_XORBI:;
                bimmediate_t b = *(bimmediate_t *)(ins + 1);
                printf("0x%" BIMMEDIATE_PRINTF "x (%" BIMMEDIATE_PRINTF "d)", b,
                       b);
                break;

            // Half Immediate.
            case OP_ADDHI:
            case OP_ANDHI:
            case OP_DIVHI:
            case OP_GETHI:
            case OP_JRHI:
            case OP_JRCONHI:
            case OP_MODHI:
            case OP_MULHI:
            case OP_ORHI:
            case OP_POPH:
            case OP_PUSHH:
            case OP_PUSHNH:
            case OP_SUBHI:
            case OP_XORHI:;
                himmediate_t h = *(himmediate_t *)(ins + 1);
                printf("0x%" HIMMEDIATE_PRINTF "x (%" HIMMEDIATE_PRINTF "d)", h,
                       h);
                break;

            // Full Immediate.
            case OP_ADDFI:
            case OP_ANDFI:
            case OP_DEREFI:
            case OP_DEREFBI:
            case OP_DIVFI:
            case OP_GETFI:
            case OP_JCONI:
            case OP_JI:
            case OP_JRFI:
            case OP_JRCONFI:
            case OP_MODFI:
            case OP_MULFI:
            case OP_ORFI:
            case OP_POPF:
            case OP_PUSHF:
            case OP_PUSHNF:
            case OP_SUBFI:
            case OP_XORFI:;
                fimmediate_t f = *(fimmediate_t *)(ins + 1);
                printf("0x%" FIMMEDIATE_PRINTF "x (%" FIMMEDIATE_PRINTF "d)", f,
                       f);
                break;

            // Byte Immediate + Byte Immediate.
            case OP_CALLRB:;
                bimmediate_t b1 = *(bimmediate_t *)(ins + 1);
                b2              = *(bimmediate_t *)(ins + 1 + BIMMEDIATE_SIZE);
                printf("0x%" BIMMEDIATE_PRINTF "x (%" BIMMEDIATE_PRINTF
                       "d), 0x%" BIMMEDIATE_PRINTF "x (%" BIMMEDIATE_PRINTF
                       "d)",
                       b1, b1, b2, b2);
                break;

            // Half Immediate + Byte Immediate.
            case OP_CALLRH:;
                himmediate_t h1 = *(himmediate_t *)(ins + 1);
                b2              = *(bimmediate_t *)(ins + 1 + HIMMEDIATE_SIZE);
                printf("0x%" HIMMEDIATE_PRINTF "x (%" HIMMEDIATE_PRINTF
                       "d), 0x%" BIMMEDIATE_PRINTF "x (%" BIMMEDIATE_PRINTF
                       "d)",
                       h1, h1, b2, b2);
                break;

            // Full Immediate + Byte Immediate.
            case OP_CALLCI:
            case OP_CALLI:
            case OP_CALLRF:;
                fimmediate_t f1 = *(fimmediate_t *)(ins + 1);
                b2              = *(bimmediate_t *)(ins + 1 + FIMMEDIATE_SIZE);
                printf("0x%" FIMMEDIATE_PRINTF "x (%" FIMMEDIATE_PRINTF
                       "d), 0x%" BIMMEDIATE_PRINTF "x (%" BIMMEDIATE_PRINTF
                       "d)",
                       f1, f1, b2, b2);
                break;

            // No immediates.
            default:
                break;
        }

        printf("\n");

        // If the text section is borked, don't go into an infinite loop.
        if (insSize == 0)
            break;

        i += insSize;
    }
}

/**
 * \fn void d_asm_data_dump(char *data, size_t size)
 * \brief Print the data section in hex format.
 *
 * \param data Pointer to the beginning of the data section.
 * \param size The size of the data section.
 */
void d_asm_data_dump(char *data, size_t size) {
    printf("           ");

    // First, print the column headers.
    for (size_t c = 0; c < DATA_DUMP_NUM_COLS; c++) {
        printf("%zx  ", c);
    }

    printf("\n");

    // Now, print of the bytes.
    char *ptr        = data;
    char *startOfRow = data;
    size_t index     = 0;

    while (index < size) {
        if (index % DATA_DUMP_NUM_COLS == 0) {
            printf("0x%08zx ", index);
            startOfRow = ptr;
        }

        printf("%02hhx ", *ptr);

        ptr++;
        index++;

        // We've reached the end of the row, print the characters.
        if (index % DATA_DUMP_NUM_COLS == 0) {
            while (startOfRow < ptr) {
                if ((*startOfRow >= 'a' && *startOfRow <= 'z') ||
                    (*startOfRow >= 'A' && *startOfRow <= 'Z') ||
                    (*startOfRow >= '0' && *startOfRow <= '9')) {
                    printf("%c", *startOfRow);
                } else
                    printf(".");

                startOfRow++;
            }

            printf("\n");
        }
    }

    // Get to where the characters are printed off,
    while (index % DATA_DUMP_NUM_COLS != 0) {
        printf("   ");
        index++;
    }

    // Print off the remaining characters.
    while (startOfRow < ptr) {
        if ((*startOfRow >= 'a' && *startOfRow <= 'z') ||
            (*startOfRow >= 'A' && *startOfRow <= 'Z') ||
            (*startOfRow >= '0' && *startOfRow <= '9')) {
            printf("%c", *startOfRow);
        } else
            printf(".");

        startOfRow++;
    }

    printf("\n");
}

/**
 * \fn void d_asm_lmeta_dump(LinkMetaList meta)
 * \brief Print the lmeta section.
 *
 * \param meta The link meta list to print.
 */
void d_asm_lmeta_dump(LinkMetaList meta) {
    for (size_t i = 0; i < meta.size; i++) {
        LinkMeta lm = meta.list[i];

        printf("%08zu: Type: %u Name: %s Pointer: %p\n", i, lm.type, lm.name,
               lm._ptr);
    }
}

/**
 * \fn void d_asm_link_dump(InstructionToLink *list, size_t size)
 * \brief Print the link section.
 *
 * \param list The list of relational records to print.
 * \param size The size of the list.
 */
void d_asm_link_dump(InstructionToLink *list, size_t size) {
    for (size_t i = 0; i < size; i++) {
        InstructionToLink itl = list[i];

        printf("INS %8zx -> LINK %8zu\n", itl.ins, itl.link);
    }
}

/**
 * \fn void d_asm_incl_dump(Sheet **includes, size_t size)
 * \brief Print the incl section.
 *
 * \param includes The list of sheets that are included.
 * \param size The size of the list.
 */
void d_asm_incl_dump(struct _sheet **includes, size_t size) {
    Sheet **includePtr = includes;

    for (size_t i = 0; i < size; i++) {
        const char *includePath = (*includePtr)->filePath;
        if ((*includePtr)->includePath != NULL)
            includePath = (*includePtr)->includePath;

        printf("%s\n", includePath);

        includePtr++;
    }
}

/**
 * \fn void d_asm_dump_all(Sheet *sheet)
 * \brief Dump all of the sections of a sheet object.
 *
 * \param sheet The sheet object to dump.
 */
void d_asm_dump_all(Sheet *sheet) {
    printf("\n.text (%p):\n", sheet->_text);
    d_asm_text_dump(sheet->_text, sheet->_textSize);

    printf("\n.main:\n%08zx\n", sheet->_main);

    printf("\n.data (%p):\n", sheet->_data);
    d_asm_data_dump(sheet->_data, sheet->_dataSize);

    printf("\n.lmeta:\n");
    d_asm_lmeta_dump(sheet->_link);

    printf("\n.link:\n");
    d_asm_link_dump(sheet->_insLinkList, sheet->_insLinkListSize);

    // Here we just cheat and use the dumping function from dsheet.h, since
    // it does what we need it to do anyway.
    printf("\n.func:\n");
    d_functions_dump(sheet->functions, sheet->numFunctions);

    // We cheat again by using the dumping function from dsheet.h
    printf("\n.var:\n");
    d_variables_dump(sheet->variables, sheet->numVariables);

    printf("\n.incl:\n");
    d_asm_incl_dump(sheet->includes, sheet->numIncludes);

    printf("\n");
}