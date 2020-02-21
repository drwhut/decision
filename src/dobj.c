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

#include "dcfunc.h"
#include "derror.h"
#include "dmalloc.h"
#include "dsheet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * \struct _objReader
 * \brief A struct used to read the contents of an object file.
 *
 * \typedef struct _objReader ObjectReader
 */
typedef struct _objReader {

} ObjectReader;

/**
 * \struct _objWriter
 * \brief A struct used to write the contents of an object file.
 *
 * \typedef struct _objWriter ObjectWriter
 */
typedef struct _objWriter {
    char *obj;  ///< The object file contents.
    size_t len; ///< The length of the object string in bytes.
} ObjectWriter;

/*
=== READER FUNCTIONS ======================================
*/

/*
=== WRITER FUNCTIONS ======================================
*/

/**
 * \fn static void writer_alloc_end(ObjectWriter *writer, size_t numBytes)
 * \brief Add allocated space to the end of a writer.
 *
 * \param writer The writer to allocate space for.
 * \param numBytes The number of bytes to add to the end of the writer.
 */
static void writer_alloc_end(ObjectWriter *writer, size_t numBytes) {
    size_t newLen = writer->len + numBytes;

    if (writer->obj == NULL) {
        writer->obj = d_malloc(newLen);
    } else {
        writer->obj = d_realloc(writer->obj, newLen);
    }

    writer->len = newLen;
}

/**
 * \fn static void write_byte(ObjectWriter *writer, char byte)
 * \brief Write a byte onto the end of an object writer.
 *
 * \param writer The writer to write the byte to.
 * \param byte The byte to write.
 */
static void write_byte(ObjectWriter *writer, char byte) {
    size_t start = writer->len;
    writer_alloc_end(writer, 1);
    *(writer->obj + start) = byte;
}

/**
 * \fn static void write_string_n(ObjectWriter *writer, const char *str,
 *                                size_t n)
 * \brief Write a length-n string onto the end of an object writer.
 *
 * \param writer The writer to write the string to.
 * \param str The string to write. If it is NULL, nothing is written.
 * \param n The number of characters to write from the string.
 */
static void write_string_n(ObjectWriter *writer, const char *str, size_t n) {
    if (str == NULL) {
        return;
    }

    size_t start = writer->len;
    writer_alloc_end(writer, n);
    memcpy(writer->obj + start, str, n);
}

/**
 * \fn static void write_string(ObjectWriter *writer, const char *str)
 * \brief Write a string onto the end of an object writer.
 *
 * **NOTE:** It includes the \0 character at the end!
 *
 * \param writer The writer to write the string to.
 * \param str The string to write. If it is NULL, a `\0` is written.
 */
static void write_string(ObjectWriter *writer, const char *str) {
    size_t lenStr = 1;
    if (str != NULL) {
        lenStr += strlen(str);
    } else {
        str = "";
    }

    write_string_n(writer, str, lenStr);
}

/**
 * \fn static void write_uinteger(ObjectWriter *writer, duint uinteger)
 * \brief Write an unsigned integer onto the end of an object writer.
 *
 * \param writer The writer to write the integer to.
 * \param uinteger The integer to write.
 */
static void write_uinteger(ObjectWriter *writer, duint uinteger) {
    size_t start = writer->len;
    writer_alloc_end(writer, sizeof(duint));
    *(duint *)(writer->obj + start) = uinteger;
}

/**
 * \fn static size_t get_socket_meta_length(SocketMeta meta)
 * \brief Get the amount of space required to store a SocketMeta.
 *
 * \return The number of bytes needed to store the socket meta.
 *
 * \param meta The socket metadata to query.
 */
/*
static size_t get_socket_meta_length(SocketMeta meta) {
    // 3 = Type + 2 \0s.
    size_t size = 3;

    if (meta.name != NULL) {
        size += strlen(meta.name);
    }

    if (meta.description != NULL) {
        size += strlen(meta.description);
    }

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
*/

/**
 * \fn static void write_socket_meta(char **ptr, SocketMeta meta)
 * \brief Write some socket metadata into a string.
 *
 * \param ptr The pointer being used to write to the string.
 * \param meta The socket metadata to write to the string.
 */
/*
static void write_socket_meta(char **ptr, const SocketMeta meta) {
    char *str = *ptr;

    if (meta.name != NULL) {
        size_t nameSize = strlen(meta.name) + 1;
        memcpy(str, meta.name, nameSize);
        str += nameSize;
    }
    else {
        *str = 0;
        str++;
    }

    if (meta.description != NULL) {
        size_t descriptionSize = strlen(meta.description) + 1;
        memcpy(str, meta.description, descriptionSize);
        str += descriptionSize;
    }
    else {
        *str = 0;
        str++;
    }

    *str = meta.type;
    str++;

    if (meta.type == TYPE_STRING) {
        if (meta.defaultValue.stringValue != NULL) {
            size_t defaultSize = strlen(meta.defaultValue.stringValue) + 1;
            memcpy(str, meta.defaultValue.stringValue, defaultSize);
            str += defaultSize;
        } else {
            *str = 0;
            str++;
        }
    } else {
        memcpy(str, &(meta.defaultValue.integerValue), sizeof(dint));
        str += sizeof(dint);
    }

    *ptr = str;
}
*/

/**
 * \fn static SocketMeta read_socket_meta(char **ptr)
 * \brief Read in from a string socket metadata.
 *
 * \return The socket metadata.
 *
 * \param ptr The pointer being used to read the string.
 */
/*
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
*/

/**
 * \fn static size_t get_node_definition_length(NodeDefinition *definition)
 * \brief Get the amount of space required to store a NodeDefinition.
 *
 * \return The number of bytes needed to store the node definition.
 *
 * \param definition The node definition to query.
 */
/*
static size_t get_node_definition_length(const NodeDefinition *definition) {
    // 2 = 2 \0s.
    size_t size = 2 * sizeof(duint) + 2;

    if (definition->name != NULL) {
        size += strlen(definition->name);
    }

    if (definition->description != NULL) {
        size += strlen(definition->description);
    }

    for (size_t i = 0; i < definition->numSockets; i++) {
        SocketMeta meta = definition->sockets[i];
        size += get_socket_meta_length(meta);
    }

    return size;
}
*/

/**
 * \fn static void write_node_definition(char **ptr, NodeDefinition *definition)
 * \brief Write a node definition into a string.
 *
 * \param ptr The pointer being used to write the string.
 * \param definition The node definition to write into the string.
 */
/*
static void write_node_definition(char **ptr,
                                  const NodeDefinition *definition) {
    char *str = *ptr;

    if (definition->name != NULL) {
        size_t nameSize = strlen(definition->name) + 1;
        memcpy(str, definition->name, nameSize);
        str += nameSize;
    }
    else {
        *str = 0;
        str++;
    }

    if (definition->description != NULL) {
        size_t descriptionSize = strlen(definition->description) + 1;
        memcpy(str, definition->description, descriptionSize);
        str += descriptionSize;
    }
    else {
        *str = 0;
        str++;
    }

    memcpy(str, &(definition->startOutputIndex), sizeof(duint));
    str += sizeof(duint);

    memcpy(str, &(definition->numSockets), sizeof(duint));
    str += sizeof(duint);

    for (size_t i = 0; i < definition->numSockets; i++) {
        SocketMeta meta = definition->sockets[i];
        write_socket_meta(&str, meta);
    }

    *ptr = str;
}
*/

/**
 * \fn static NodeDefinition read_node_definition(char **ptr)
 * \brief Read a node definition from a string.
 *
 * \return The node definition.
 *
 * \param ptr The pointer being used to read the string.
 */
/*
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
*/

/*
=== GLOBAL FUNCTIONS ======================================
*/

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
    // Create the ObjectWriter struct.
    ObjectWriter writer = {NULL, 0};

#ifdef DECISION_32
    write_string_n(&writer, "D32", 3);
#else
    write_string_n(&writer, "D64", 3);
#endif // DECISION_32

    // .text
    // If no bytecode has been compiled, don't bother putting anything here.
    if (sheet->_textSize > 0) {
        write_string_n(&writer, ".text", 5);
        write_uinteger(&writer, sheet->_textSize);
        write_string_n(&writer, sheet->_text, sheet->_textSize);
    }

    // .main
    // If there is no Start function, don't bother putting anything here.
    if (sheet->_main > 0) {
        write_string_n(&writer, ".main", 5);
        write_uinteger(&writer, sheet->_main);
    }

    // .data
    // If there is nothing in the data section, don't bother putting anything
    // here.
    if (sheet->_dataSize > 0) {
        write_string_n(&writer, ".data", 5);
        write_uinteger(&writer, sheet->_dataSize);
        write_string_n(&writer, sheet->_data, sheet->_dataSize);
    }

    // .lmeta
    // If there is nothing linkable, don't bother putting anything here.
    if (sheet->_link.size > 0) {
        write_string_n(&writer, ".lmeta", 6);
        write_uinteger(&writer, sheet->_link.size);

        for (size_t i = 0; i < sheet->_link.size; i++) {
            LinkMeta lm = sheet->_link.list[i];

            write_byte(&writer, lm.type);
            write_string(&writer, lm.name);

            // If the object is in another sheet, we can't store the pointer as
            // it is now! We will need to re-calculate it when we run the object
            // after it is built.
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

            write_uinteger(&writer, (duint)newPtr);
        }
    }

    // .link
    // If there are no instructions that need to be linked, don't bother putting
    // anything here.
    if (sheet->_insLinkListSize > 0) {
        write_string_n(&writer, ".link", 5);
        write_uinteger(&writer, sheet->_insLinkListSize);

        for (size_t i = 0; i < sheet->_insLinkListSize; i++) {
            InstructionToLink itl = sheet->_insLinkList[i];

            write_uinteger(&writer, itl.ins);
            write_uinteger(&writer, itl.link);
        }
    }

    // .func
    // If there are no functions, don't bother putting anything here.
    if (sheet->numFunctions > 0) {
        write_string_n(&writer, ".func", 5);
        write_uinteger(&writer, sheet->numFunctions);

        for (size_t i = 0; i < sheet->numFunctions; i++) {
            const NodeDefinition funcDef =
                sheet->functions[i].functionDefinition;
            write_definition(&writer, funcDef);
        }
    }

    // .var
    // If there are no variables, don't bother putting anything here.
    if (sheet->numVariables > 0) {
        write_string_n(&writer, ".var", 4);
        write_uinteger(&writer, sheet->numVariables);

        for (size_t i = 0; i < sheet->numVariables; i++) {
            const SocketMeta varMeta = sheet->variables[i].variableMeta;
            write_socket(&writer, varMeta);
        }
    }

    // .incl
    // If there are no included sheets, don't bother putting anything here.
    if (sheet->numIncludes > 0) {
        write_string_n(&writer, ".incl", 5);
        write_uinteger(&writer, sheet->numIncludes);

        for (size_t i = 0; i < sheet->numIncludes; i++) {
            Sheet *include   = sheet->includes[i];
            const char *path = include->filePath;

            if (include->includePath != NULL) {
                path = include->includePath;
            }

            write_string(&writer, path);
        }
    }

    // .c
    // If the sheet doesn't use any C functions, don't bother putting anything
    // here.
    size_t numCFunctionsUsed = 0;

    for (size_t i = 0; i < sheet->_link.size; i++) {
        LinkMeta meta = sheet->_link.list[i];

        if (meta.type == LINK_CFUNCTION) {
            numCFunctionsUsed++;
        }
    }

    if (numCFunctionsUsed > 0) {
        write_string_n(&writer, ".c", 2);
        write_uinteger(&writer, numCFunctionsUsed);

        for (size_t i = 0; i < sheet->_link.size; i++) {
            LinkMeta meta = sheet->_link.list[i];

            if (meta.type == LINK_CFUNCTION) {
                CFunction *cFunc = (CFunction *)meta.meta;
                const NodeDefinition cDef = cFunc->definition;

                write_definition(&writer, cDef);
            }
        }
    }

    *size = writer.len;
    return (const char *)writer.obj;
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