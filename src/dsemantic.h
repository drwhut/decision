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

/**
 * \file dsemantic.h
 * \brief This header checks that the program we're about to compile is correct.
 */

#ifndef DSEMANTIC_H
#define DSEMANTIC_H

#include "dbool.h"
#include "dcfg.h"
#include "dcore.h"
#include "dlex.h"
#include "dsyntax.h"
#include "dtype.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* Forward declaration of the SheetNode struct from dsheet.h */
struct _sheetNode;

/* Forward declaration of the SheetSocket struct from dsheet.h */
struct _sheetSocket;

/* Forward declaration of the SheetVariable struct from dsheet.h */
struct _sheetVariable;

/* Forward declaration of the SheetFunction struct from dsheet.h */
struct _sheetFunction;

/**
 * \struct _lineSocketPair
 * \brief A struct associating a line identifier with a socket.
 *
 * \typedef struct _lineSocketPair LineSocketPair
 */
typedef struct _lineSocketPair {
    dint identifier;
    struct _sheetSocket *socket;
} LineSocketPair;

/**
 * \enum _nameType
 * \brief An enum for saying what a name corresponds to.
 *
 * \typedef enum _nameType NameType
 */
typedef enum _nameType {
    NAME_CORE,     // = 0
    NAME_VARIABLE, // = 1
    NAME_FUNCTION, // = 2
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
    CoreFunction coreFunc;
    struct _sheetVariable *variable;
    struct _sheetFunction *function;
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

/**
 * \struct _nodeTrueProperties
 * \brief A struct to describe a node's *true* properties.
 *
 * \typedef struct _nodeTrueProperties NodeTrueProperties
 */
typedef struct _nodeTrueProperties {
    bool isDefined;
    bool needFuncName;
    bool _mallocd;

    const DType *inputTypes;
    long numInputs;

    const DType *outputTypes;
    long numOutputs;
} NodeTrueProperties;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn AllNameDefinitions d_semantic_get_name_definitions(Sheet *sheet,
 *                                                        const char *name)
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
DECISION_API AllNameDefinitions
d_semantic_get_name_definitions(struct _sheet *sheet, const char *name);

/**
 * \fn bool d_semantic_select_name_definition(const char *name,
 *                                            AllNameDefinitions allDefinitions,
 *                                            NameDefinition *selection)
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
DECISION_API bool
d_semantic_select_name_definition(const char *name,
                                  AllNameDefinitions allDefinitions,
                                  NameDefinition *selection);

/**
 * \fn void d_semantic_free_name_definitions(AllNameDefinitions definitions)
 * \brief Free an `AllNameDefinitions` struct. It should not be used after it
 * has been freed.
 *
 * \param definitions The structure to free.
 */
DECISION_API void
d_semantic_free_name_definitions(AllNameDefinitions definitions);

/**
 * \fn NodeTrueProperties d_semantic_get_node_properties(
 *     Sheet *sheet,
 *     const char *name,
 *     size_t lineNum,
 *     const char *funcName,
 *     NameDefinition *definition)
 * \brief Get a node's true properties (e.g. input and output types) by it's
 * name.
 *
 * \return A NodeTrueProperties object with malloc'd elements.
 *
 * \param sheet The sheet the node is a part of.
 * \param name The name of the node.
 * \param lineNum In case we error, say where we errored from.
 * \param funcName If the name is Return, we need the function name so we can
 * get the correct return values.
 * \param definition A convenience reference that is set to the name definition
 * of the name or funcName given. This can be used to set the definition of a
 * node. Should not be used if the NodeTrueProperties struct has isDefined set
 * to false.
 */
DECISION_API NodeTrueProperties d_semantic_get_node_properties(
    struct _sheet *sheet, const char *name, size_t lineNum,
    const char *funcName, NameDefinition *definition);

/**
 * \fn bool d_semantic_is_execution_node(NodeTrueProperties trueProperties)
 * \brief Given the properties of a node, decide if it's an execution node or
 * not.
 *
 * \return If this theoretical node is an execution node.
 *
 * \param trueProperties The properties to query.
 */
DECISION_API bool
d_semantic_is_execution_node(NodeTrueProperties trueProperties);

/**
 * \fn void d_semantic_free_true_properties(NodeTrueProperties trueProperties)
 * \brief Free malloc'd elements of a NodeTrueProperties object.
 *
 * \param trueProperties The object to free elements of.
 */
DECISION_API void
d_semantic_free_true_properties(NodeTrueProperties trueProperties);

/**
 * \fn void d_semantic_scan_properties(Sheet *sheet, SyntaxNode *root)
 * \brief Sets the properties of the sheet, given the syntax tree.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
DECISION_API void d_semantic_scan_properties(struct _sheet *sheet,
                                             struct _syntaxNode *root);

/**
 * \fn void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root)
 * \brief Sets the nodes of the sheet, given the syntax tree.
 *
 * **NOTE:** This function also sets the connections between the nodes.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
DECISION_API void d_semantic_scan_nodes(struct _sheet *sheet,
                                        struct _syntaxNode *root);

/**
 * \fn void d_semantic_reduce_types(Sheet *sheet)
 * \brief Take the connections of a sheet which may have "vague" connections,
 * and reduce them to unique types with the information we have.
 *
 * e.g. If `Multiply` has at least one `Float` input, the output must be a
 * `Float`.
 *
 * \param sheet The sheet to reduce the types on.
 */
DECISION_API void d_semantic_reduce_types(struct _sheet *sheet);

/**
 * \fn void d_semantic_detect_loops(Sheet *sheet)
 * \brief After a sheet has been connected in `d_semantic_scan_nodes`, go
 * through the sheet and see if there are any loops.
 *
 * "Loops are bad, and should be given coal at christmas."
 *
 * In retrospect, this was a bad quote. Instead, give it something that won't
 * cause climate change, like a really cheap sticker.
 *
 * While we are here, we also check to see if there are any redundant nodes.
 *
 * \param sheet The connected sheet to check for loops.
 */
DECISION_API void d_semantic_detect_loops(struct _sheet *sheet);

/**
 * \fn void d_semantic_scan(Sheet *sheet, SyntaxNode *root)
 * \brief Perform Semantic Analysis on a syntax tree.
 *
 * \param sheet The sheet to put everything into.
 * \param root The *valid* syntax tree to scan everything from.
 */
DECISION_API void d_semantic_scan(struct _sheet *sheet,
                                  struct _syntaxNode *root);

#endif // DSEMANTIC_H
