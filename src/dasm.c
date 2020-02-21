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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A macro constant to state the number of columns d_asm_data_dump prints. */
#define DATA_DUMP_NUM_COLS 16

/* An array of mnemonics, where the index matches the opcode. */
static const char *MNEMONICS[NUM_OPCODES] = {
    "RET",    "RETN",   "ADD",    "ADDF",    "ADDBI",   "ADDHI",   "ADDFI",
    "AND",    "ANDBI",  "ANDHI",  "ANDFI",   "CALL",    "CALLC",   "CALLCI",
    "CALLI",  "CALLR",  "CALLRB", "CALLRH",  "CALLRF",  "CEQ",     "CEQF",
    "CLEQ",   "CLEQF",  "CLT",    "CLTF",    "CMEQ",    "CMEQF",   "CMT",
    "CMTF",   "CVTF",   "CVTI",   "DEREF",   "DEREFI",  "DEREFB",  "DEREFBI",
    "DIV",    "DIVF",   "DIVBI",  "DIVHI",   "DIVFI",   "GET",     "GETBI",
    "GETHI",  "GETFI",  "J",      "JCON",    "JCONI",   "JI",      "JR",
    "JRBI",   "JRHI",   "JRFI",   "JRCON",   "JRCONBI", "JRCONHI", "JRCONFI",
    "MOD",    "MODBI",  "MODHI",  "MODFI",   "MUL",     "MULF",    "MULBI",
    "MULHI",  "MULFI",  "NOT",    "OR",      "ORBI",    "ORHI",    "ORFI",
    "POP",    "POPB",   "POPH",   "POPF",    "PUSHB",   "PUSHH",   "PUSHF",
    "PUSHNB", "PUSHNH", "PUSHNF", "SETADR",  "SETADRB", "SUB",     "SUBF",
    "SUBBI",  "SUBHI",  "SUBFI",  "SYSCALL", "XOR",     "XORBI",   "XORHI",
    "XORFI"};

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
}