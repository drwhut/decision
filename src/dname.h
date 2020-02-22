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

/**
 * \file dname.h
 * \brief This header contains functions to handle the names of things in
 * Decision, like the names of variables, functions, etc.
 */

#ifndef DNAME_H
#define DNAME_H

#include "dcfg.h"
#include "dcore.h"

#include <stdbool.h>
#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* Forward declaration of the SheetVariable struct from dsheet.h */
struct _sheetVariable;

/* Forward declaration of the SheetFunction struct from dsheet.h */
struct _sheetFunction;

/* Forward declaration of the NodeDefinition struct from dsheet.h */
struct _nodeDefinition;

/* Forward declaration of the CFunction struct from dcfunc.h */
struct _cFunction;

/**
 * \enum _nameType
 * \brief An enum for saying what a name corresponds to.
 *
 * \typedef enum _nameType NameType
 */
typedef enum _nameType {
    NAME_CORE,      // = 0
    NAME_VARIABLE,  // = 1
    NAME_FUNCTION,  // = 2
    NAME_CFUNCTION, // = 3
} NameType;

/**
 * \struct _nameDefinition
 * \brief A struct saying where a name is defined.
 *
 * \typedef struct _nameDefinition NameDefinition
 */
typedef struct _nameDefinition {
    struct _sheet *sheet;
    NameType type;
    union {
        CoreFunction coreFunc;
        struct _sheetVariable *variable;
        struct _sheetFunction *function;
        struct _cFunction *cFunction;
    } definition;
} NameDefinition;

/**
 * \struct _allNameDefinitions
 * \brief A list of all of a name's definitions.
 *
 * \typedef struct _allNameDefinitions AllNameDefinitions
 */
typedef struct _allNameDefinitions {
    NameDefinition *definitions;
    size_t numDefinitions;
} AllNameDefinitions;

/*
=== FUNCTIONS =============================================
*/

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
DECISION_API AllNameDefinitions d_get_name_definitions(struct _sheet *sheet,
                                                       const char *name);

/**
 * \fn bool d_select_name_definition(const char *name,
 *                                   AllNameDefinitions allDefinitions,
 *                                   NameDefinition *selection)
 * \brief Given a set of definitions, select the one the user intended, give
 * the name of the node.
 *
 * \return If a definition was selected.
 *
 * \param name The name that decides the definition to use.
 * \param allDefinitions The set of definitions to choose from.
 * \param selection A pointer whose value is set to the definition if it was
 * selected, i.e. if we return `true`. If `false` is returned, do not trust
 * this value.
 */
DECISION_API bool d_select_name_definition(const char *name,
                                           AllNameDefinitions allDefinitions,
                                           NameDefinition *selection);

/**
 * \fn void d_free_name_definitions(AllNameDefinitions *definitions)
 * \brief Free an `AllNameDefinitions` struct. It should not be used after it
 * has been freed.
 *
 * \param definitions The structure to free.
 */
DECISION_API void d_free_name_definitions(AllNameDefinitions *definitions);

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
DECISION_API const struct _nodeDefinition *
d_get_definition(struct _sheet *sheet, const char *name, size_t lineNum,
                 const char *funcName, NameDefinition *nameDef);

#endif // DNAME_H