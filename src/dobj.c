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
#include "decision.h"
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
    const char *obj;  ///< The object file contents.
    const size_t len; ///< The length of the object file.
    size_t ptr;       ///< The current "pointer".
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

/**
 * \struct _indexList
 * \brief A struct for storing a list of indexes.
 *
 * \typedef struct _indexList IndexList
 */
typedef struct _indexList {
    size_t *indexList;
    size_t length;
} IndexList;

/*
=== READER FUNCTIONS ======================================
*/

/**
 * \fn static bool reader_test_string_n(ObjectReader *reader, const char *str,
 *                                      size_t n)
 * \brief Check if there is a specific length-n string at the current position.
 * If the string is at the current position, move the reader's point to the
 * character after the string.
 *
 * \return If the string of length n is at the current position in the reader.
 *
 * \param reader The reader to query.
 * \param str The string to test for.
 * \param n The number of characters to test.
 */
static bool reader_test_string_n(ObjectReader *reader, const char *str,
                                 size_t n) {
    if (reader->obj == NULL || str == NULL) {
        return false;
    }

    if (reader->ptr + n > reader->len) {
        return false;
    }

    if (strncmp(str, reader->obj + reader->ptr, n) == 0) {
        reader->ptr += n;
        return true;
    }

    return false;
}

/**
 * \fn static char read_byte(ObjectReader *reader)
 * \brief Read a byte from an object reader.
 *
 * \return The byte at the reader's current position.
 *
 * \param reader The reader to read from.
 */
static char read_byte(ObjectReader *reader) {
    char out = *(reader->obj + reader->ptr);
    reader->ptr++;
    return out;
}

/**
 * \fn static char *read_string_n(ObjectReader *reader, size_t n)
 * \brief Read an malloc'd length-n string from an object reader.
 *
 * \return The malloc'd length-n string at the reader's current position.
 *
 * \param reader The reader to read from.
 * \param n The number of bytes to read.
 */
static char *read_string_n(ObjectReader *reader, size_t n) {
    char *out = d_calloc(n, sizeof(char));
    memcpy(out, reader->obj + reader->ptr, n);
    reader->ptr += n;
    return out;
}

/**
 * \fn static char *read_string(ObjectReader *reader)
 * \brief Read a malloc'd string from an object reader. The end of the string
 * is determined by where the next \0 character is.
 *
 * \return The malloc'd string at the reader's current position.
 *
 * \param reader The reader to read from.
 */
static char *read_string(ObjectReader *reader) {
    size_t nameLen = strlen(reader->obj + reader->ptr);
    return read_string_n(reader, nameLen + 1);
}

/**
 * \fn static duint read_uinteger(ObjectReader *reader)
 * \brief Read an unsigned integer from an object reader.
 *
 * \return The unsigned integer at the reader's current position.
 *
 * \param reader The reader to read from.
 */
static duint read_uinteger(ObjectReader *reader) {
    duint out = *(duint *)(reader->obj + reader->ptr);
    reader->ptr += sizeof(duint);
    return out;
}

/**
 * \fn static SocketMeta read_socket_meta(ObjectWriter *writer, bool hasName,
 *                                        bool hasDefault)
 * \brief Read socket metadata from an object reader.
 *
 * \return The socket metadata at the reader's current position.
 *
 * \param reader The reader to read from.
 * \param hasName Should we read the name as well?
 * \param hasDefault Should we read the default value as well?
 */
static SocketMeta read_socket_meta(ObjectReader *reader, bool hasName,
                                   bool hasDefault) {
    SocketMeta out = {NULL, NULL, TYPE_NONE, {0}};

    if (hasName) {
        out.name = read_string(reader);
    }

    out.description = read_string(reader);
    out.type        = read_byte(reader);

    if (hasDefault) {
        if (out.type == TYPE_STRING) {
            out.defaultValue.stringValue = read_string(reader);
        } else {
            out.defaultValue.integerValue = read_uinteger(reader);
        }
    }

    return out;
}

/**
 * \fn static NodeDefinition read_definition(ObjectReader *reader, bool hasName)
 * \brief Read a node definition from an object reader.
 *
 * \return The node definition at the reader's current position.
 *
 * \param reader The reader to read from.
 * \param hasName Should we read the name as well?
 */
static NodeDefinition read_definition(ObjectReader *reader, bool hasName) {
    NodeDefinition def = {NULL, NULL, NULL, 0, 0, false};

    if (hasName) {
        def.name = read_string(reader);
    }

    def.description = read_string(reader);

    def.numSockets       = read_uinteger(reader);
    def.startOutputIndex = read_uinteger(reader);

    def.infiniteInputs = false;

    SocketMeta *sockets = d_calloc(def.numSockets, sizeof(SocketMeta));

    for (size_t i = 0; i < def.numSockets; i++) {
        SocketMeta meta = read_socket_meta(reader, true, true);
        sockets[i]      = meta;
    }

    *(SocketMeta **)(&(def.sockets)) = sockets;

    return def;
}

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
        writer->obj = d_calloc(newLen, sizeof(char));
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
 * \fn static void write_socket_meta(ObjectWriter *writer,
 *                                   const SocketMeta meta, bool writeName,
 *                                   bool writeDefault)
 * \brief Write socket metadata onto the end of an object writer.
 *
 * \param writer The writer to write the socket to.
 * \param meta The socket metadata to write.
 * \param writeName Do we write the name onto the writer?
 * \param writeDefault Do we write the default value onto the writer?
 */
static void write_socket_meta(ObjectWriter *writer, const SocketMeta meta,
                              bool writeName, bool writeDefault) {
    if (writeName) {
        write_string(writer, meta.name);
    }
    write_string(writer, meta.description);
    write_byte(writer, meta.type);

    if (writeDefault) {
        if (meta.type == TYPE_STRING) {
            write_string(writer, meta.defaultValue.stringValue);
        } else {
            write_uinteger(writer, meta.defaultValue.integerValue);
        }
    }
}

/**
 * \fn static void write_definition(ObjectWriter *writer,
 *                                  const NodeDefinition def, bool writeName)
 * \brief Write a node definition onto the end of an object writer.
 *
 * \param writer The writer to wrire the definition to.
 * \param def The definition to write.
 * \param writeName Do we write the name onto the writer?
 */
static void write_definition(ObjectWriter *writer, const NodeDefinition def,
                             bool writeName) {
    if (writeName) {
        write_string(writer, def.name);
    }
    write_string(writer, def.description);

    write_uinteger(writer, def.numSockets);
    write_uinteger(writer, def.startOutputIndex);

    for (size_t i = 0; i < def.numSockets; i++) {
        const SocketMeta meta = def.sockets[i];
        write_socket_meta(writer, meta, true, true);
    }
}

/*
=== GLOBAL FUNCTIONS ======================================
*/

/**
 * \fn static int find_link(LinkMetaList list, const char *name, LinkType type)
 * \brief Find the index of the link metadata with a given name and type.
 *
 * \return The index of the corresponding link metadata in the list. `-1` if
 * it cannot be found.
 *
 * \param list The list to query.
 * \param name The name to query.
 * \param type The type to query.
 */
static int find_link(LinkMetaList list, const char *name, LinkType type) {
    if (list.list == NULL || list.size == 0) {
        return -1;
    }

    for (int i = 0; i < (int)list.size; i++) {
        LinkMeta meta = list.list[i];

        if (strcmp(name, meta.name) == 0 && type == meta.type) {
            return i;
        }
    }

    return -1;
}

/**
 * \fn static void push_index(IndexList *list, size_t index)
 * \brief Add an index to an index list.
 *
 * \param list The list to add the index to.
 * \param index The index to add.
 */
static void push_index(IndexList *list, size_t index) {
    list->length++;

    const size_t newAlloc = list->length * sizeof(size_t);

    if (list->indexList == NULL) {
        list->indexList = d_calloc(list->length, sizeof(size_t));
    } else {
        list->indexList = d_realloc(list->indexList, newAlloc);
    }

    list->indexList[list->length - 1] = index;
}

/**
 * \fn static void free_index_list(IndexList *list)
 * \brief Free the contents of an index list.
 *
 * \param list The index list to free.
 */
static void free_index_list(IndexList *list) {
    if (list->indexList != NULL) {
        free(list->indexList);
    }

    list->indexList = NULL;
    list->length    = 0;
}

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
const char *d_obj_generate(Sheet *sheet, size_t *size) {
    // First, check to see if the sheet has any C functions defined.
    // We are checking because we cannot compile the sheet into an object file
    // if the sheet relies on a C function that won't be put in.
    if (sheet->numCFunctions > 0) {
        printf("%s cannot be compiled into an object file: Has C functions.",
               sheet->filePath);
        return NULL;
    }

    // Create the ObjectWriter struct.
    ObjectWriter writer = {NULL, 0};

#ifdef DECISION_32
    write_string_n(&writer, "D32", 3);
#else
    write_string_n(&writer, "D64", 3);
#endif // DECISION_32

    // Add the current version of Decision.
    write_byte(&writer, DECISION_VERSION_MAJOR);
    write_byte(&writer, DECISION_VERSION_MINOR);
    write_byte(&writer, DECISION_VERSION_PATCH);

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

            int linkIndex =
                find_link(sheet->_link, funcDef.name, LINK_FUNCTION);

            // TODO: Error if the index is invalid.

            if (linkIndex >= 0) {
                write_uinteger(&writer, linkIndex);
                write_definition(&writer, funcDef, false);
            }
        }
    }

    // .var
    // If there are no variables, don't bother putting anything here.
    if (sheet->numVariables > 0) {
        write_string_n(&writer, ".var", 4);
        write_uinteger(&writer, sheet->numVariables);

        for (size_t i = 0; i < sheet->numVariables; i++) {
            const SocketMeta varMeta = sheet->variables[i].variableMeta;

            LinkType linkType = LINK_VARIABLE;

            if (varMeta.type == TYPE_STRING) {
                linkType = LINK_VARIABLE_POINTER;
            }

            int linkIndex = find_link(sheet->_link, varMeta.name, linkType);

            // TODO: Error if the index is invalid.

            if (linkIndex >= 0) {
                write_uinteger(&writer, linkIndex);
                write_socket_meta(&writer, varMeta, false, false);
            }
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

    *size = writer.len;
    return (const char *)writer.obj;
}

/**
 * \fn Sheet *d_obj_load(const char *obj, size_t size,
 *                              const char *filePath, Sheet **includes)
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
 * \param includes A NULL-terminated list of initially included sheets.
 * Can be NULL.
 */
Sheet *d_obj_load(const char *obj, size_t size, const char *filePath,
                  Sheet **includes) {
    Sheet *out       = d_sheet_create(filePath);
    out->_isCompiled = true;

    if (includes != NULL) {
        Sheet **include = includes;

        while (*include) {
            d_sheet_add_include(out, *include);
            include++;
        }
    }

    // TODO: Account for edianness in the instructions, and also for variables
    // in the data section.

    ObjectReader reader        = {NULL, 0, 0};
    reader.obj                 = obj;
    *(size_t *)(&(reader.len)) = size;

    if (!reader_test_string_n(&reader, "D", 1)) {
        printf("%s cannot be loaded: object file is not a valid object file.\n",
               filePath);
        out->hasErrors = true;
        return out;
    }

// We need to check if sizeof(dint) is the same as it is in the object.
#ifdef DECISION_32
    if (!reader_test_string_n(&reader, "32", 2)) {
        printf("%s cannot be loaded: object file is not 32-bit.\n", filePath);
        out->hasErrors = true;
        return out;
    }
#else
    if (!reader_test_string_n(&reader, "64", 2)) {
        printf("%s cannot be loaded: object file is not 64-bit.\n", filePath);
        out->hasErrors = true;
        return out;
    }
#endif // DECISION_32

    // Load this object file's Decision version.
    unsigned char major = read_byte(&reader);
    unsigned char minor = read_byte(&reader);
    unsigned char patch = read_byte(&reader);

    // Is this version in the past or in the future?
    short cmp = 0;

    if (major > DECISION_VERSION_MAJOR) {
        cmp = 1;
    } else if (major < DECISION_VERSION_MAJOR) {
        cmp = -1;
    } else {
        if (minor > DECISION_VERSION_MINOR) {
            cmp = 1;
        } else if (minor < DECISION_VERSION_MINOR) {
            cmp = -1;
        } else {
            if (patch > DECISION_VERSION_PATCH) {
                cmp = 1;
            } else if (patch < DECISION_VERSION_PATCH) {
                cmp = -1;
            }
        }
    }

    // If the object file was written in the future, warn the user that things
    // are very likely going to break.
    if (cmp > 0) {
        printf("Warning: %s was compiled with a future version of Decision "
               "(%hhi.%hhi.%hhi)\n",
               filePath, major, minor, patch);
    }

    // .text
    if (reader_test_string_n(&reader, ".text", 5)) {
        size_t textSize = read_uinteger(&reader);
        char *text      = read_string_n(&reader, textSize);

        out->_text     = text;
        out->_textSize = textSize;
    } else {
        out->_text     = NULL;
        out->_textSize = 0;
    }

    // .main
    if (reader_test_string_n(&reader, ".main", 5)) {
        out->_main = read_uinteger(&reader);
    } else {
        out->_main = 0;
    }

    // .data
    if (reader_test_string_n(&reader, ".data", 5)) {
        size_t dataSize = read_uinteger(&reader);
        char *data      = read_string_n(&reader, dataSize);

        out->_data     = data;
        out->_dataSize = dataSize;
    } else {
        out->_data     = NULL;
        out->_dataSize = 0;
    }

    // .lmeta
    if (reader_test_string_n(&reader, ".lmeta", 6)) {
        size_t numMeta = read_uinteger(&reader);

        for (size_t i = 0; i < numMeta; i++) {
            LinkMeta meta;

            meta.type = read_byte(&reader);
            meta.name = read_string(&reader);
            meta._ptr = (char *)read_uinteger(&reader);

            // If the metadata isn't in our sheet, then we don't know where it
            // is at all. This will need to be found out at link time.
            if (meta._ptr == (char *)-1) {
                meta.meta = (void *)-1;
            }

            d_link_meta_list_push(&(out->_link), meta);
        }
    } else {
        out->_link.list = NULL;
        out->_link.size = 0;
    }

    // .link
    if (reader_test_string_n(&reader, ".link", 5)) {
        size_t numLinks = read_uinteger(&reader);

        out->_insLinkList     = d_calloc(numLinks, sizeof(InstructionToLink));
        out->_insLinkListSize = numLinks;

        for (size_t i = 0; i < numLinks; i++) {
            InstructionToLink itl;

            itl.ins  = read_uinteger(&reader);
            itl.link = read_uinteger(&reader);

            out->_insLinkList[i] = itl;
        }
    } else {
        out->_insLinkList     = NULL;
        out->_insLinkListSize = 0;
    }

    // .func
    if (reader_test_string_n(&reader, ".func", 5)) {
        size_t numFunctions = read_uinteger(&reader);

        // We need to make sure that the entries in the LinkMetaList of the
        // sheet point to these SheetFunctions we are about to make. But we can
        // only link them once we've created the array of functions fully, as
        // reallocing *may* move the address of the array.
        IndexList funcMetaIndexList = {NULL, 0};

        for (size_t i = 0; i < numFunctions; i++) {
            // TODO: Error if the index is out of bounds.
            size_t metaLinkIndex = read_uinteger(&reader);

            // Copy the name of the function from the link meta list.
            const char *name = out->_link.list[metaLinkIndex].name;
            size_t nameSize  = strlen(name) + 1;
            char *newName    = d_calloc(nameSize, sizeof(char));
            strcpy(newName, name);

            NodeDefinition funcDef = read_definition(&reader, false);
            funcDef.name           = newName;

            // Add the function to the sheet.
            d_sheet_add_function(out, funcDef);

            // Add the LinkMetaList index to the dynamic array we are creating.
            push_index(&funcMetaIndexList, metaLinkIndex);
        }

        // Now the function array has been fully created, we can now safely
        // point to the functions.
        for (size_t i = 0; i < funcMetaIndexList.length; i++) {
            size_t funcMetaIndex = funcMetaIndexList.indexList[i];

            // NOTE: This assumes there were no functions in the sheet
            // beforehand.
            out->_link.list[funcMetaIndex].meta = (void *)(out->functions + i);
        }

        free_index_list(&funcMetaIndexList);
    } else {
        out->functions    = NULL;
        out->numFunctions = 0;
    }

    // .var
    if (reader_test_string_n(&reader, ".var", 4)) {
        size_t numVars = read_uinteger(&reader);

        // We need to make sure that the entries in the LinkMetaList of the
        // sheet point to these SheetVariables we are about to make. But we can
        // only link them once we've created the array of variables fully, as
        // reallocing *may* move the address of the array.
        IndexList varMetaIndexList = {NULL, 0};

        for (size_t i = 0; i < numVars; i++) {
            // TODO: Error if the index is out of bounds.
            size_t metaLinkIndex = read_uinteger(&reader);

            SocketMeta varMeta = read_socket_meta(&reader, false, false);

            LinkMeta varLinkMeta = out->_link.list[metaLinkIndex];

            // Reference the name of the function from the link meta list.
            varMeta.name = varLinkMeta.name;

            // Reference the default value from the data section.
            char *ptr = out->_data + (size_t)varLinkMeta._ptr;

            if (varMeta.type == TYPE_BOOL) {
                varMeta.defaultValue.integerValue = *ptr;
            } else if (varMeta.type == TYPE_STRING) {
                varMeta.defaultValue.stringValue = (char *)(*(dint *)ptr);
            } else {
                varMeta.defaultValue.integerValue = *(dint *)ptr;
            }

            // Add the variable to the sheet.
            d_sheet_add_variable(out, varMeta);

            // Add the LinkMetaList index to the dynamic array we are creating.
            push_index(&varMetaIndexList, metaLinkIndex);
        }

        // Now the variable array has been fully created, we can now safely
        // point to the variables.
        for (size_t i = 0; i < varMetaIndexList.length; i++) {
            size_t funcMetaIndex = varMetaIndexList.indexList[i];

            // NOTE: This assumes there were no variables in the sheet
            // beforehand.
            out->_link.list[funcMetaIndex].meta = (void *)(out->variables + i);
        }

        free_index_list(&varMetaIndexList);
    } else {
        out->variables    = NULL;
        out->numVariables = 0;
    }

    // .incl
    if (reader_test_string_n(&reader, ".incl", 5)) {
        size_t numIncludes = read_uinteger(&reader);

        for (size_t i = 0; i < numIncludes; i++) {
            const char *path = read_string(&reader);

            // Check that it wasn't in the includes list!
            Sheet **test    = includes;
            bool inIncludes = false;

            if (includes != NULL) {
                while (*test) {
                    Sheet *testInclude = *test;
                    if (strcmp(path, testInclude->filePath) == 0) {
                        inIncludes = true;
                        break;
                    }

                    test++;
                }
            }

            if (!inIncludes) {
                Sheet *include = d_sheet_add_include_from_path(out, path, false);

                if (include->hasErrors) {
                    ERROR_COMPILER(out->filePath, 0, true,
                                   "Included sheet %s produced errors",
                                   include->filePath);
                }
            }

            // When the file path enters the sheet, it is copied, so we can
            // safely free our copy here.
            free((char *)path);
        }
    } else {
        out->includes    = NULL;
        out->numIncludes = 0;
    }

    // There isn't a C function section, but we may still need to find the
    // metadata of C functions in our includes.
    for (size_t i = 0; i < out->_link.size; i++) {
        LinkMeta *meta = out->_link.list + i;

        if (meta->type == LINK_CFUNCTION) {
            NameDefinition nameDefinition;

            if (d_get_definition(out, meta->name, 0, NULL, &nameDefinition)) {
                if (nameDefinition.type == NAME_CFUNCTION) {
                    meta->meta = nameDefinition.definition.cFunction;
                } else {
                    printf("%s failed to load: %s is not a C function.\n",
                           out->filePath, meta->name);
                    out->hasErrors = true;
                }
            } else {
                printf("%s failed to load: Could not find definition of C "
                       "function %s.\n",
                       out->filePath, meta->name);
                out->hasErrors = true;
            }
        }
    }

    return out;
}