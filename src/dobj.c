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

#include "dobj.h"

#include "derror.h"
#include "dmalloc.h"
#include "dsheet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * \fn static size_t get_socket_meta_length(SocketMeta meta)
 * \brief Get the amount of space required to store a SocketMeta.
 *
 * \return The number of bytes needed to store the socket meta.
 *
 * \param meta The socket metadata to query.
 */
static size_t get_socket_meta_length(SocketMeta meta) {
    // 3 = Type + 2 \0s.
    size_t size = strlen(meta.name) + strlen(meta.description) + 3;

    // The size could still vary depending on the default value.
    if (meta.type == TYPE_STRING) {
        if (meta.defaultValue.stringValue != NULL) {
            size += strlen(meta.defaultValue.stringValue) + 1;
        } else {
            size += 1; // 0 to say no default value.
        }
    } else {
        size += sizeof(dint);
    }

    return size;
}

/**
 * \fn static void write_socket_meta(char **ptr, SocketMeta meta)
 * \brief Write some socket metadata into a string.
 *
 * \param ptr The pointer being used to write to the string.
 * \param meta The socket metadata to write to the string.
 */
static void write_socket_meta(char **ptr, const SocketMeta meta) {
    char *str = *ptr;

    size_t nameSize = strlen(meta.name) + 1;
    memcpy(str, meta.name, nameSize);
    str += nameSize;

    size_t descriptionSize = strlen(meta.description) + 1;
    memcpy(str, meta.description, descriptionSize);
    str += descriptionSize;

    *str = meta.type;
    str++;

    if (meta.type == TYPE_STRING) {
        if (meta.defaultValue.stringValue != NULL) {
            size_t defaultSize = strlen(meta.defaultValue.stringValue) + 1;
            memcpy(ptr, meta.defaultValue.stringValue, defaultSize);
            ptr += defaultSize;
        } else {
            *ptr = 0;
            ptr++;
        }
    } else {
        memcpy(ptr, &(meta.defaultValue.integerValue), sizeof(dint));
        ptr += sizeof(dint);
    }

    *ptr = str;
}

/**
 * \fn static SocketMeta read_socket_meta(char **ptr)
 * \brief Read in from a string socket metadata.
 *
 * \return The socket metadata.
 *
 * \param ptr The pointer being used to read the string.
 */
static const SocketMeta read_socket_meta(char **ptr) {
    char *str = *ptr;

    size_t nameSize = strlen(str) + 1;
    char *name      = (char *)d_malloc(nameSize);
    memcpy(name, str, nameSize);
    str += nameSize;

    size_t desciptionSize = strlen(str) + 1;
    char *description     = (char *)d_malloc(desciptionSize);
    memcpy(description, str, desciptionSize);
    str += desciptionSize;

    DType type = *str;
    str++;

    LexData defaultValue;

    if (type == TYPE_STRING) {
        size_t defaultSize = strlen(str) + 1;
        char *defaultStr   = (char *)d_malloc(defaultSize);
        memcpy(defaultStr, str, defaultSize);
        str += defaultSize;

        defaultValue.stringValue = defaultStr;
    } else {
        defaultValue.integerValue = *((dint *)str);
    }

    *ptr = str;

    SocketMeta meta;
    meta.name         = name;
    meta.description  = description;
    meta.type         = type;
    meta.defaultValue = defaultValue;

    return meta;
}

/**
 * \fn static size_t get_node_definition_length(NodeDefinition *definition)
 * \brief Get the amount of space required to store a NodeDefinition.
 *
 * \return The number of bytes needed to store the node definition.
 *
 * \param definition The node definition to query.
 */
static size_t get_node_definition_length(const NodeDefinition *definition) {
    // 3 =  2 \0s.
    size_t size = strlen(definition->name) + strlen(definition->description) +
                  2 * sizeof(duint) + 2;

    for (size_t i = 0; i < definition->numSockets; i++) {
        SocketMeta meta = definition->sockets[i];
        size += get_socket_meta_length(meta);
    }

    return size;
}

/**
 * \fn static void write_node_definition(char **ptr, NodeDefinition *definition)
 * \brief Write a node definition into a string.
 *
 * \param ptr The pointer being used to write the string.
 * \param definition The node definition to write into the string.
 */
static void write_node_definition(char **ptr,
                                  const NodeDefinition *definition) {
    char *str = *ptr;

    size_t nameSize = strlen(definition->name) + 1;
    memcpy(str, definition->name, nameSize);
    str += nameSize;

    size_t descriptionSize = strlen(definition->description) + 1;
    memcpy(str, definition->description, descriptionSize);
    str += descriptionSize;

    memcpy(ptr, &(definition->startOutputIndex), sizeof(duint));
    ptr += sizeof(duint);

    memcpy(ptr, &(definition->numSockets), sizeof(duint));
    ptr += sizeof(duint);

    for (size_t i = 0; i < definition->numSockets; i++) {
        SocketMeta meta = definition->sockets[i];
        write_socket_meta(&str, meta);
    }

    *ptr = str;
}

/**
 * \fn static NodeDefinition read_node_definition(char **ptr)
 * \brief Read a node definition from a string.
 *
 * \return The node definition.
 *
 * \param ptr The pointer being used to read the string.
 */
static const NodeDefinition read_node_definition(char **ptr) {
    char *str = *ptr;

    size_t nameSize = strlen(str) + 1;
    char *name      = (char *)d_malloc(nameSize);
    memcpy(name, str, nameSize);
    str += nameSize;

    size_t desciptionSize = strlen(str) + 1;
    char *description     = (char *)d_malloc(desciptionSize);
    memcpy(description, str, desciptionSize);
    str += desciptionSize;

    size_t startOutputIndex = *((duint *)str);
    str += sizeof(duint);

    size_t numSockets = *((duint *)str);
    str += sizeof(duint);

    SocketMeta *list = (SocketMeta *)d_malloc(numSockets * sizeof(SocketMeta));

    for (size_t i = 0; i < numSockets; i++) {
        SocketMeta meta = read_socket_meta(&str);
        memcpy(list + i, &meta, sizeof(SocketMeta));
    }

    *ptr = str;

    NodeDefinition definition             = {NULL, NULL, NULL, 0, 0, false};
    definition.name                       = name;
    definition.description                = description;
    *(SocketMeta **)&(definition.sockets) = list;
    definition.numSockets                 = numSockets;
    definition.startOutputIndex           = startOutputIndex;

    return definition;
}

/**
 * \fn const char *d_obj_generate(Sheet *sheet, size_t *size)
 * \brief Given a sheet has been compiled, create the contents of the sheet's
 * object file.
 *
 * This function is essentially the reverse of `d_obj_load`.
 *
 * \return A malloc'd string of the contents of the future object file.
 *
 * \param sheet The sheet to use to create the object.
 * \param size A pointer to a size that is overwritten with the size of the
 * generated string.
 */
const char *d_obj_generate(Sheet *sheet, size_t *size) {
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

    for (size_t funcIndex = 0; funcIndex < sheet->numFunctions; funcIndex++) {
        SheetFunction *func = sheet->functions + funcIndex;
        funcLen += get_node_definition_length(&(func->functionDefinition));
    }

    len += funcLen;

    // The .var section can highly vary as well.
    size_t varLen = 0;

    for (size_t varIndex = 0; varIndex < sheet->numVariables; varIndex++) {
        SheetVariable *var = sheet->variables + varIndex;
        varLen += get_socket_meta_length(var->variableMeta);
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
    /*
    for (size_t metaIndex = 0; metaIndex < sheet->_link.size; metaIndex++) {
        LinkMeta meta = sheet->_link.list[metaIndex];

        if (meta.type == LINK_CFUNCTION) {
            CFunction *cFunc = (CFunction *)meta.meta;
            const NodeDefinition cDef = cFunc->definition;

            // The name of the C function, along with it's inputs and outputs,
            // which are TYPE_NONE terminated.
            cLen += strlen(cDef.name) +
            cLen += strlen(cFunc->name) + 1 +
                    (cFunc->numInputs + cFunc->numOutputs + 2) * sizeof(DType);
        }
    }
    */

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
    for (size_t funcIndex = 0; funcIndex < sheet->numFunctions; funcIndex++) {
        // TODO: Error if it's somehow not in the meta list?
        SheetFunction *func = sheet->functions + funcIndex;

        // Find the function's index in the link meta list.
        duint funcMetaIndex = 0;

        for (; funcMetaIndex < sheet->_link.size; funcMetaIndex++) {
            LinkMeta linkMeta = sheet->_link.list[funcMetaIndex];

            if (linkMeta.type == LINK_FUNCTION) {
                // In order to identify our function, we check the names are
                // the same.
                if (strcmp(func->functionDefinition.name, linkMeta.name) == 0) {
                    break;
                }
            }
        }

        memcpy(ptr, &funcMetaIndex, sizeof(duint));
        ptr += sizeof(duint);

        // TODO: The name of the function will be written again with this call.
        // Add a way to stop writing the name, as it's already in the link meta
        // list?
        write_node_definition(&ptr, &(func->functionDefinition));

        func++;
    }

    // ".var"
    memcpy(ptr, ".var", 4);
    ptr += 4;

    // sizeof(.var)
    memcpy(ptr, &varLen, sizeof(duint));
    ptr += sizeof(duint);

    // .var section.
    for (size_t varIndex = 0; varIndex < sheet->numVariables; varIndex++) {
        // Find the variable's entry in the LinkMeta array.
        // TODO: Error if it cannot be found.
        SheetVariable *var = sheet->variables + varIndex;
        size_t metaIndex   = 0;

        for (; metaIndex < sheet->_link.size; metaIndex++) {
            LinkMeta lm = sheet->_link.list[metaIndex];

            if ((lm.type == LINK_VARIABLE ||
                 lm.type == LINK_VARIABLE_POINTER) &&
                strcmp(var->variableMeta.name, lm.name) == 0) {
                break;
            }
        }

        memcpy(ptr, &metaIndex, sizeof(duint));
        ptr += sizeof(duint);

        // TODO: The name of the variable will be written again with this call.
        // Add a way to stop writing the name, as it's already in the link meta
        // list?
        write_socket_meta(&ptr, var->variableMeta);

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
    /*
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
    */

    // '.'
    *ptr = '.';
    ptr++;

    // \0
    *ptr = 0;

    return (const char *)out;
}

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
Sheet *d_obj_load(const char *obj, size_t size, const char *filePath) {
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

                NodeDefinition funcDef = read_node_definition(&ptr);

                // Add the function to the sheet.
                d_sheet_add_function(out, funcDef);

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

                // TODO: The default value is already in the data section!
                SocketMeta varMeta = read_socket_meta(&ptr);

                // Add the variable to the sheet.
                d_sheet_add_variable(out, varMeta);

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
            /*
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
            */
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