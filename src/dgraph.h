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
 * \file dgraph.h
 * \brief This header contains the definitions of Decision graphs, and their
 * nodes and wires.
 */

#ifndef DGRAPH_H
#define DGRAPH_H

#include "dcfg.h"
#include "dname.h"
#include "dtype.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

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

    DType type;
    LexData defaultValue; ///< If there is no input wire, use this value.
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
    size_t numSockets;
    size_t startOutputIndex; ///< Any socket before this index is an
                             ///< input socket, the rest are output
                             ///< sockets.
    bool infiniteInputs;
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
} Wire;

/**
 * \struct _sheetNode
 * \brief A struct for storing node data.
 *
 * \typedef struct _sheetNode SheetNode
 */
typedef struct _sheetNode {
    const NodeDefinition *definition;
    size_t lineNum;

    DType *reducedTypes;     ///< Needs to be malloc'd, and have as many
                             ///< elements as sockets. Can be NULL if the types
                             ///< are the same as in *definition.
    LexData *literalValues;  ///< Needs to be malloc'd, and have
                             ///< `startOutputIndex` elements. Can be NULL if
                             ///< the default values are the same as in
                             ///< *definition.
    size_t startOutputIndex; ///< This will by default be the same value as in
                             ///< the definition, but in the event that the
                             ///< definition allows for infinite inputs, this
                             ///< value will change to signify where the start
                             ///< of the outputs actually is.

    NameDefinition nameDefinition; ///< If the node is the getter or setter of
                                   ///< a variable, then this points to the
                                   ///< variable. If the node is a Define or
                                   ///< Return node, this points to the
                                   ///< function. Otherwise, it points to the
                                   ///< name definition of the node.

    int *_stackPositions; ///< Used by Code Generation.
} Node;

/**
 * \struct Graph
 * \brief A collection of nodes and wires. Essentially a representation of the
 * code that the user wrote.
 *
 * \typedef struct _graph Graph
 */
typedef struct _graph {
    Node *nodes;
    size_t numNodes;

    Wire *wires; ///< This list will be stored in lexicographical order,
                 ///< so binary search can be performed on it.
    size_t numWires;
} Graph;

/**
 * \def EMPTY_GRAPH
 * \brief A graph wil no nodes or wires defined.
 */
#define EMPTY_GRAPH      \
    (Graph) {            \
        NULL, 0, NULL, 0 \
    }

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
 * \def bool d_is_node_index_valid(Graph graph, size_t nodeIndex)
 * \brief Given a graph, does a given node index exist within that graph?
 *
 * \return If the node index exists in the graph.
 *
 * \param graph The graph to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API bool d_is_node_index_valid(Graph graph, size_t nodeIndex);

/**
 * \fn size_t d_node_num_inputs(Graph graph, size_t nodeIndex)
 * \brief Get the number of input sockets a node has.
 *
 * \return The number of input sockets the node has, 0 if the index is not
 * valid.
 *
 * \param graph The graph to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API size_t d_node_num_inputs(Graph graph, size_t nodeIndex);

/**
 * \fn size_t d_node_num_outputs(Graph graph, size_t nodeIndex)
 * \brief Get the number of output sockets a node has.
 *
 * \return The number of output sockets the node has, 0 if the index is not
 * valid.
 *
 * \param graph The graph to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API size_t d_node_num_outputs(Graph graph, size_t nodeIndex);

/**
 * \fn size_t d_is_execution_node(Graph graph, size_t nodeIndex)
 * \brief Is the node an execution node, i.e. does it have at least one
 * execution socket?
 *
 * \return If the node is an execution node.
 *
 * \param graph The graph to query.
 * \param nodeIndex The node index to query.
 */
DECISION_API bool d_is_execution_node(Graph graph, size_t nodeIndex);

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
 * \fn const NodeDefinition *d_get_node_definition(Graph graph,
 *                                                 size_t *nodeIndex)
 * \brief Given the index of a node, get the definition of the node.
 *
 * \return The definition of the node, or NULL if the index does not exist.
 *
 * \param graph The graph the node belongs to.
 * \param nodeIndex The node to get the definition of.
 */
DECISION_API const NodeDefinition *d_get_node_definition(Graph graph,
                                                         size_t nodeIndex);

/**
 * \fn bool d_is_node_socket_valid(Graph graph, NodeSocket socket)
 * \brief Given a NodeSocket, does it exist in the graph?
 *
 * \return If the node socket index exists in the given graph.
 *
 * \param graph The graph to query.
 * \param socket The node socket index to query.
 */
DECISION_API bool d_is_node_socket_valid(Graph graph, NodeSocket socket);

/**
 * \fn bool d_is_input_socket(Graph graph, NodeSocket socket)
 * \brief Is the given socket an input socket?
 *
 * \return If the socket is an input socket.
 *
 * \param graph The graph the socket belongs to.
 * \param socket The socket to query.
 */
DECISION_API bool d_is_input_socket(Graph graph, NodeSocket socket);

/**
 * \fn const SocketMeta d_get_socket_meta(Graph graph, NodeSocket nodeSocket)
 * \brief Get the metadata of a node's socket.
 *
 * \return The socket's metadata.
 *
 * \param graph The graph the socket belongs to.
 * \param nodeSocket The socket to get the metadata for.
 */
DECISION_API const SocketMeta d_get_socket_meta(Graph graph,
                                                NodeSocket nodeSocket);

/**
 * \fn short d_wire_cmp(Wire wire1, Wire wire2)
 * \brief Since wires are stored in lexicographical order, return an integer
 * value stating the equality, or inequality of the wires.
 *
 * \return 0 if wire1 == wire2, > 0 if wire1 > wire2, and < 0 if wire1 < wire2.
 *
 * \param wire1 The first wire.
 * \param wire2 The second wire.
 */
DECISION_API short d_wire_cmp(Wire wire1, Wire wire2);

/**
 * \fn int d_wire_find_first(Graph graph, NodeSocket socket)
 * \brief Given a socket, find the first wire in a sheet that originates from
 * the given socket.
 *
 * \return The index of the wire, or -1 if it is not found.
 *
 * \param graph The graph to search for the wire.
 * \param socket The "from" socket to search for.
 */
DECISION_API int d_wire_find_first(Graph graph, NodeSocket socket);

/**
 * \def IS_WIRE_FROM(graph, wireIndex, socket)
 * \brief Check if a wire starts from a given socket.
 */
#define IS_WIRE_FROM(graph, wireIndex, socket)                                 \
    ((wireIndex) >= 0 && (wireIndex) < (int)((graph).numWires) &&             \
     (socket).nodeIndex == (graph).wires[(wireIndex)].socketFrom.nodeIndex && \
     (socket).socketIndex ==                                                   \
         (graph).wires[(wireIndex)].socketFrom.socketIndex)

/**
 * \fn size_t d_socket_num_connections(Graph graph, NodeSocket socket)
 * \brief Get the number of connections via wires a socket has.
 *
 * \return The number of connected wires to a socket.
 *
 * \param graph The graph the socket belongs to.
 * \param socket The socket to query.
 */
DECISION_API size_t d_socket_num_connections(Graph graph, NodeSocket socket);

/**
 * \fn bool d_graph_add_wire(Graph *graph, Wire wire, const char *filePath)
 * \brief Add a wire to a sheet, connecting two sockets.
 *
 * \return If the operation was successful.
 *
 * \param graph The graph to add the wire to. Both nodes have to belong to this
 * graph.
 * \param wire The wire to add to the sheet.
 * \param filePath In case we error, say where we errored from.
 */
DECISION_API bool d_graph_add_wire(Graph *graph, Wire wire,
                                   const char *filePath);

/**
 * \fn size_t d_graph_add_node(Sheet *sheet, Node node)
 * \brief Add a node to a graph.
 *
 * \return The new node index.
 *
 * \param graph The graph to add the node to.
 * \param node The node to add.
 */
DECISION_API size_t d_graph_add_node(Graph *graph, Node node);

/**
 * \fn void d_graph_dump(Graph graph)
 * \brief Dump the contents of a Graph to `stdout`.
 * 
 * \param graph The graph to dump.
 */
DECISION_API void d_graph_dump(Graph graph);

/**
 * \fn void d_graph_free(Graph *graph)
 * \brief Free the malloc'd elements of a Graph structure. Note that you may
 * also need to free the Graph itself if you malloc'd it.
 *
 * \param graph The graph to free.
 */
DECISION_API void d_graph_free(Graph *graph);

/**
 * \fn void d_definition_free(const NodeDefinition nodeDef)
 * \brief Free the malloc'd elements of a NodeDefinition.
 *
 * \param nodeDef The definition whose elements free from memory.
 */
DECISION_API void d_definition_free(const NodeDefinition nodeDef);

#endif // DGRAPH_H