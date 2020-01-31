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
 * \file dsyntax.h
 * \brief This header deals with checking the syntax of the program, and
 * generating a syntax tree from lexical tokens.
 */

#ifndef DSYNTAX_H
#define DSYNTAX_H

#include <stdbool.h>
#include "dcfg.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the LexToken struct from dlex.h */
struct _lexToken;

/* Forward declaration of the LexStream struct from dlex.h */
struct _lexStream;

/**
 * \enum _syntaxDefinition
 * \brief An enum for each kind of syntax definition.
 *
 * \typedef enum _syntaxDefinition SyntaxDefinition
 */
typedef enum _syntaxDefinition {
    STX_TOKEN, ///< A single element related to a lexical token.

    // The rest are all defined in dparse.c
    // NOTE: Try to keep in order!
    STX_lineIdentifier,          // = 1
    STX_listOfLineIdentifier,    // = 2
    STX_dataType,                // = 3
    STX_literal,                 // = 4
    STX_oneeos,                  // = 5
    STX_eos,                     // = 6
    STX_argument,                // = 7
    STX_propertyArgument,        // = 8
    STX_listOfArguments,         // = 9
    STX_listOfPropertyArguments, // = 10
    STX_call,                    // = 11
    STX_propertyCall,            // = 12
    STX_expression,              // = 13
    STX_propertyExpression,      // = 14
    STX_statement,               // = 15
    STX_propertyStatement,       // = 16
    STX_generalStatement,        // = 17
    STX_program                  // = 18
} SyntaxDefinition;

/**
 * \struct _syntaxNode
 * \brief A syntax node in the syntax tree. It has 2 pointers: One for the
 * child, and one for the next sibling.
 *
 * \typedef struct _syntaxNode SyntaxNode
 */
typedef struct _syntaxNode {
    SyntaxDefinition definition;
    struct _lexToken *info; ///< Only defined if `definition == STX_TOKEN`.
    struct _syntaxNode *child;
    struct _syntaxNode *sibling;
    size_t onLineNum;
} SyntaxNode;

/**
 * \struct _syntaxResult
 * \brief A structure for the results given by definitions.
 *
 * \typedef struct _syntaxResult SyntaxResult
 */
typedef struct _syntaxResult {
    bool success;
    SyntaxNode *node;
} SyntaxResult;

/**
 * \struct _syntaxSearchResult
 * \brief A structure for returning all occurances of a syntax definition.
 *
 * \typedef struct _syntaxSearchResult SyntaxSearchResult
 */
typedef struct _syntaxSearchResult {
    SyntaxNode **occurances;
    size_t numOccurances;
} SyntaxSearchResult;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn SyntaxNode *d_syntax_create_node(SyntaxDefinition d, LexToken *info,
 *                                      size_t line)
 * \brief Create a malloc'd syntax node with default nulled family pointers.
 *
 * \return A pointer to a malloc'd `SyntaxNode`.
 *
 * \param d What definition does this syntax node have?
 * \param info The corresponding lexical token, if any.
 * \param line The line number this syntax definition appears in.
 */
DECISION_API SyntaxNode *
d_syntax_create_node(SyntaxDefinition d, struct _lexToken *info, size_t line);

/**
 * \fn SyntaxNode *d_syntax_last_sibling(SyntaxNode *node)
 * \brief Get the pointer to the last of a chain of children with the same
 * common parent.
 *
 * \return The last sibling.
 *
 * \param node The node to find the last sibling of.
 */
DECISION_API SyntaxNode *d_syntax_last_sibling(SyntaxNode *node);

/**
 * \fn void d_syntax_add_child(SyntaxNode *parent, SyntaxNode *child)
 * \brief Add a child node to a parent node.
 *
 * \param parent The parent node to attach the child onto.
 * \param child The child node to add onto the parent.
 */
DECISION_API void d_syntax_add_child(SyntaxNode *parent, SyntaxNode *child);

/**
 * \fn size_t d_syntax_get_num_children(SyntaxNode *parent)
 * \brief Get how many children a node has.
 *
 * \return The number of children `parent` has.
 *
 * \param parent The node to query.
 */
DECISION_API size_t d_syntax_get_num_children(SyntaxNode *parent);

/**
 * \fn SyntaxNode *d_syntax_get_child_by_index(SyntaxNode *parent, size_t index)
 * \brief Get a node's child from it's index.
 *
 * \return The `index`th child of the parent. `NULL` if the index is out of
 * range.
 *
 * \param parent The parent node to get the child from.
 * \param index The index of the child we want.
 */
DECISION_API SyntaxNode *d_syntax_get_child_by_index(SyntaxNode *parent,
                                                     size_t index);

/**
 * \fn SyntaxNode *d_syntax_get_child_by_definition(SyntaxNode *parent,
 *                                                  SyntaxDefinition definition)
 * \brief Find the first occurance of a child with a specific definition.
 *
 * \return The first child of parent with the given definition. NULL if there
 * is none.
 *
 * \param parent The parent to get the child from.
 * \param definition The definition we want from the child.
 */
DECISION_API SyntaxNode *
d_syntax_get_child_by_definition(SyntaxNode *parent,
                                 SyntaxDefinition definition);

/**
 * \fn SyntaxSearchResult d_syntax_get_all_nodes_with(
 *     SyntaxNode *root,
 *     SyntaxDefinition definition,
 *     bool traverseChildrenOfFound)
 * \brief Return all occurances of a tree with a given definition as a malloc'd
 * SyntaxSearchResult.
 *
 * \return A malloc'd SyntaxSearchResult.
 *
 * \param root The root node to search from.
 * \param definition The definition we want our found nodes to have.
 * \param traverseChildrenOfFound If we find a node that we want, should w
 * also traverse the children of that found node?
 */
DECISION_API SyntaxSearchResult
d_syntax_get_all_nodes_with(SyntaxNode *root, SyntaxDefinition definition,
                            bool traverseChildrenOfFound);

/**
 * \fn void d_syntax_dump_tree_raw(SyntaxNode *root, int n)
 * \brief Dump the contents of a tree recursively.
 *
 * \param root The root node to start from.
 * \param n The "level" of the recursion, so we can print the tree with proper
 * indentation.
 */
DECISION_API void d_syntax_dump_tree_raw(SyntaxNode *root, int n);

/**
 * \fn void d_syntax_dump_tree(SyntaxNode *root)
 * \brief Dump the contents of a tree recursively.
 *
 * \param root The root node to start from.
 */
DECISION_API void d_syntax_dump_tree(SyntaxNode *root);

/**
 * \fn void d_syntax_free_tree(SyntaxNode *root)
 * \brief Free all the nodes from a syntax tree.
 *
 * \param root The root node of the tree.
 */
DECISION_API void d_syntax_free_tree(SyntaxNode *root);

/**
 * \fn void d_syntax_free_results(SyntaxSearchResult results)
 * \brief Free search results from memory.
 *
 * \param results The results to free.
 */
DECISION_API void d_syntax_free_results(SyntaxSearchResult results);

/**
 * \fn SyntaxResult d_syntax_parse(LexStream stream, const char *filePath)
 * \brief Parse a lexical stream, and generate a syntax tree.
 *
 * \return The malloc'd root node of the syntax tree, and whether the parsing
 * was successful or not.
 *
 * \param stream The stream to parse from.
 * \param filePath In case we error, say what the file path was.
 */
DECISION_API SyntaxResult d_syntax_parse(struct _lexStream stream,
                                         const char *filePath);

#endif // DSYNTAX_H
