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

#include "dname.h"

#include "dcfunc.h"
#include "dcore.h"
#include "decision.h"
#include "derror.h"
#include "dmalloc.h"
#include "dsheet.h"

#include <stdlib.h>
#include <string.h>

/*
    void add_name_definition(AllNameDefinitions* all, NameDefinition definition)
    Add a name definition to the array of all name definitions.
    Similar to the LIST_PUSH macro in dsheet.c

    AllNameDefinitions* all: The list of name definitions to add onto.
    NameDefinition definition: The name definition to add on.
*/
static void add_name_definition(AllNameDefinitions *all,
                                NameDefinition definition) {
    size_t newNumDefinitions = all->numDefinitions + 1;
    if (newNumDefinitions == 1)
        all->definitions = d_malloc(sizeof(NameDefinition));
    else
        all->definitions = d_realloc(
            all->definitions, newNumDefinitions * sizeof(NameDefinition));
    all->definitions[all->numDefinitions++] = definition;
}

/*
    void get_name_definitions_recursive(struct _sheet* sheet, const char* name,
   AllNameDefinitions* state) Recursively find a definition of a name.

    Sheet* sheet: The sheet to start looking from.
    const char* name: The name to query.
    AllNameDefinitions* state: The state of the recursion. This is added onto
   when we find a name definition.
*/
static void get_name_definitions_recursive(Sheet *sheet, const char *name,
                                           AllNameDefinitions *state) {
    VERBOSE(5, "Checking sheet %s for name %s...\n", sheet->filePath, name)

    // First, check to see if it is a core function.
    CoreFunction coreFunc = d_core_find_name(name);

    if ((int)coreFunc > -1) {
        NameDefinition definition;
        definition.sheet               = sheet;
        definition.type                = NAME_CORE;
        definition.definition.coreFunc = coreFunc;

        add_name_definition(state, definition);

        VERBOSE(5, "Core function: YES\n")
    } else {
        VERBOSE(5, "Core function: NO\n")
    }

    // Maybe it's a variable name?
    if (sheet->variables != NULL && sheet->numVariables > 0) {
        size_t hits = 0;
        for (size_t i = 0; i < sheet->numVariables; i++) {
            if (strcmp(name, sheet->variables[i].variableMeta.name) == 0) {
                hits++;

                NameDefinition definition;
                definition.sheet               = sheet;
                definition.type                = NAME_VARIABLE;
                definition.definition.variable = &(sheet->variables[i]);

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "Variable: %zu\n", hits)
    } else {
        VERBOSE(
            5,
            "Not checking if a variable, no variables defined in this sheet.\n")
    }

    // Maybe, just maybe... it's a function name?
    if (sheet->functions != NULL && sheet->numFunctions > 0) {
        size_t hits = 0;
        for (size_t i = 0; i < sheet->numFunctions; i++) {
            if (strcmp(name, sheet->functions[i].functionDefinition.name) ==
                0) {
                hits++;

                NameDefinition definition;
                definition.sheet               = sheet;
                definition.type                = NAME_FUNCTION;
                definition.definition.function = &(sheet->functions[i]);

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "Function: %zu\n", hits)
    } else {
        VERBOSE(
            5,
            "Not checking if a function, no functions defined in this sheet.\n")
    }

    // It could also be a C function.
    if (sheet->cFunctions != NULL && sheet->numCFunctions > 0) {
        size_t hits = 0;
        for (size_t i = 0; i < sheet->numCFunctions; i++) {
            if (strcmp(name, sheet->cFunctions[i].definition.name) == 0) {
                hits++;

                NameDefinition definition;
                definition.sheet                = sheet;
                definition.type                 = NAME_CFUNCTION;
                definition.definition.cFunction = &(sheet->cFunctions[i]);

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "C Function: %zu\n", hits);
    } else {
        VERBOSE(5, "Not checking if a C function, no C functions defined in "
                   "this sheet.\n")
    }

    // Please sir... may I have some more names? (If it isn't a core function)
    if (sheet->includes != NULL && sheet->numIncludes > 0 &&
        (int)coreFunc == -1) {
        for (size_t i = 0; i < sheet->numIncludes; i++) {
            get_name_definitions_recursive(sheet->includes[i], name, state);
        }
    }
}

/**
 * \fn AllNameDefinitions d_get_name_definitions(Sheet *sheet, const char *name)
 * \brief Get all of the places where a name is defined, and what the name's
 * type is.
 *
 * We will also check recursively up the includes of sheets.
 *
 * \return An array of NameDefinition.
 *
 * \param sheet The sheet to start looking from.
 * \param name The name to query.
 */
AllNameDefinitions d_get_name_definitions(Sheet *sheet, const char *name) {
    VERBOSE(5, "Finding definitions for name %s...\n", name)

    AllNameDefinitions allDefinitions = (AllNameDefinitions){NULL, 0};
    get_name_definitions_recursive(sheet, name, &allDefinitions);

    VERBOSE(5, "Found %zu results for name %s.\n",
            allDefinitions.numDefinitions, name)

    return allDefinitions;
}

/**
 * \fn void d_free_name_definitions(AllNameDefinitions *definitions)
 * \brief Free an `AllNameDefinitions` struct. It should not be used after it
 * has been freed.
 *
 * \param definitions The structure to free.
 */
void d_free_name_definitions(AllNameDefinitions *definitions) {
    if (definitions->definitions != NULL) {
        free(definitions->definitions);
        definitions->definitions = NULL;
    }

    definitions->numDefinitions = 0;
}

/* The definition of the Start node. */
static const SocketMeta startSocket = {
    "start",
    "This output gets activated when the program starts.",
    TYPE_EXECUTION,
    {0}};

static const NodeDefinition startDefinition = {
    "Start",
    "The node that gets activated first when the program starts.",
    &startSocket,
    1,
    0,
    false};

/**
 * \fn const NodeDefinition *d_get_definition(Sheet *sheet, const char *name,
 *                                            size_t lineNum,
 *                                            const char *funcName,
 *                                            NameDefinition *nameDef)
 * \brief Get a node's definition from it's name.
 *
 * \return The node's definition.
 *
 * \param sheet The sheet the node is a part of.
 * \param name The name of the node.
 * \param lineNum In case we error, say where we errored from.
 * \param funcName If the name is Define or Return, we need the function name
 * so we can get the correct sockets.
 * \param nameDef A pointer that is set to the node's name definition. If the
 * node definition returns NULL, do not trust this value.
 */
const NodeDefinition *d_get_definition(Sheet *sheet, const char *name,
                                       size_t lineNum, const char *funcName,
                                       NameDefinition *nameDef) {
    VERBOSE(5, "Getting node definitions of node %s on line %zu in %s...\n",
            name, lineNum, sheet->filePath);

    NameDefinition nameDefinition;

    // First, let's check if it's a special function like Start.
    // We do this now so we don't need to search all of our includes
    // for this function.
    if (strcmp(name, "Start") == 0) {
        return &startDefinition;
    } else if (strcmp(name, "Return") == 0) {
        // For return, we need to make sure funcName is the name
        // of a function that is defined in the same sheet!
        if (funcName != NULL) {
            AllNameDefinitions funcDefinitions =
                d_get_name_definitions(sheet, funcName);
            if (funcDefinitions.numDefinitions == 1) {
                nameDefinition = funcDefinitions.definitions[0];
                if (nameDefinition.sheet == sheet) {
                    SheetFunction *func = nameDefinition.definition.function;
                    d_free_name_definitions(&funcDefinitions);
                    *nameDef = nameDefinition;
                    return &(func->returnDefinition);
                } else {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Return node for function %s that is not "
                                   "defined on the same sheet",
                                   funcName);
                }
            } else if (funcDefinitions.numDefinitions == 0) {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Return call for undefined function %s",
                               funcName);
            } else {
                ERROR_COMPILER(
                    sheet->filePath, lineNum, true,
                    "Return call for function %s defined multiple times",
                    funcName);
            }

            d_free_name_definitions(&funcDefinitions);
        } else {
            d_error_compiler_push("Return call but function name not found",
                                  sheet->filePath, lineNum, true);
        }
    } else if (strcmp(name, "Define") == 0) {
        // For define, we need to make sure funcName is the name
        // of a function that is defined in the same sheet!
        if (funcName != NULL) {
            AllNameDefinitions funcDefinitions =
                d_get_name_definitions(sheet, funcName);
            if (funcDefinitions.numDefinitions == 1) {
                nameDefinition = funcDefinitions.definitions[0];
                if (nameDefinition.sheet == sheet) {
                    SheetFunction *func = nameDefinition.definition.function;
                    d_free_name_definitions(&funcDefinitions);
                    *nameDef = nameDefinition;
                    return &(func->defineDefinition);
                } else {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Define node for function %s that is not "
                                   "defined on the same sheet",
                                   funcName);
                }
            } else if (funcDefinitions.numDefinitions == 0) {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Define call for undefined function %s",
                               funcName);
            } else {
                ERROR_COMPILER(
                    sheet->filePath, lineNum, true,
                    "Define call for function %s defined multiple times",
                    funcName);
            }

            d_free_name_definitions(&funcDefinitions);
        } else {
            d_error_compiler_push("Define call but function name not found",
                                  sheet->filePath, lineNum, true);
        }
    } else {
        // Let's find out if the name we're looking for exists, first.
        AllNameDefinitions nameDefinitions =
            d_get_name_definitions(sheet, name);
        if (nameDefinitions.numDefinitions == 1) {
            nameDefinition = nameDefinitions.definitions[0];
            switch (nameDefinition.type) {
                case NAME_CORE:;

                    // This bit's simple, all of the info we need is in dcore.h
                    d_free_name_definitions(&nameDefinitions);
                    *nameDef = nameDefinition;
                    return d_core_get_definition(
                        nameDefinition.definition.coreFunc);

                // Specifically, the GETTER of a variable.
                case NAME_VARIABLE:;

                    // The definition of the getter will be stored in the
                    // variable structure itself.
                    SheetVariable *var = nameDefinition.definition.variable;
                    d_free_name_definitions(&nameDefinitions);
                    *nameDef = nameDefinition;
                    return &(var->getterDefinition);

                // Specifically, the calling of a function.
                case NAME_FUNCTION:;

                    SheetFunction *func = nameDefinition.definition.function;
                    d_free_name_definitions(&nameDefinitions);
                    *nameDef = nameDefinition;
                    return &(func->functionDefinition);

                // Specifically, the calling of a C function.
                case NAME_CFUNCTION:;

                    const CFunction *cFunction =
                        nameDefinition.definition.cFunction;

                    // This one is simple, all the info we need is in the
                    // CFunction struct.
                    d_free_name_definitions(&nameDefinitions);
                    *nameDef = nameDefinition;
                    return &(cFunction->definition);

                default:
                    d_error_compiler_push("Name definition is not that of a "
                                          "core function, variable, function "
                                          "or C function",
                                          sheet->filePath, lineNum, true);
                    break;
            }
        } else if (nameDefinitions.numDefinitions == 0) {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "Name %s is not defined", name);
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "Name %s defined multiple times", name);
        }

        d_free_name_definitions(&nameDefinitions);
    }

    return NULL;
}