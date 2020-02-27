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

#include "dsheet.h"

#include "decision.h"
#include "derror.h"
#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * \def LIST_PUSH(array, arrayType, numCurrentItems, newItem)
 * \brief A generic macro for adding items to a dynamic array.
 */
#define LIST_PUSH(array, arrayType, numCurrentItems, newItem)      \
    size_t newNumItems = (numCurrentItems) + 1;                    \
    if (newNumItems == 1)                                          \
        array = d_calloc(1, sizeof(arrayType));                    \
    else                                                           \
        array = d_realloc(array, newNumItems * sizeof(arrayType)); \
    memcpy(array + numCurrentItems++, &newItem, sizeof(arrayType));

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param varMeta The variable metadata to add.
 */
void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta) {
    // Define the getter node definition.

    // Copy the name.
    size_t nameSize  = strlen(varMeta.name) + 1;
    char *nameGetter = d_calloc(nameSize, sizeof(char));
    memcpy(nameGetter, varMeta.name, nameSize);

    // Create a new description for the getter, i.e.
    // "Get the value of the variable <VARIABLE NAME>."
    size_t descriptionSize  = 32 + strlen(varMeta.name);
    char *descriptionGetter = d_calloc(descriptionSize, sizeof(char));
    sprintf(descriptionGetter, "Get the value of the variable %s.",
            varMeta.name);

    // Copy the variable metadata.
    SocketMeta *getterMeta = d_malloc(sizeof(SocketMeta));
    memcpy(getterMeta, &varMeta, sizeof(SocketMeta));

    NodeDefinition getter;
    getter.name             = nameGetter;
    getter.description      = descriptionGetter;
    getter.sockets          = getterMeta;
    getter.numSockets       = 1;
    getter.startOutputIndex = 0;
    getter.infiniteInputs   = false;

    SheetVariable variable;
    *(SocketMeta *)&(variable.variableMeta)         = varMeta;
    *(NodeDefinition *)&(variable.getterDefinition) = getter;
    variable.sheet                                  = sheet;

    LIST_PUSH(sheet->variables, SheetVariable, sheet->numVariables, variable)
}

/* The names and descriptions of Define and Return name sockets. */
static const char *defineName        = "function/subroutine";
static const char *defineDescription = "The function or subroutine to define.";

static const char *returnName = "function/subroutine";
static const char *returnDescription =
    "The function or subroutine to return from.";

/**
 * \fn void d_sheet_add_function(Sheet *sheet, const NodeDefinition funcDef)
 * \brief Add a function to a sheet.
 *
 * \param sheet The sheet to add the function to.
 * \param funcDef The function definition to add.
 */
void d_sheet_add_function(Sheet *sheet, const NodeDefinition funcDef) {
    // Before we add the function to the sheet, we need to know what the Define
    // and Return nodes for this function will look like.

    char *nameDefine = d_calloc(7, sizeof(char));
    strcpy(nameDefine, "Define");

    char *descriptionDefine = d_calloc(33, sizeof(char));
    strcpy(descriptionDefine, "Define a function or subroutine.");

    const size_t numInputs        = d_definition_num_inputs(&funcDef);
    const size_t numSocketsDefine = 1 + numInputs;

    SocketMeta *defineMeta = d_calloc(numSocketsDefine, sizeof(SocketMeta));

    SocketMeta defineNameSocket;
    defineNameSocket.name                     = defineName;
    defineNameSocket.description              = defineDescription;
    defineNameSocket.type                     = TYPE_NAME;
    defineNameSocket.defaultValue.stringValue = (char *)funcDef.name;

    memcpy(defineMeta, &defineNameSocket, sizeof(SocketMeta));
    memcpy(defineMeta + 1, funcDef.sockets, numInputs * sizeof(SocketMeta));

    NodeDefinition defineDef;
    defineDef.name             = nameDefine;
    defineDef.description      = descriptionDefine;
    defineDef.sockets          = defineMeta;
    defineDef.numSockets       = numSocketsDefine;
    defineDef.startOutputIndex = 1;
    defineDef.infiniteInputs   = false;

    char *nameReturn = d_calloc(7, sizeof(char));
    strcpy(nameReturn, "Return");

    char *descriptionReturn = d_calloc(38, sizeof(char));
    strcpy(descriptionReturn, "Return from a function or subroutine.");

    const size_t numOutputs       = d_definition_num_outputs(&funcDef);
    const size_t numSocketsReturn = 1 + numOutputs;

    SocketMeta *returnMeta = d_calloc(numSocketsReturn, sizeof(SocketMeta));

    SocketMeta returnNameSocket;
    returnNameSocket.name                     = returnName;
    returnNameSocket.description              = returnDescription;
    returnNameSocket.type                     = TYPE_NAME;
    returnNameSocket.defaultValue.stringValue = (char *)funcDef.name;

    memcpy(returnMeta, &returnNameSocket, sizeof(SocketMeta));
    memcpy(returnMeta + 1, funcDef.sockets + funcDef.startOutputIndex,
           numOutputs * sizeof(SocketMeta));

    NodeDefinition returnDef;
    returnDef.name             = nameReturn;
    returnDef.description      = descriptionReturn;
    returnDef.sockets          = returnMeta;
    returnDef.numSockets       = numSocketsReturn;
    returnDef.startOutputIndex = numSocketsReturn;
    returnDef.infiniteInputs   = false;

    SheetFunction func;
    *(NodeDefinition *)&(func.functionDefinition) = funcDef;
    *(NodeDefinition *)&(func.defineDefinition)   = defineDef;
    *(NodeDefinition *)&(func.returnDefinition)   = returnDef;
    func.defineNodeIndex                          = 0;
    func.numDefineNodes                           = 0;
    func.lastReturnNodeIndex                      = 0;
    func.numReturnNodes                           = 0;
    func.sheet                                    = sheet;

    LIST_PUSH(sheet->functions, SheetFunction, sheet->numFunctions, func)
}

/**
 * \fn void d_sheet_add_c_function(Sheet *sheet, CFunction cFunction)
 * \brief Add a C function to a sheet.
 *
 * **NOTE:** To create a C function, have a look at `dcfunc.h`.
 *
 * \param sheet The sheet to add the C function to.
 * \param cFunction The C function to add.
 */
void d_sheet_add_c_function(Sheet *sheet, CFunction cFunction) {
    LIST_PUSH(sheet->cFunctions, CFunction, sheet->numCFunctions, cFunction);
}

/**
 * \fn bool d_is_subroutine(SheetFunction func)
 * \brief Is the given function a subroutine?
 *
 * \return If the function is a subroutine.
 *
 * \param func The function to query.
 */
bool d_is_subroutine(SheetFunction func) {
    return d_is_execution_definition(&(func.functionDefinition));
}

/**
 * \fn void d_sheet_add_include(Sheet *sheet, Sheet *include)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \param sheet The sheet to add the include to.
 * \param include The sheet to include.
 */
void d_sheet_add_include(Sheet *sheet, Sheet *include) {
    LIST_PUSH(sheet->includes, Sheet *, sheet->numIncludes, include);
}

/**
 * \fn Sheet *d_sheet_add_include_from_path(Sheet *sheet,
 *                                          const char *includePath,
 *                                          Sheet **priors,
 *                                          bool debugInclude)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \return A pointer to the sheet that was created from the include path.
 *
 * \param sheet The sheet to add the include to.
 * \param includePath The path from sheet to the sheet being included.
 * Note that this should be equivalent to the argument of the Include property.
 * \param priors A NULL-terminated list of sheets that, if included, will throw
 * an error. This is to prevent circular includes.
 * \param debugInclude If we can compile the included sheet in debug mode,
 * do so if set to true.
 */
Sheet *d_sheet_add_include_from_path(Sheet *sheet, const char *includePath,
                                     Sheet **priors, bool debugInclude) {
    // TODO: Implement standard library paths as well.

    // If the current sheet was accessed from a different directory,
    // we need to stick to that directory for this include as well.
    // i.e. If we ran: decision ../main.dc, we need to include ../include.dc.
    // If main.dc and include.dc were in the same directory.

    // Copy the file path of the current sheet.
    const size_t filePathLen = strlen(sheet->filePath);
    char *dir                = d_calloc(filePathLen + 1, sizeof(char));
    memcpy(dir, sheet->filePath, filePathLen + 1);

    // Find the last / or \ character.
    long lastSeperator = (long)filePathLen - 1;

    for (; lastSeperator >= 0; lastSeperator--) {
        if (dir[lastSeperator] == '/' || dir[lastSeperator] == '\\') {
            dir[lastSeperator + 1] = 0; // While we're here, put a NULL in front
                                        // of the last seperator.
            break;
        }
    }

    // If there isn't either character, we don't need to worry about
    // changing the directory.
    const char *finalPath = includePath;

    if ((int)lastSeperator >= 0) {
        // Concatenate the dir string (with the NULL inserted) with the contents
        // of the literal string.
        const size_t newPathLength =
            (size_t)lastSeperator + 1 + strlen(includePath);
        dir = d_realloc(dir, newPathLength + 1);

        strcat(dir, includePath);

        finalPath = dir;
    }

    // We need to check if this path is the path of any of the prior sheets.
    // This way, we can check if we're about to enter a circular include cycle.
    if (priors != NULL) {
        Sheet **prior = priors;

        while (*prior) {
            if (strcmp((*prior)->filePath, finalPath) == 0) {
                printf("Fatal: Circular include detected from sheet %s\n",
                       finalPath);
                Sheet *errorSheet     = d_sheet_create(finalPath);
                errorSheet->hasErrors = true;
                return errorSheet;
            }

            prior++;
        }
    }

    CompileOptions opts = DEFAULT_COMPILE_OPTIONS;
    opts.debug          = debugInclude;

    // Add the current sheet to the list of priors when compiling the next
    // sheet.

    // The length of the priors list.
    size_t lenPriors = 0;

    if (priors != NULL) {
        Sheet **p = priors;
        while (*p) {
            lenPriors++;
            p++;
        }
    }

    Sheet **newPriors = d_calloc((lenPriors + 2), sizeof(Sheet **));
    memcpy(newPriors, priors, lenPriors * sizeof(Sheet **));
    *(newPriors + lenPriors) = sheet;

    opts.priors = newPriors;

    Sheet *includeSheet = d_load_file(finalPath, &opts);

    d_sheet_add_include(sheet, includeSheet);

    free(newPriors);

    // Set the includePath property of the included sheet, as we need to save
    // that value, instead of the directory, for if we run the sheet including
    // this sheet from a different working directory.
    const size_t includePathLen = strlen(includePath);
    char *cpyIncludePath        = d_calloc(includePathLen + 1, sizeof(char));
    memcpy(cpyIncludePath, includePath, includePathLen + 1);

    includeSheet->includePath = (const char *)cpyIncludePath;

    free(dir);

    return includeSheet;
}

/**
 * \fn Sheet *d_sheet_create(const char *filePath)
 * \brief Create a malloc'd sheet object.
 *
 * \return The malloc'd sheet object.
 *
 * \param filePath The file where this sheet originated.
 */
Sheet *d_sheet_create(const char *filePath) {
    Sheet *sheet = d_malloc(sizeof(Sheet));

    // Copy the file path string into heap memory so we know for sure we can
    // free it later.
    size_t filePathLen = strlen(filePath);
    char *cpyFilePath  = d_calloc(filePathLen + 1, sizeof(char));
    memcpy(cpyFilePath, filePath, filePathLen + 1);

    sheet->graph            = EMPTY_GRAPH;
    sheet->_debugInfo       = NO_DEBUG_INFO;
    sheet->_link            = d_link_new_meta_list();
    sheet->filePath         = (const char *)cpyFilePath;
    sheet->includePath      = NULL;
    sheet->includes         = NULL;
    sheet->numIncludes      = 0;
    sheet->variables        = NULL;
    sheet->numVariables     = 0;
    sheet->functions        = NULL;
    sheet->numFunctions     = 0;
    sheet->cFunctions       = NULL;
    sheet->numCFunctions    = 0;
    sheet->_main            = 0;
    sheet->_text            = NULL;
    sheet->_textSize        = 0;
    sheet->_data            = NULL;
    sheet->_dataSize        = 0;
    sheet->_insLinkList     = NULL;
    sheet->_insLinkListSize = 0;
    sheet->numStarts        = 0;
    sheet->startNodeIndex   = -1;
    sheet->hasErrors        = false;
    sheet->allowFree        = true;
    sheet->_isCompiled      = false;
    sheet->_isLinked        = false;

    return sheet;
}

/**
 * \fn void d_sheet_free(Sheet *sheet)
 * \brief Free malloc'd memory in a sheet.
 *
 * **NOTE:** This will also free all included sheets recursively!
 *
 * \param sheet The sheet to free from memory.
 */
void d_sheet_free(Sheet *sheet) {
    if (sheet != NULL) {
        // Dereferenced sheet here so VS stops giving us a warning about this
        // one line.
        Sheet sheetDeref = *sheet;
        free((char *)sheetDeref.filePath);
        sheet->filePath = NULL;

        if (sheet->includePath != NULL) {
            free((char *)sheet->includePath);
            sheet->includePath = NULL;
        }

        d_graph_free(&(sheet->graph));

        if (sheet->variables != NULL) {
            for (size_t i = 0; i < sheet->numVariables; i++) {
                SheetVariable var = sheet->variables[i];

                // Free the variable description. The name will be freed by the
                // link meta list.
                if (var.variableMeta.description != NULL) {
                    free((char *)var.variableMeta.description);
                }

                // Free the getter definition.
                d_definition_free(var.getterDefinition, false);
            }

            free(sheet->variables);
            sheet->variables    = NULL;
            sheet->numVariables = 0;
        }

        if (sheet->functions != NULL) {
            for (size_t i = 0; i < sheet->numFunctions; i++) {
                SheetFunction func = sheet->functions[i];

                // Free the definitions.
                d_definition_free(func.functionDefinition, true);
                d_definition_free(func.defineDefinition, false);
                d_definition_free(func.returnDefinition, false);
            }

            free(sheet->functions);
            sheet->functions    = NULL;
            sheet->numFunctions = 0;
        }

        if (sheet->cFunctions != NULL) {
            for (size_t i = 0; i < sheet->numCFunctions; i++) {
                CFunction cFunc = sheet->cFunctions[i];

                d_definition_free(cFunc.definition, true);
            }

            free(sheet->cFunctions);
            sheet->cFunctions    = NULL;
            sheet->numCFunctions = 0;
        }

        if (sheet->includes != NULL) {
            for (size_t i = 0; i < sheet->numIncludes; i++) {
                Sheet *include = sheet->includes[i];

                if (include->allowFree) {
                    d_sheet_free(include);
                }
            }

            free(sheet->includes);
            sheet->includes    = NULL;
            sheet->numIncludes = 0;
        }

        if (sheet->_text != NULL) {
            free(sheet->_text);
            sheet->_text     = NULL;
            sheet->_textSize = 0;
        }

        // Before we free the data section, we need to free any string variables
        // that will have been malloc'd. These pointers should only be malloc'd
        // when linking has taken place.
        if (sheet->_isLinked) {
            for (size_t i = 0; i < sheet->_link.size; i++) {
                LinkMeta meta = sheet->_link.list[i];

                if (meta.type == LINK_VARIABLE_POINTER) {
                    SheetVariable *var = (SheetVariable *)meta.meta;
                    Sheet *extSheet    = var->sheet;

                    // We can only free it if it is from our sheet.
                    if (sheet == extSheet) {
                        char *varLoc = (char *)sheet->_data + (size_t)meta._ptr;

                        // We needed to convert it from a char* so it shifted
                        // the corrent amount.
                        char *strPtr = *((char **)varLoc);

                        // Make sure it doesn't point to somewhere else in the
                        // .data section, we can't free it then!
                        if (strPtr < sheet->_data ||
                            strPtr >= sheet->_data + (size_t)sheet->_dataSize) {
                            if (strPtr != NULL) {
                                free(strPtr);
                            }
                        }
                    }
                }
            }
        }

        if (sheet->_data != NULL) {
            free(sheet->_data);
            sheet->_data     = NULL;
            sheet->_dataSize = 0;
        }

        d_debug_free_info(&(sheet->_debugInfo));

        d_link_free_list(&(sheet->_link));

        if (sheet->_insLinkList != NULL) {
            free(sheet->_insLinkList);
            sheet->_insLinkList     = NULL;
            sheet->_insLinkListSize = 0;
        }

        free(sheet);
    }
}

/**
 * \fn void d_variables_dump(SheetVariable *variables, size_t numVariables)
 * \brief Dump the details of an array of variables to `stdout`.
 *
 * \param variables The array of variables.
 * \param numVariables The number of variables in the array.
 */
void d_variables_dump(SheetVariable *variables, size_t numVariables) {
    printf("# Variables: %zu\n", numVariables);

    // Dump the variables, if there are any.
    if (variables != NULL && numVariables > 0) {
        for (size_t i = 0; i < numVariables; i++) {
            SheetVariable var  = variables[i];
            SocketMeta varMeta = var.variableMeta;

            printf("\tVariable %s is of type %s with default value ",
                   varMeta.name, d_type_name(varMeta.type));
            switch (varMeta.type) {
                case TYPE_INT:
                    printf("%" DINT_PRINTF_d,
                           varMeta.defaultValue.integerValue);
                    break;
                case TYPE_FLOAT:
                    printf("%f", varMeta.defaultValue.floatValue);
                    break;
                case TYPE_STRING:
                    printf("%s", varMeta.defaultValue.stringValue);
                    break;
                case TYPE_BOOL:
                    printf("%d", varMeta.defaultValue.booleanValue);
                    break;
                default:
                    break;
            }
            printf("\n");
        }
    }
}

/* A helper function for printing a node definition. */
static void dump_definition(const NodeDefinition *definition) {
    size_t numInputs  = d_definition_num_inputs(definition);
    size_t numOutputs = d_definition_num_outputs(definition);

    printf("\tFunction %s is %s with %zu arguments:\n", definition->name,
           ((d_is_execution_definition(definition)) ? "a SUBROUTINE"
                                                    : "a FUNCTION"),
           numInputs);

    if (definition->sockets != NULL && numInputs > 0) {
        for (size_t j = 0; j < numInputs; j++) {
            SocketMeta arg = definition->sockets[j];

            printf("\t\tArgument %s (%s) is of type %s with default "
                   "value ",
                   arg.name, arg.description, d_type_name(arg.type));
            switch (arg.type) {
                case TYPE_INT:
                    printf("%" DINT_PRINTF_d, arg.defaultValue.integerValue);
                    break;
                case TYPE_FLOAT:
                    printf("%f", arg.defaultValue.floatValue);
                    break;
                case TYPE_STRING:
                    printf("%s", arg.defaultValue.stringValue);
                    break;
                case TYPE_BOOL:
                    printf("%d", arg.defaultValue.booleanValue);
                    break;
                default:
                    break;
            }
            printf("\n");
        }
    }

    printf("\tand %zu returns:\n", numOutputs);

    if (definition->sockets != NULL && numOutputs > 0) {
        for (size_t j = 0; j < numOutputs; j++) {
            SocketMeta ret =
                definition->sockets[definition->startOutputIndex + j];

            printf("\t\tReturn %s (%s) is of type %s\n", ret.name,
                   ret.description, d_type_name(ret.type));
        }
    }
}

/**
 * \fn void d_functions_dump(SheetFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of functions to `stdout`.
 *
 * \param functions The array of functions.
 * \param numFunctions The number of functions in the array.
 */
void d_functions_dump(SheetFunction *functions, size_t numFunctions) {
    printf("# Functions: %zu\n", numFunctions);

    if (functions != NULL && numFunctions > 0) {
        for (size_t i = 0; i < numFunctions; i++) {
            SheetFunction function        = functions[i];
            const NodeDefinition *funcDef = &(function.functionDefinition);
            dump_definition(funcDef);
        }
    }
}

/**
 * \fn void d_c_functions_dump(CFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of C functions to `stdout`.
 *
 * \param functions The array of C functions.
 * \param numFunctions The number of functions in the array.
 */
void d_c_functions_dump(CFunction *functions, size_t numFunctions) {
    printf("# C Functions: %zu\n", numFunctions);

    if (functions != NULL && numFunctions > 0) {
        for (size_t i = 0; i < numFunctions; i++) {
            CFunction function            = functions[i];
            const NodeDefinition *funcDef = &(function.definition);
            dump_definition(funcDef);
        }
    }
}

/**
 * \fn void d_sheet_dump(Sheet *sheet)
 * \brief Dump the contents of a `Sheet` struct to `stdout`.
 *
 * \param sheet The sheet to dump.
 */
void d_sheet_dump(Sheet *sheet) {
    printf("\nSHEET %s DUMP\n", sheet->filePath);
    printf("# Start functions: %zu\n", sheet->numStarts);
    printf("# Includes: %zu\n", sheet->numIncludes);

    if (sheet->includes != NULL && sheet->numIncludes > 0) {
        for (size_t i = 0; i < sheet->numIncludes; i++) {
            printf("\t%s\n", sheet->includes[i]->filePath);
        }
    }

    // Dump the variables, if there are any.
    d_variables_dump(sheet->variables, sheet->numVariables);

    // Dump the functions, if there are any.
    d_functions_dump(sheet->functions, sheet->numFunctions);

    // Dump the C functions, if there are any.
    d_c_functions_dump(sheet->cFunctions, sheet->numCFunctions);

    // Dump the graph.
    d_graph_dump(sheet->graph);

    printf("\n");
}
