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
 * \file dsheet.h
 * \brief This header contains definitions for Sheets in Decision.
 */

#ifndef DSHEET_H
#define DSHEET_H

#include "dcfg.h"
#include "dlex.h"
#include "dlink.h"
#include "dsemantic.h"
#include "dtype.h"
#include "dvm.h"
#include <stdbool.h>

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from later on. */
struct _sheet;

/* Forward declaration of the SheetNode struct from later on. */
struct _sheetNode;

/**
 * \struct _insToLink
 * \brief A struct to describe which instructions needs to be replaced with
 * linked pointers.
 *
 * \typedef struct _insToLink InstructionToLink
 */
typedef struct _insToLink {
    size_t ins;  ///< Index of the LOADUI instruction in the text section.
    size_t link; ///< Index of the LinkMeta structure in the LinkMetaList.
} InstructionToLink;

/**
 * \struct _socketMeta
 * \brief Defines the metadata of a socket, i.e. it's type, name, description,
 * etc.
 *
 * \typedef struct _socketMeta SocketMeta
 */
typedef struct _socketMeta {
    const char *name;
    const char *description;

    const DType type;
    const LexData defaultValue; ///< If there is no input wire, use this value.
} SocketMeta;

/**
 * \struct _nodeDefinition
 * \brief Defined a node in Decision, i.e. it's name, what sockets it has, etc.
 *
 * \typedef struct _nodeDefinition NodeDefinition
 */
typedef struct _nodeDefinition {
    const char *name;
    const char *description;

    const SocketMeta *sockets;
    const size_t numSockets;
    const size_t startOutputIndex; ///< Any socket before this index is an
                                   ///< input socket, the rest are output
                                   ///< sockets.
    const bool infiniteInputs;
} NodeDefinition;

/**
 * \struct _nodeSocket
 * \brief A struct for indexing a node's socket.
 *
 * \typedef struct _nodeSocket NodeSocket
 */
typedef struct _nodeSocket {
    size_t nodeIndex;
    size_t socketIndex;
} NodeSocket;

/**
 * \struct _sheetWire
 * \brief A struct for connecting two sockets together, effectively an edge of
 * a graph.
 *
 * \typedef struct _sheetWire SheetWire
 */
typedef struct _sheetWire {
    NodeSocket socketFrom;
    NodeSocket socketTo;
} SheetWire;

/**
 * \struct _sheetNode
 * \brief A struct for storing node data.
 *
 * \typedef struct _sheetNode SheetNode
 */
typedef struct _sheetNode {
    const NodeDefinition *definition;
    const size_t lineNum;

    const DType *reducedTypes;     ///< Needs to be malloc'd, and have as many
                                   ///< elements as sockets. Can be NULL if the
                                   ///< types are the same as in *definition.
    const LexData *literalValues;  ///< Needs to be malloc'd, and have
                                   ///< `startOutputIndex` elements. Can be
                                   ///< NULL if the default values are the same
                                   ///< as in *definition.
    const size_t startOutputIndex; ///< This will by default be the same value
                                   ///< as in the definition, but in the event
                                   ///< that the definition allows for infinite
                                   ///< inputs, this value will change to
                                   ///< signify where the start of the outputs
                                   ///< actually is.

    const NameDefinition nameDefinition; ///< If the node is the getter or
                                         ///< setter of a variable, then this
                                         ///< points to the variable. If the
                                         ///< node is a Define or return node,
                                         ///< this points to the function.
                                         ///< Otherwise, it points to the name
                                         ///< definition of the node.
    
    int *_stackPositions; ///< Used by Code Generation.
} SheetNode;

/**
 * \struct _sheetVariable
 * \brief A struct for storing variable data.
 *
 * \typedef struct _sheetVariable SheetVariable.
 */
typedef struct _sheetVariable {
    const SocketMeta variableMeta;

    const NodeDefinition getterDefinition;

    struct _sheet *sheet;
} SheetVariable;

/**
 * \struct _sheetFunction
 * \brief A struct for storing function data.
 *
 * \typedef struct _sheetFunction SheetFunction.
 */
typedef struct _sheetFunction {
    const NodeDefinition functionDefinition;

    const NodeDefinition defineDefinition;
    const NodeDefinition returnDefinition;

    size_t defineNodeIndex; ///< Used in Semantic Analysis.
    size_t numDefineNodes;  ///< Used in Semantic Analysis.

    size_t lastReturnNodeIndex; ///< Used in Semantic Analysis.
    size_t numReturnNodes;      ///< Used in Semantic Analysis.

    struct _sheet *sheet;
} SheetFunction;

/**
 * \struct _sheet
 * \brief A struct for storing sheet data.
 *
 * \typedef struct _sheet Sheet
 */
typedef struct _sheet {
    const char *filePath;
    const char *includePath; ///< i.e. what was the argument of the Include
                             ///< property that included this sheet. Default
                             ///< value is `NULL`.
    bool hasErrors;

    bool _isCompiled;
    bool _isLinked;

    struct _sheet **includes;
    size_t numIncludes;

    SheetVariable *variables;
    size_t numVariables;
    SheetFunction *functions;
    size_t numFunctions;

    SheetNode *nodes;
    size_t numNodes;

    SheetWire *wires; ///< This list will be stored in lexicographical order,
                      ///< so binary search can be performed on it.
    size_t numWires;

    int startNodeIndex; ///< If this value is `-1`, then no Start node exists.
    size_t numStarts;

    size_t _main; ///< Points to the index of the first instruction of Start,
                  ///< *not* the `RET` instruction one before.

    char *_text;
    size_t _textSize;
    char *_data;
    size_t _dataSize;

    LinkMetaList _link;
    InstructionToLink *_insLinkList;
    size_t _insLinkListSize;

} Sheet;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn size_t d_definition_num_inputs(const NodeDefinition *nodeDef)
 * \brief Get the number of input sockets a definition has.
 *
 * \return The number of input sockets the definition has.
 *
 * \param nodeDef The definition of the node.
 */
DECISION_API size_t d_definition_num_inputs(const NodeDefinition *nodeDef);

/**
 * \fn size_t d_definition_num_outputs(const NodeDefinition *nodeDef)
 * \brief Get the number of output sockets a definition has.
 *
 * \return The number of output sockets the definition has.
 *
 * \param nodeDef The definition of the node.
 */
DECISION_API size_t d_definition_num_outputs(const NodeDefinition *nodeDef);

/**
 * \fn bool d_is_execution_definition(const NodeDefinition *nodeDef)
 * \brief Is the definition an execution definition, i.e. does it have at least
 * one execution socket?
 *
 * \return If the definition is an execution definition.
 *
 * \param nodeDef The definition of the node.
 */
DECISION_API bool d_is_execution_definition(const NodeDefinition *nodeDef);

/**
 * \def bool d_is_node_index_valid(Sheet *sheet, size_t nodeIndex)
 * \brief Given a sheet, does a given node index exist within that sheet?
 *
 * \return If the node index exists in the sheet.
 *
 * \param sheet The sheet to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API bool d_is_node_index_valid(Sheet *sheet, size_t nodeIndex);

/**
 * \fn size_t d_node_num_inputs(Sheet *sheet, size_t nodeIndex)
 * \brief Get the number of input sockets a node has.
 *
 * \return The number of input sockets the node has, 0 if the index is not
 * valid.
 *
 * \param sheet The sheet to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API size_t d_node_num_inputs(Sheet *sheet, size_t nodeIndex);

/**
 * \fn size_t d_node_num_outputs(Sheet *sheet, size_t nodeIndex)
 * \brief Get the number of output sockets a node has.
 *
 * \return The number of output sockets the node has, 0 if the index is not
 * valid.
 *
 * \param sheet The sheet to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API size_t d_node_num_outputs(Sheet *sheet, size_t nodeIndex);

/**
 * \fn size_t d_is_execution_node(Sheet *sheet, size_t nodeIndex)
 * \brief Is the node an execution node, i.e. does it have at least one
 * execution socket?
 *
 * \return If the node is an execution node.
 *
 * \param sheet The sheet to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API bool d_is_execution_node(Sheet *sheet, size_t nodeIndex);

/**
 * \def bool d_is_socket_index_valid(const NodeDefinition *nodeDef,
 *                                   size_t socketIndex)
 * \brief Given a node definition, does a given socket index exist within that
 * node?
 *
 * \return If the socket index exists in the node.
 *
 * \param nodeDef The node definition to query.
 * \param socketIndex The socket index to query.
 */
DECISION_API bool d_is_socket_index_valid(const NodeDefinition *nodeDef,
                                          size_t socketIndex);

/**
 * \fn const NodeDefinition *d_get_node_definition(Sheet *sheet,
 *                                                 size_t *nodeIndex)
 * \brief Given the index of a node, get the definition of the node.
 *
 * \return The definition of the node, or NULL if the index does not exist.
 *
 * \param sheet The sheet the node belongs to.
 * \param nodeIndex The node to get the definition of.
 */
DECISION_API const NodeDefinition *d_get_node_definition(Sheet *sheet,
                                                         size_t nodeIndex);

/**
 * \fn bool d_is_node_socket_valid(Sheet *sheet, NodeSocket nodeSocket)
 * \brief Given a NodeSocket, does it exist in the sheet?
 *
 * \return If the node socket index exists in the given sheet.
 *
 * \param sheet The sheet to query.
 * \param nodeSocket The node socket index to query.
 */
DECISION_API bool d_is_node_socket_valid(Sheet *sheet, NodeSocket nodeSocket);

/**
 * \fn bool d_is_input_socket(Sheet *sheet, NodeSocket socket)
 * \brief Is the given socket an input socket?
 *
 * \return If the socket is an input socket.
 *
 * \param sheet The sheet the socket belongs to.
 * \param socket The socket to query.
 */
DECISION_API bool d_is_input_socket(Sheet *sheet, NodeSocket socket);

/**
 * \fn const SocketMeta d_get_socket_meta(Sheet *sheet, NodeSocket nodeSocket)
 * \brief Get the metadata of a node's socket.
 *
 * \return The socket's metadata.
 *
 * \param sheet The sheet the socket belongs to.
 * \param nodeSocket The socket to get the metadata for.
 */
DECISION_API const SocketMeta d_get_socket_meta(Sheet *sheet,
                                                NodeSocket nodeSocket);

/**
 * \fn short d_wire_cmp(SheetWire wire1, SheetWire wire2)
 * \brief Since wires are stored in lexicographical order, return an integer
 * value stating the equality, or inequality of the wires.
 *
 * \return 0 if wire1 == wire2, > 0 if wire1 > wire2, and < 0 if wire1 < wire2.
 *
 * \param wire1 The first wire.
 * \param wire2 The second wire.
 */
DECISION_API short d_wire_cmp(SheetWire wire1, SheetWire wire2);

/**
 * \fn int d_wire_find_first(Sheet *sheet, NodeSocket socket)
 * \brief Given a socket, find the first wire in a sheet that originates from
 * the given socket.
 *
 * \return The index of the wire, or -1 if it is not found.
 *
 * \param sheet The sheet to search for the wire.
 * \param socket The "from" socket to search for.
 */
DECISION_API int d_wire_find_first(Sheet *sheet, NodeSocket socket);

/**
 * \def IS_WIRE_FROM(sheet, wireIndex, socket)
 * \brief Check if a wire starts from a given socket.
 */
#define IS_WIRE_FROM(sheet, wireIndex, socket)                                 \
    ((wireIndex) >= 0 && (wireIndex) < (sheet)->numWires &&                    \
     (socket).nodeIndex == (sheet)->wires[(wireIndex)].socketFrom.nodeIndex && \
     (socket).socketIndex ==                                                   \
         (sheet)->wires[(wireIndex)].socketFrom.socketIndex)

/**
 * \fn size_t d_socket_num_connections(Sheet *sheet, NodeSocket socket)
 * \brief Get the number of connections via wires a socket has.
 *
 * \return The number of connected wires to a socket.
 *
 * \param sheet The sheet the socket belongs to.
 * \param socket The socket to query.
 */
DECISION_API size_t d_socket_num_connections(Sheet *sheet, NodeSocket socket);

/**
 * \fn bool d_sheet_add_wire(Sheet *sheet, SheetWire wire)
 * \brief Add a wire to a sheet, connecting two sockets.
 *
 * \return If the operation was successful.
 *
 * \param sheet The sheet to add the wire to. Both nodes have to belong to this
 * node.
 * \param wire The wire to add to the sheet.
 */
DECISION_API bool d_sheet_add_wire(Sheet *sheet, SheetWire wire);

/**
 * \fn size_t d_sheet_add_node(Sheet *sheet, SheetNode node)
 * \brief Add a node to a sheet.
 *
 * \return The new node index.
 *
 * \param sheet The sheet to add the node to.
 * \param node The node to add.
 */
DECISION_API size_t d_sheet_add_node(Sheet *sheet, SheetNode node);

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param varMeta The variable metadata to add.
 */
DECISION_API void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta);

/**
 * \fn void d_sheet_add_function(Sheet *sheet, const NodeDefinition funcDef)
 * \brief Add a function to a sheet.
 *
 * \param sheet The sheet to add the function to.
 * \param funcDef The function definition to add.
 */
DECISION_API void d_sheet_add_function(Sheet *sheet,
                                       const NodeDefinition funcDef);

/**
 * \fn bool d_is_subroutine(SheetFunction func)
 * \brief Is the given function a subroutine?
 *
 * \return If the function is a subroutine.
 *
 * \param func The function to query.
 */
DECISION_API bool d_is_subroutine(SheetFunction func);

/**
 * \fn void d_sheet_add_include(Sheet *sheet, Sheet *include)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \param sheet The sheet to add the include to.
 * \param include The sheet to include.
 */
DECISION_API void d_sheet_add_include(Sheet *sheet, Sheet *include);

/**
 * \fn Sheet *d_sheet_add_include_from_path(Sheet *sheet,
 *                                          const char *includePath)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \return A pointer to the sheet that was created from the include path.
 *
 * \param sheet The sheet to add the include to.
 * \param includePath The path from sheet to the sheet being included.
 * Note that this should be equivalent to the argument of the Include property.
 */
DECISION_API Sheet *d_sheet_add_include_from_path(Sheet *sheet,
                                                  const char *includePath);

/**
 * \fn Sheet *d_sheet_create(const char *filePath)
 * \brief Create a malloc'd sheet object.
 *
 * \return The malloc'd sheet object.
 *
 * \param filePath The file where this sheet originated.
 */
DECISION_API Sheet *d_sheet_create(const char *filePath);

/**
 * \fn void d_definition_free(const NodeDefinition nodeDef)
 * \brief Free the malloc'd elements of a NodeDefinition.
 *
 * \param nodeDef The definition whose elements free from memory.
 */
DECISION_API void d_definition_free(const NodeDefinition nodeDef);

/**
 * \fn void d_sheet_free(Sheet *sheet)
 * \brief Free malloc'd memory in a sheet.
 *
 * \param sheet The sheet to free from memory.
 */
DECISION_API void d_sheet_free(Sheet *sheet);

/**
 * \fn void d_variables_dump(SheetVariable *variables, size_t numVariables)
 * \brief Dump the details of an array of variables to `stdout`.
 *
 * \param variables The array of variables.
 * \param numVariables The number of variables in the array.
 */
DECISION_API void d_variables_dump(SheetVariable *variables,
                                   size_t numVariables);

/**
 * \fn void d_functions_dump(SheetFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of functions to `stdout`.
 *
 * \param functions The array of functions.
 * \param numFunctions The number of functions in the array.
 */
DECISION_API void d_functions_dump(SheetFunction *functions,
                                   size_t numFunctions);

/**
 * \fn void d_sheet_dump(Sheet *sheet)
 * \brief Dump the contents of a `Sheet` struct to `stdout`.
 *
 * \param sheet The sheet to dump.
 */
DECISION_API void d_sheet_dump(Sheet *sheet);

#endif // DSHEET_H
