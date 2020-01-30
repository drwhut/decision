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

#include "dasm.h"

#include "dcfunc.h"
#include "decision.h"
#include "dlink.h"
#include "dmalloc.h"
#include "dsheet.h"
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

        // Print the index of the instruction, the actual instruction, and the
        // mnemonic.
        printf("%8zx\t", i);

        for (size_t j = 0; j < sizeof(dint); j++) {
            if (j < insSize)
                printf("%02hhx ", *(ins + j));
            else
                printf("   ");
        }

        printf("\t%s ", mnemonic);

        bimmediate_t b1;

        // We group together instructions with the same "format" to simplify.
        switch (opcode) {
            // Byte Immediate.
            case OP_RETN:
            case OP_ADDBI:
            case OP_ANDBI:
            case OP_CALL:
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
            case OP_XORBI:
                printf("0x%" BIMMEDIATE_PRINTF "x", *(bimmediate_t *)(ins + 1));
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
            case OP_XORHI:
                printf("0x%" HIMMEDIATE_PRINTF "x", *(himmediate_t *)(ins + 1));
                break;

            // Full Immediate.
            case OP_ADDFI:
            case OP_ANDFI:
            case OP_CALLCI:
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
            case OP_XORFI:
                printf("0x%" FIMMEDIATE_PRINTF "x", *(fimmediate_t *)(ins + 1));
                break;

            // Byte Immediate + Byte Immediate.
            case OP_CALLRB:
                b1              = *(bimmediate_t *)(ins + 1);
                bimmediate_t b2 = *(bimmediate_t *)(ins + 1 + BIMMEDIATE_SIZE);
                printf("0x%" BIMMEDIATE_PRINTF "x, 0x%" BIMMEDIATE_PRINTF "x",
                       b1, b2);
                break;

            // Half Immediate + Byte Immediate.
            case OP_CALLRH:
                b1              = *(bimmediate_t *)(ins + 1);
                himmediate_t h2 = *(himmediate_t *)(ins + 1 + BIMMEDIATE_SIZE);
                printf("0x%" BIMMEDIATE_PRINTF "x, 0x%" HIMMEDIATE_PRINTF "x",
                       b1, h2);
                break;
            
            // Full Immediate + Byte Immediate.
            case OP_CALLI:
            case OP_CALLRF:
                b1              = *(bimmediate_t *)(ins + 1);
                fimmediate_t f2 = *(fimmediate_t *)(ins + 1 + BIMMEDIATE_SIZE);
                printf("0x%" BIMMEDIATE_PRINTF "x, 0x%" FIMMEDIATE_PRINTF "x",
                       b1, f2);
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

        printf("%08zu: Type: %d Name: %s Pointer: %p\n", i, lm.type, lm.name,
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
const char *d_asm_generate_object(Sheet *sheet, size_t *size) {
    // Firstly, we need to figure out the size of the string.

    size_t lenLinkMeta = 0;
    for (size_t i = 0; i < sheet->_link.size; i++) {
        LinkMeta lm = sheet->_link.list[i];

        // 1 byte for the type, variable for the name + NULL, and the pointer.
        lenLinkMeta += 1 + strlen(lm.name) + 1 + sizeof(dint);
    }

    size_t len = 3 +                // Either D32 or D64, depending on if
                                    // DECISION_32 is defined.
                 5 +                // ".text"
                 sizeof(duint) +    // Unsigned integer to state the size of
                                    // the next section.
                 sheet->_textSize + // To store the .text section.
                 5 +                // ".main"
                 sizeof(duint) +    // To store the .main section.
                 5 +                // ".data"
                 sizeof(duint) +    // Unsigned integer to state the size of
                                    // the next section.
                 sheet->_dataSize + // To store the .data section.
                 6 +                // ".lmeta"
                 sizeof(duint) +    // Unsigned integer to state the size of
                                    // the next section.
                 lenLinkMeta +      // To store the .lmeta section.
                 5 +                // ".link"
                 sizeof(duint) +    // Unsigned integer to state the size of
                                    // the next section.
                 sheet->_insLinkListSize * 2 * // To store the .link section.
                     sizeof(duint) +           //
                 5 +                           // ".func"
                 sizeof(duint) + // Unsigned integer to state the size of
                                 // the next section.
                 4 +             // ".var"
                 sizeof(duint) + // Unsigned integer to state the size of
                                 // the next section.
                 5 +             // ".incl"
                 sizeof(duint) + // Unsigned integer to state the size of
                                 // the next section.
                 2 +             // ".c"
                 sizeof(duint) + // Unsigned integer to state the size of
                                 // the next section.
                 2;              // '.' and \0 at the end of the string.

    // The .func section size can highly vary, so it gets it own special spot
    // right here.
    size_t funcLen = 0;

    SheetFunction *func = sheet->functions;

    for (size_t funcIndex = 0; funcIndex < sheet->numFunctions; funcIndex++) {
        funcLen += sizeof(duint) + // Index of the function in .lmeta
                   1 +             // isSubroutine.
                   sizeof(duint) + // Number of arguments.
                   sizeof(duint);  // Number of returns.

        SheetVariable *arg = func->arguments;

        for (size_t argIndex = 0; argIndex < func->numArguments; argIndex++) {
            size_t sizeOfNamePlusNull = 1;

            if (arg->name != NULL)
                sizeOfNamePlusNull = sizeOfNamePlusNull + strlen(arg->name);

            funcLen += 1 + // 0 = Argument.
                       1;  // DType dataType.

            // The default value.
            if (arg->dataType == TYPE_STRING) {
                if (arg->defaultValue.stringValue != NULL) {
                    funcLen += strlen(arg->defaultValue.stringValue) + 1;
                } else
                    funcLen += 1; // 0 to say no default value.
            } else
                funcLen += sizeof(dint); // LexData defaultValue.

            funcLen += sizeOfNamePlusNull; // Name + NULL.

            arg++;
        }

        SheetVariable *ret = func->returns;

        for (size_t retIndex = 0; retIndex < func->numReturns; retIndex++) {
            size_t sizeOfNamePlusNull = 1;

            if (ret->name != NULL)
                sizeOfNamePlusNull = sizeOfNamePlusNull + strlen(ret->name);

            funcLen += 1 + // 1 = Return.
                       1 + // DType dataType.
                       // sizeof(dint) +   // LexData defaultValue.
                       sizeOfNamePlusNull; // Name + NULL.

            ret++;
        }

        func++;
    }

    len += funcLen;

    // The .var section can highly vary as well.
    size_t varLen = 0;

    SheetVariable *var = sheet->variables;

    for (size_t varIndex = 0; varIndex < sheet->numVariables; varIndex++) {
        varLen += sizeof(duint) + // Index of variable in LinkMeta array.
                  sizeof(duint) + // Index of variable's string default value in
                                  // LinkMeta.
                  1;              // Data type.

        var++;
    }

    len += varLen;

    // The .incl section can highly vary as well.
    size_t inclLen = 0;

    Sheet **includePtr = sheet->includes;

    for (size_t inclIndex = 0; inclIndex < sheet->numIncludes; inclIndex++) {
        const char *includePath = (*includePtr)->filePath;
        if ((*includePtr)->includePath != NULL)
            includePath = (*includePtr)->includePath;

        inclLen += strlen(includePath) + 1;

        includePtr++;
    }

    len += inclLen;

    // The .c section can vary as well.
    size_t cLen = 0;

    // We only need to put C functions that we use in this list.
    for (size_t metaIndex = 0; metaIndex < sheet->_link.size; metaIndex++) {
        LinkMeta meta = sheet->_link.list[metaIndex];

        if (meta.type == LINK_CFUNCTION) {
            CFunction *cFunc = (CFunction *)meta.meta;

            // The name of the C function, along with it's inputs and outputs,
            // which are TYPE_NONE terminated.
            cLen += strlen(cFunc->name) + 1 +
                    (cFunc->numInputs + cFunc->numOutputs + 2) * sizeof(DType);
        }
    }

    len += cLen;

    *size = len;

    // Allocate memory to the string.
    char *out = (char *)d_malloc(len);

    // A pointer to where we want to put content in the string.
    char *ptr = out;

// D32 or D64
#ifdef DECISION_32
    memcpy(ptr, "D32", 3);
#else
    memcpy(ptr, "D64", 3);
#endif // DECISION_32
    ptr += 3;

    // END OF METADATA
    // ".text"
    memcpy(ptr, ".text", 5);
    ptr += 5;

    // sizeof(.text)
    duint sizeOfSection = (duint)(sheet->_textSize);
    memcpy(ptr, &sizeOfSection, sizeof(duint));
    ptr += sizeof(duint);

    // .text section.
    memcpy(ptr, sheet->_text, sizeOfSection);
    ptr += sizeOfSection;

    // ".main"
    memcpy(ptr, ".main", 5);
    ptr += 5;

    // .main section.
    memcpy(ptr, &(sheet->_main), sizeof(duint));
    ptr += sizeof(duint);

    // ".data"
    memcpy(ptr, ".data", 5);
    ptr += 5;

    // sizeof(.data)
    sizeOfSection = (duint)(sheet->_dataSize);
    memcpy(ptr, &sizeOfSection, sizeof(duint));
    ptr += sizeof(duint);

    // .data section.
    memcpy(ptr, sheet->_data, sizeOfSection);
    ptr += sizeOfSection;

    // ".lmeta"
    memcpy(ptr, ".lmeta", 6);
    ptr += 6;

    // sizeof(.lmeta)
    sizeOfSection = lenLinkMeta;
    memcpy(ptr, &sizeOfSection, sizeof(duint));
    ptr += sizeof(duint);

    // .lmeta section.
    for (size_t i = 0; i < sheet->_link.size; i++) {
        LinkMeta lm = sheet->_link.list[i];

        *ptr = lm.type;
        ptr++;

        size_t sizeOfNamePlusNull = strlen(lm.name) + 1;
        memcpy(ptr, lm.name, sizeOfNamePlusNull);
        ptr += sizeOfNamePlusNull;

        // If the object is in another sheet, we can't store the pointer as it
        // is now! We will need to re-calculate it when we run the object after
        // it is built.
        char *newPtr = lm._ptr;

        if (lm.type == LINK_VARIABLE || lm.type == LINK_VARIABLE_POINTER) {
            SheetVariable *extVar = (SheetVariable *)lm.meta;
            Sheet *extSheet       = extVar->sheet;

            if (sheet != extSheet) {
                newPtr = (char *)-1;
            }
        } else if (lm.type == LINK_FUNCTION) {
            SheetFunction *extFunc = (SheetFunction *)lm.meta;
            Sheet *extSheet        = extFunc->sheet;

            if (sheet != extSheet) {
                newPtr = (char *)-1;
            }
        }

        memcpy(ptr, &newPtr, sizeof(dint));
        ptr += sizeof(dint);
    }

    // ".link"
    memcpy(ptr, ".link", 5);
    ptr += 5;

    // sizeof(.link)
    sizeOfSection = sheet->_insLinkListSize * 2 * sizeof(duint);
    memcpy(ptr, &sizeOfSection, sizeof(duint));
    ptr += sizeof(duint);

    // .link section.
    for (size_t i = 0; i < sheet->_insLinkListSize; i++) {
        InstructionToLink itl = sheet->_insLinkList[i];

        memcpy(ptr, &itl.ins, sizeof(duint));
        ptr += sizeof(duint);

        memcpy(ptr, &itl.link, sizeof(duint));
        ptr += sizeof(duint);
    }

    // ".func"
    memcpy(ptr, ".func", 5);
    ptr += 5;

    // sizeof(.func)
    memcpy(ptr, &funcLen, sizeof(duint));
    ptr += sizeof(duint);

    // .func section.
    func = sheet->functions;

    for (size_t funcIndex = 0; funcIndex < sheet->numFunctions; funcIndex++) {
        // TODO: Error if it's somehow not in the meta list?

        // Find the function's index in the link meta list.
        duint funcMetaIndex = 0;

        for (; funcMetaIndex < sheet->_link.size; funcMetaIndex++) {
            LinkMeta linkMeta = sheet->_link.list[funcMetaIndex];

            if (linkMeta.type == LINK_FUNCTION) {
                // In order to identify our function, we check the names are
                // the same.
                if (strcmp(func->name, linkMeta.name) == 0) {
                    break;
                }
            }
        }

        memcpy(ptr, &funcMetaIndex, sizeof(duint));
        ptr += sizeof(duint);

        *ptr = func->isSubroutine;
        ptr++;

        memcpy(ptr, &(func->numArguments), sizeof(duint));
        ptr += sizeof(duint);

        memcpy(ptr, &(func->numReturns), sizeof(duint));
        ptr += sizeof(duint);

        SheetVariable *arg = func->arguments;

        // TODO: If the default argument is a string, we need to store the
        // string! I couldn't be bothered to do this at the time of writing, so
        // to my future self: Good luck.

        for (size_t argIndex = 0; argIndex < func->numArguments; argIndex++) {
            *ptr = 0; // Is an argument.
            ptr++;

            *ptr = arg->dataType;
            ptr++;

            if (arg->dataType == TYPE_STRING) {
                if (arg->defaultValue.stringValue != NULL) {
                    size_t defaultSize =
                        strlen(arg->defaultValue.stringValue) + 1;
                    memcpy(ptr, arg->defaultValue.stringValue, defaultSize);
                    ptr += defaultSize;
                } else {
                    *ptr = 0;
                    ptr++;
                }
            } else {
                memcpy(ptr, &(arg->defaultValue.integerValue), sizeof(dint));
                ptr += sizeof(dint);
            }

            if (arg->name == NULL) {
                *ptr = 0;
                ptr++;
            } else {
                size_t sizeOfNamePlusNull = strlen(arg->name) + 1;

                memcpy(ptr, arg->name, sizeOfNamePlusNull);
                ptr += sizeOfNamePlusNull;
            }

            arg++;
        }

        SheetVariable *ret = func->returns;

        for (size_t retIndex = 0; retIndex < func->numReturns; retIndex++) {
            *ptr = 1; // Is a return.
            ptr++;

            *ptr = ret->dataType;
            ptr++;

            // memcpy(ptr, &(ret->defaultValue.integerValue), sizeof(dint));
            // ptr += sizeof(dint);

            if (ret->name == NULL) {
                *ptr = 0;
                ptr++;
            } else {
                size_t sizeOfNamePlusNull = strlen(ret->name) + 1;

                memcpy(ptr, ret->name, sizeOfNamePlusNull);
                ptr += sizeOfNamePlusNull;
            }

            arg++;
        }

        func++;
    }

    // ".var"
    memcpy(ptr, ".var", 4);
    ptr += 4;

    // sizeof(.var)
    memcpy(ptr, &varLen, sizeof(duint));
    ptr += sizeof(duint);

    // .var section.
    var = sheet->variables;

    for (size_t varIndex = 0; varIndex < sheet->numVariables; varIndex++) {
        // Find the variable's entry in the LinkMeta array.
        // TODO: Error if it cannot be found.
        size_t metaIndex = 0;

        for (; metaIndex < sheet->_link.size; metaIndex++) {
            LinkMeta lm = sheet->_link.list[metaIndex];

            if ((lm.type == LINK_VARIABLE ||
                 lm.type == LINK_VARIABLE_POINTER) &&
                strcmp(var->name, lm.name) == 0) {
                break;
            }
        }

        memcpy(ptr, &metaIndex, sizeof(duint));
        ptr += sizeof(duint);

        // If the variable is a string, we also need to include the index to the
        // metadata containing the location of the default string value.
        if (var->dataType == TYPE_STRING) {
            size_t defaultStringIndex = 0;

            for (; defaultStringIndex < sheet->_link.size;
                 defaultStringIndex++) {
                LinkMeta lm = sheet->_link.list[defaultStringIndex];

                if (lm.type == LINK_VARIABLE_STRING_DEFAULT_VALUE &&
                    strcmp(var->name, lm.name) == 0) {
                    break;
                }
            }

            memcpy(ptr, &defaultStringIndex, sizeof(duint));
        } else {
            size_t tmp = (size_t)-1;
            memcpy(ptr, &tmp, sizeof(duint));
        }

        ptr += sizeof(duint);

        // The data type of the variable.
        *ptr = var->dataType;
        ptr++;

        var++;
    }

    // ".incl"
    memcpy(ptr, ".incl", 5);
    ptr += 5;

    // sizeof(.incl)
    memcpy(ptr, &inclLen, sizeof(duint));
    ptr += sizeof(duint);

    // .incl section.

    includePtr = sheet->includes;

    for (size_t inclIndex = 0; inclIndex < sheet->numIncludes; inclIndex++) {
        Sheet *include          = *includePtr;
        const char *includePath = include->filePath;

        if (include->includePath != NULL)
            includePath = include->includePath;

        size_t strLenPlusNull = strlen(includePath) + 1;
        memcpy(ptr, includePath, strLenPlusNull);
        ptr += strLenPlusNull;

        includePtr++;
    }

    // ".c"
    memcpy(ptr, ".c", 2);
    ptr += 2;

    // sizeof(.c)
    memcpy(ptr, &cLen, sizeof(duint));
    ptr += sizeof(duint);

    // .c section.
    for (size_t metaIndex = 0; metaIndex < sheet->_link.size; metaIndex++) {
        LinkMeta meta = sheet->_link.list[metaIndex];

        if (meta.type == LINK_CFUNCTION) {
            CFunction *cFunc = (CFunction *)meta.meta;

            size_t nameLenPlusNull = strlen(cFunc->name) + 1;
            memcpy(ptr, cFunc->name, nameLenPlusNull);
            ptr += nameLenPlusNull;

            DType none = TYPE_NONE;

            // Since we are writing the arrays like they exist in the CFunction,
            // TYPE_NONE terminated, we might as well copy the arrays directly,
            // and then add the TYPE_NONEs afterward.
            size_t inputLen = cFunc->numInputs * sizeof(DType);
            memcpy(ptr, cFunc->inputs, inputLen);
            ptr += inputLen;

            memcpy(ptr, &none, sizeof(DType));
            ptr += sizeof(DType);

            size_t outputLen = cFunc->numOutputs * sizeof(DType);
            memcpy(ptr, cFunc->outputs, outputLen);
            ptr += outputLen;

            memcpy(ptr, &none, sizeof(DType));
            ptr += sizeof(DType);
        }
    }

    // '.'
    *ptr = '.';
    ptr++;

    // \0
    *ptr = 0;

    return (const char *)out;
}

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
Sheet *d_asm_load_object(const char *obj, size_t size, const char *filePath) {
    Sheet *out       = d_sheet_create(filePath);
    out->_isCompiled = true;

    // TODO: Account for edianness in the instructions, and also for variables
    // in the data section.

    // A pointer to parse through the content.
    char *ptr = (char *)obj;

    // We need to check that there is a 'D' character at the start.
    if (*ptr != 'D') {
        printf("%s cannot be loaded: object file is not a valid object file.\n",
               filePath);
        out->hasErrors = true;
        return out;
    }
    ptr++;

// We need to check if sizeof(dint) is the same as it is in the object.
#ifdef DECISION_32
    if (strncmp(ptr, "32", 2) != 0) {
        printf("%s cannot be loaded: object file is not 32-bit.\n", filePath);
        out->hasErrors = true;
        return out;
    }
#else
    if (strncmp(ptr, "64", 2) != 0) {
        printf("%s cannot be loaded: object file is not 64-bit.\n", filePath);
        out->hasErrors = true;
        return out;
    }
#endif // DECISION_32
    ptr += 2;

    // END OF METADATA
    // Now, we check for each section.
    duint sizeOfSection;
    while ((size_t)(ptr - obj) < size) {
        if (strncmp(ptr, ".text", 5) == 0) {
            ptr += 5;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            // Extract the machine code.
            out->_text = (char *)d_malloc(sizeOfSection);
            memcpy(out->_text, ptr, sizeOfSection);

            // Make sure we get the size correct, and move on.
            out->_textSize = (size_t)(sizeOfSection);
            ptr += sizeOfSection;
        } else if (strncmp(ptr, ".main", 5) == 0) {
            ptr += 5;
            out->_main = (size_t)(*((duint *)ptr));
            ptr += sizeof(duint);
        } else if (strncmp(ptr, ".data", 5) == 0) {
            ptr += 5;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            // Extract the data.
            out->_data = (char *)d_malloc(sizeOfSection);
            memcpy(out->_data, ptr, sizeOfSection);

            // Make sure we get the size correct, and move on.
            out->_dataSize = (size_t)(sizeOfSection);
            ptr += sizeOfSection;

        } else if (strncmp(ptr, ".lmeta", 6) == 0) {
            ptr += 6;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            char *startOfData = ptr;

            // Extract the data.
            out->_link = d_link_new_meta_list();
            while ((size_t)(ptr - startOfData) < sizeOfSection) {
                LinkMeta lm;

                lm.type = *ptr;
                ptr++;

                size_t strLen = strlen(ptr);
                char *name    = (char *)d_malloc(strLen + 1);
                memcpy(name, ptr, strLen + 1);
                lm.name = (const char *)name;
                ptr += strLen + 1;

                lm._ptr = (char *)(*((dint *)ptr));
                ptr += sizeof(dint);

                // If the metadata isn't in our sheet, then we don't know where
                // it is at all. This will need to be found out at link time.
                if (lm._ptr == (char *)-1)
                    lm.meta = (void *)-1;

                d_link_meta_list_push(&out->_link, lm);
            }
        } else if (strncmp(ptr, ".link", 5) == 0) {
            ptr += 5;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            size_t numElements = sizeOfSection / (2 * sizeof(duint));

            // Extract the data.
            out->_insLinkList = (InstructionToLink *)d_malloc(
                numElements * sizeof(InstructionToLink));
            out->_insLinkListSize = numElements;
            for (size_t i = 0; i < numElements; i++) {
                InstructionToLink itl;

                itl.ins = (size_t)(*((duint *)ptr));
                ptr += sizeof(duint);

                itl.link = (size_t)(*((duint *)ptr));
                ptr += sizeof(duint);

                out->_insLinkList[i] = itl;
            }
        } else if (strncmp(ptr, ".func", 5) == 0) {
            // TODO: Make sure this sections runs after the .lmeta section, as
            // we need to make sure we can access the array!

            ptr += 5;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            char *startOfData = ptr;

            // We need to make sure that the entries in the LinkMetaList of the
            // sheet point to these SheetFunctions we are about to make. But
            // we can only link them once we've created the array of functions
            // fully, as reallocing *may* move the address of the array.
            size_t *funcMetaIndexList    = NULL;
            size_t funcMetaIndexListSize = 0;

            while ((size_t)(ptr - startOfData) < sizeOfSection) {
                // Extract the data for this function.
                size_t metaListIndex = (size_t)(*((duint *)ptr));
                ptr += sizeof(duint);

                bool isSubroutine = *ptr;
                ptr++;

                size_t numArguments = (size_t)(*((duint *)ptr));
                ptr += sizeof(duint);

                size_t numReturns = (size_t)(*((duint *)ptr));
                ptr += sizeof(duint);

                // Get the name of the function from the link meta array.
                // TODO: Check the entry is a function.
                const char *funcName = out->_link.list[metaListIndex].name;

                // Create the function inside the sheet.
                d_sheet_create_function(out, funcName, isSubroutine);

                // Edit the LinkMeta entry's metadata pointer to be the new
                // SheetFunction struct in the sheet array. Note that the
                // functions are appended to the array.
                SheetFunction *metaPtr = out->functions;
                metaPtr += (out->numFunctions - 1);

                out->_link.list[metaListIndex].meta = (void *)metaPtr;

                // TODO: Implement sanity checks to check that we got the right
                // amount of arguments and returns.
                size_t numCurrentSockets = 0;

                while (numCurrentSockets < numArguments + numReturns) {
                    // Extract the data for this socket.
                    bool isReturn = *ptr;
                    ptr++;

                    DType dataType = *ptr;
                    ptr++;

                    LexData defaultValue;

                    if (isReturn) {
                        defaultValue.integerValue = 0;
                    } else {
                        if (dataType == TYPE_STRING) {
                            if (*ptr != 0) {
                                size_t defaultLen = strlen(ptr);
                                char *defaultStr =
                                    (char *)d_malloc(defaultLen + 1);
                                memcpy(defaultStr, ptr, defaultLen + 1);

                                // TODO: Ensure this gets freed.

                                defaultValue.stringValue = defaultStr;

                                ptr += defaultLen + 1;
                            } else {
                                defaultValue.stringValue = NULL;
                                ptr++;
                            }
                        } else {
                            defaultValue.integerValue = *((dint *)ptr);
                            ptr += sizeof(dint);
                        }
                    }

                    char *socketName = 0;

                    if (*ptr != 0) {
                        size_t nameLen = strlen(ptr);
                        socketName     = (char *)d_malloc(nameLen + 1);
                        memcpy(socketName, ptr, nameLen + 1);

                        ptr += nameLen;
                    }

                    ptr++;

                    // Add the socket to the function.
                    if (isReturn)
                        d_sheet_function_add_return(out, funcName, socketName,
                                                    dataType);
                    else
                        d_sheet_function_add_argument(out, funcName, socketName,
                                                      dataType, defaultValue);

                    numCurrentSockets++;
                }

                // Add the LinkMetaList index to the dynamic array we are
                // creating.
                funcMetaIndexListSize++;

                if (funcMetaIndexListSize == 1) {
                    funcMetaIndexList = (size_t *)d_malloc(sizeof(size_t));
                } else {
                    funcMetaIndexList = (size_t *)d_realloc(
                        funcMetaIndexList,
                        funcMetaIndexListSize * sizeof(size_t));
                }

                funcMetaIndexList[funcMetaIndexListSize - 1] = metaListIndex;
            }

            // Now the function array has been fully created, we can now safely
            // point to the functions.
            for (size_t i = 0; i < funcMetaIndexListSize; i++) {
                size_t varMetaIndex = funcMetaIndexList[i];

                // NOTE: This assumes there were no functions in the sheet
                // beforehand.
                out->_link.list[varMetaIndex].meta =
                    (void *)(out->functions + i);
            }

            free(funcMetaIndexList);
        } else if (strncmp(ptr, ".var", 4) == 0) {
            // TODO: Make sure this sections runs after the .lmeta section, as
            // we need to make sure we can access the array!

            ptr += 4;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            char *startOfData = ptr;

            // We need to make sure that the entries in the LinkMetaList of the
            // sheet point to these SheetVariables we are about to make. But
            // we can only link them once we've created the array of variables
            // fully, as reallocing *may* move the address of the array.
            size_t *varMetaIndexList    = NULL;
            size_t varMetaIndexListSize = 0;

            while ((size_t)(ptr - startOfData) < sizeOfSection) {
                // Extract the data.
                size_t varMetaIndex = *((duint *)ptr);
                ptr += sizeof(duint);

                // TODO: Make sure this works for strings!
                size_t stringDefaultIndex = *((duint *)ptr);
                ptr += sizeof(duint);

                DType varType = *ptr;
                ptr++;

                // Build the SheetVariable structure.
                SheetVariable var;

                // Copy the variable name.
                LinkMeta varMeta          = out->_link.list[varMetaIndex];
                size_t nameLengthPlusNull = strlen(varMeta.name) + 1;
                char *varName = (char *)d_malloc(nameLengthPlusNull);
                memcpy(varName, varMeta.name, nameLengthPlusNull);

                var.name     = (const char *)varName;
                var.dataType = varType;

                // We can get the default value of the variable from the data
                // section.
                size_t dataOffset = (size_t)varMeta._ptr;
                if (varType == TYPE_STRING) {
                    LinkMeta strPtrMeta = out->_link.list[stringDefaultIndex];
                    dataOffset          = (size_t)strPtrMeta._ptr;

                    // Since it is a string default value, we need to copy it.
                    char *defaultStr     = out->_data + dataOffset;
                    size_t defaultStrLen = strlen(defaultStr);
                    char *cpyDefaultStr  = (char *)d_malloc(defaultStrLen + 1);
                    memcpy(cpyDefaultStr, defaultStr, defaultStrLen + 1);

                    var.defaultValue.stringValue = cpyDefaultStr;
                } else if (varType == TYPE_BOOL) {
                    var.defaultValue.booleanValue =
                        *((char *)(out->_data + dataOffset));
                } else {
                    var.defaultValue.integerValue =
                        *((dint *)(out->_data + dataOffset));
                }

                // var.defaultValue.integerValue = *((dint*)(out->_data +
                // dataOffset));

                // Add the variable to the sheet.
                d_sheet_add_variable(out, var);

                // Add the LinkMetaList index to the dynamic array we are
                // creating.
                varMetaIndexListSize++;

                if (varMetaIndexListSize == 1) {
                    varMetaIndexList = (size_t *)d_malloc(sizeof(size_t));
                } else {
                    varMetaIndexList = (size_t *)d_realloc(
                        varMetaIndexList,
                        varMetaIndexListSize * sizeof(size_t));
                }

                varMetaIndexList[varMetaIndexListSize - 1] = varMetaIndex;
            }

            // Now the variable array has been fully created, we can now safely
            // point to the variables.
            for (size_t i = 0; i < varMetaIndexListSize; i++) {
                size_t varMetaIndex = varMetaIndexList[i];

                // NOTE: This assumes there were no variables in the sheet
                // beforehand.
                out->_link.list[varMetaIndex].meta =
                    (void *)(out->variables + i);
            }

            free(varMetaIndexList);
        } else if (strncmp(ptr, ".incl", 5) == 0) {
            ptr += 5;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            char *startOfData = ptr;

            char *includeFilePath;

            while ((size_t)(ptr - startOfData) < sizeOfSection) {
                size_t filePathLen = strlen(ptr);

                includeFilePath = (char *)d_malloc(filePathLen + 1);
                memcpy(includeFilePath, ptr, filePathLen + 1);

                Sheet *includedSheet =
                    d_sheet_add_include_from_path(out, includeFilePath);

                if (includedSheet->hasErrors) {
                    ERROR_COMPILER(out->filePath, 0, true,
                                   "Included sheet %s produced errors",
                                   includedSheet->filePath);
                }

                // When the file path enters the sheet, it is copied, so we can
                // safely free the file path here.
                free(includeFilePath);

                ptr += filePathLen + 1;
            }
        } else if (strncmp(ptr, ".c", 2) == 0) {
            // TODO: Make sure this sections runs after the .lmeta section, as
            // we need to make sure we can access the array!

            ptr += 2;
            sizeOfSection = *((duint *)ptr);
            ptr += sizeof(duint);

            char *startOfData = ptr;

            while ((size_t)(ptr - startOfData) < sizeOfSection) {
                // The name of the function.
                char *funcName = ptr;
                ptr += strlen(funcName) + 1;

                // The input array.
                DType *inputs = (DType *)ptr;

                while (*((DType *)ptr) != TYPE_NONE) {
                    ptr += sizeof(DType);
                }

                ptr += sizeof(DType); // Skip over the TYPE_NONE.

                // The output array.
                DType *outputs = (DType *)ptr;

                while (*((DType *)ptr) != TYPE_NONE) {
                    ptr += sizeof(DType);
                }

                ptr += sizeof(DType); // Skip over the TYPE_NONE.

                // We have all the information we need - now we need to make
                // sure the C function the object described exists.
                bool cFuncFound  = false;
                CFunction *cFunc = NULL;

                size_t numCFunctions = d_get_num_c_functions();
                for (size_t cIndex = 0; cIndex < numCFunctions; cIndex++) {
                    cFunc = (CFunction *)d_get_c_functions() + cIndex;

                    // Does it have the same name?
                    if (strcmp(cFunc->name, funcName) == 0) {

                        // Does it have the same number of inputs and outputs?
                        size_t numInputs = 0, numOutputs = 0;
                        DType *typePtr;

                        for (typePtr = inputs; *typePtr != TYPE_NONE;
                             typePtr++) {
                            numInputs++;
                        }

                        for (typePtr = outputs; *typePtr != TYPE_NONE;
                             typePtr++) {
                            numOutputs++;
                        }

                        if ((cFunc->numInputs == numInputs) &&
                            (cFunc->numOutputs == numOutputs)) {

                            // Are the input and output types the same?
                            bool sameInputs = true, sameOutputs = true;

                            typePtr           = inputs;
                            DType *typePtrDef = (DType *)cFunc->inputs;
                            for (size_t i = 0; i < numInputs; i++) {
                                if (*typePtr != *typePtrDef) {
                                    sameInputs = false;
                                    break;
                                }
                                typePtr++;
                                typePtrDef++;
                            }

                            typePtr    = outputs;
                            typePtrDef = (DType *)cFunc->outputs;
                            for (size_t i = 0; i < numOutputs; i++) {
                                if (*typePtr != *typePtrDef) {
                                    sameOutputs = false;
                                    break;
                                }
                                typePtr++;
                                typePtrDef++;
                            }

                            if (sameInputs && sameOutputs) {
                                // Then we've found the C function!
                                cFuncFound = true;
                                break;
                            }
                        }
                    }
                }

                if (cFuncFound) {
                    // Great! We just need to edit the LinkMeta entry to point
                    // to the CFunction.
                    for (size_t linkMetaIndex = 0;
                         linkMetaIndex < out->_link.size; linkMetaIndex++) {
                        LinkMeta *meta = out->_link.list + linkMetaIndex;

                        if (meta->type == LINK_CFUNCTION) {
                            if (strcmp(meta->name, funcName) == 0) {
                                meta->meta = cFunc;
                            }
                        }
                    }
                } else {
                    ERROR_COMPILER(out->filePath, 0, true,
                                   "Sheet requires a C function %s to work",
                                   funcName);
                }
            }
        } else if (strncmp(ptr, ".\0", 2) == 0) {
            // We've reached the end, stop parsing.
            break;
        } else {
            // We don't recognise this bit, keep going until we find a bit we
            // do recognise.
            ptr++;
        }
    }

    return out;
}
