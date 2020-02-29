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

#include "dgraph.h"

#include "derror.h"
#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * \fn size_t d_definition_num_inputs(const NodeDefinition *nodeDef)
 * \brief Get the number of input sockets a definition has.
 *
 * \return The number of input sockets the definition has.
 *
 * \param nodeDef The definition of the node.
 */
size_t d_definition_num_inputs(const NodeDefinition *nodeDef) {
    if (nodeDef == NULL) {
        return 0;
    }

    return nodeDef->startOutputIndex;
}

/**
 * \fn size_t d_definition_num_outputs(const NodeDefinition *nodeDef)
 * \brief Get the number of output sockets a definition has.
 *
 * \return The number of output sockets the definition has.
 *
 * \param nodeDef The definition of the node.
 */
size_t d_definition_num_outputs(const NodeDefinition *nodeDef) {
    if (nodeDef == NULL) {
        return 0;
    }

    return nodeDef->numSockets - nodeDef->startOutputIndex;
}

/**
 * \fn bool d_is_execution_definition(const NodeDefinition *nodeDef)
 * \brief Is the definition an execution definition, i.e. does it have at least
 * one execution socket?
 *
 * \return If the definition is an execution definition.
 *
 * \param nodeDef The definition of the node.
 */
bool d_is_execution_definition(const NodeDefinition *nodeDef) {
    if (nodeDef == NULL) {
        return false;
    }

    for (size_t i = 0; i < nodeDef->numSockets; i++) {
        SocketMeta socket = nodeDef->sockets[i];

        if (socket.type == TYPE_EXECUTION) {
            return true;
        }
    }

    return false;
}

/**
 * \fn bool d_is_node_index_valid(Graph graph, size_t nodeIndex)
 * \brief Given a graph, does a given node index exist within that graph?
 *
 * \return If the node index exists in the graph.
 *
 * \param graph The graph to query.
 * \param nodeIndex The node index to query.
 */
bool d_is_node_index_valid(Graph graph, size_t nodeIndex) {
    return nodeIndex < graph.numNodes;
}

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
size_t d_node_num_inputs(Graph graph, size_t nodeIndex) {
    if (!d_is_node_index_valid(graph, nodeIndex)) {
        return 0;
    }

    Node node                     = graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;

    if (nodeDef->infiniteInputs) {
        return node.startOutputIndex;
    } else {
        return d_definition_num_inputs(nodeDef);
    }
}

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
size_t d_node_num_outputs(Graph graph, size_t nodeIndex) {
    if (!d_is_node_index_valid(graph, nodeIndex)) {
        return 0;
    }

    Node node                     = graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;

    return d_definition_num_outputs(nodeDef);
}

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
bool d_is_execution_node(Graph graph, size_t nodeIndex) {
    if (!d_is_node_index_valid(graph, nodeIndex)) {
        return 0;
    }

    Node node                     = graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;

    return d_is_execution_definition(nodeDef);
}

/**
 * \fn bool d_is_socket_index_valid(const NodeDefinition *nodeDef,
 *                                  size_t socketIndex)
 * \brief Given a node definition, does a given socket index exist within that
 * node?
 *
 * \return If the socket index exists in the node.
 *
 * \param nodeDef The node definition to query.
 * \param socketIndex The socket index to query.
 */
bool d_is_socket_index_valid(const NodeDefinition *nodeDef,
                             size_t socketIndex) {
    if (nodeDef == NULL) {
        return false;
    }

    if (nodeDef->infiniteInputs) {
        return true;
    }

    return socketIndex < nodeDef->numSockets;
}

/**
 * \fn const NodeDefinition *d_get_node_definition(Graph graph,
 *                                                 size_t nodeIndex)
 * \brief Given the index of a node, get the definition of the node.
 *
 * \return The definition of the node, or NULL if the index does not exist.
 *
 * \param graph The graph the node belongs to.
 * \param nodeIndex The node to get the definition of.
 */
const NodeDefinition *d_get_node_definition(Graph graph, size_t nodeIndex) {
    if (!d_is_node_index_valid(graph, nodeIndex)) {
        return NULL;
    }

    return graph.nodes[nodeIndex].definition;
}

/**
 * \fn bool d_is_node_socket_valid(Graph graph, NodeSocket socket)
 * \brief Given a NodeSocket, does it exist in the graph?
 *
 * \return If the node socket index exists in the given graph.
 *
 * \param graph The graph to query.
 * \param socket The node socket index to query.
 */
bool d_is_node_socket_valid(Graph graph, NodeSocket socket) {
    size_t nodeIndex = socket.nodeIndex;
    if (!d_is_node_index_valid(graph, socket.nodeIndex)) {
        return false;
    }

    const NodeDefinition *nodeDef = d_get_node_definition(graph, nodeIndex);
    if (nodeDef == NULL) {
        return false;
    }

    size_t socketIndex = socket.socketIndex;
    if (!d_is_socket_index_valid(nodeDef, socketIndex)) {
        return false;
    }

    return true;
}

/**
 * \fn bool d_is_input_socket(Graph graph, NodeSocket socket)
 * \brief Is the given socket an input socket?
 *
 * \return If the socket is an input socket.
 *
 * \param graph The graph the socket belongs to.
 * \param socket The socket to query.
 */
bool d_is_input_socket(Graph graph, NodeSocket socket) {
    if (!d_is_node_socket_valid(graph, socket)) {
        return false;
    }

    Node node                     = graph.nodes[socket.nodeIndex];
    const NodeDefinition *nodeDef = node.definition;

    size_t startOutput = nodeDef->startOutputIndex;

    if (nodeDef->infiniteInputs) {
        startOutput = node.startOutputIndex;
    }

    return (socket.socketIndex < startOutput);
}

/**
 * \fn SocketMeta d_get_socket_meta(Graph graph, NodeSocket nodeSocket)
 * \brief Get the metadata of a node's socket.
 *
 * \return The socket's metadata.
 *
 * \param graph The graph the socket belongs to.
 * \param nodeSocket The socket to get the metadata for.
 */
SocketMeta d_get_socket_meta(Graph graph, NodeSocket nodeSocket) {
    if (!d_is_node_socket_valid(graph, nodeSocket)) {
        const SocketMeta meta = {NULL, NULL, TYPE_NONE, {0}};
        return meta;
    }

    size_t nodeIndex              = nodeSocket.nodeIndex;
    Node node                     = graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;

    size_t socketIndex = nodeSocket.socketIndex;

    if (nodeDef->infiniteInputs) {
        // If we have more inputs to deal with than we expected, convert the
        // socket index we've been given to one that the node definition
        // expects.

        if (socketIndex >= node.startOutputIndex) {
            // This is an output after all of the inputs.
            socketIndex -= (node.startOutputIndex - nodeDef->startOutputIndex);
        } else if (socketIndex >= nodeDef->startOutputIndex) {
            // This is one of the extra inputs.
            socketIndex = nodeDef->startOutputIndex - 1;
        }
    }

    SocketMeta out = nodeDef->sockets[socketIndex];

    // Replace some of the elements with what is stored in the node.

    if (node.reducedTypes != NULL) {
        out.type = node.reducedTypes[nodeSocket.socketIndex];
    }

    if (node.literalValues != NULL) {
        if (nodeSocket.socketIndex < node.startOutputIndex) {
            out.defaultValue = node.literalValues[nodeSocket.socketIndex];
        }
    }

    return out;
}

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
short d_wire_cmp(Wire wire1, Wire wire2) {
    if (wire1.socketFrom.nodeIndex < wire2.socketFrom.nodeIndex) {
        return -1;
    } else if (wire1.socketFrom.nodeIndex > wire2.socketFrom.nodeIndex) {
        return 1;
    } else {
        if (wire1.socketFrom.socketIndex < wire2.socketFrom.socketIndex) {
            return -1;
        } else if (wire1.socketFrom.socketIndex >
                   wire2.socketFrom.socketIndex) {
            return 1;
        } else {
            if (wire1.socketTo.nodeIndex < wire2.socketTo.nodeIndex) {
                return -1;
            } else if (wire1.socketTo.nodeIndex < wire2.socketTo.nodeIndex) {
                return 1;
            } else {
                if (wire1.socketTo.socketIndex < wire2.socketTo.socketIndex) {
                    return -1;
                } else if (wire1.socketTo.socketIndex >
                           wire2.socketTo.socketIndex) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

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
int d_wire_find_first(Graph graph, NodeSocket socket) {
    int left   = 0;
    int right  = graph.numWires - 1;
    int middle = (left + right) / 2;

    bool found = false;

    while (left <= right) {
        middle = (left + right) / 2;

        short cmp = 0;
        Wire wire = graph.wires[middle];

        if (socket.nodeIndex < wire.socketFrom.nodeIndex) {
            cmp = -1;
        } else if (socket.nodeIndex > wire.socketFrom.nodeIndex) {
            cmp = 1;
        } else {
            if (socket.socketIndex < wire.socketFrom.socketIndex) {
                cmp = -1;
            } else if (socket.socketIndex > wire.socketFrom.socketIndex) {
                cmp = 1;
            }
        }

        if (cmp > 0) {
            left = middle + 1;
        } else if (cmp < 0) {
            right = middle - 1;
        } else {
            found = true;
            break;
        }
    }

    if (!found) {
        return -1;
    }

    // We found *a* wire with the correct from socket, so we just need to go
    // backwards until we find the *first*.
    Wire wire = graph.wires[middle];

    while (socket.nodeIndex == wire.socketFrom.nodeIndex &&
           socket.socketIndex == wire.socketFrom.socketIndex) {
        middle--;

        if (middle < 0) {
            break;
        } else {
            wire = graph.wires[middle];
        }
    }

    return middle + 1;
}

/**
 * \fn size_t d_socket_num_connections(Graph graph, NodeSocket socket)
 * \brief Get the number of connections via wires a socket has.
 *
 * \return The number of connected wires to a socket.
 *
 * \param graph The graph the socket belongs to.
 * \param socket The socket to query.
 */
size_t d_socket_num_connections(Graph graph, NodeSocket socket) {
    int first = d_wire_find_first(graph, socket);

    if (first < 0) {
        return 0;
    }

    int index = first;

    while (IS_WIRE_FROM(graph, index, socket)) {
        index++;
    }

    return (size_t)(index - first);
}

/**
 * \fn static void add_edge(Graph *graph, Wire wire, const char *filePath)
 * \brief Add an edge to a graph. Detect "too many connections" errors in the
 * process.
 *
 * \param graph The graph to add the edge to.
 * \param wire The edge to add.
 * \param filePath In case we error, say where we errored from.
 */
static void add_edge(Graph *graph, Wire wire, const char *filePath) {
    if (graph == NULL) {
        return;
    }

    // We can't just add the wire to the end of the list here. The list is
    // being stored in lexicographical order, so we need to insert it into the
    // correct position.
    if (graph->wires == NULL) {
        graph->numWires = 1;
        graph->wires    = d_malloc(sizeof(Wire));
        *(graph->wires) = wire;
    } else {
        // Use binary insertion, since the list should be sorted!
        int left   = 0;
        int right  = (int)graph->numWires - 1;
        int middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            short cmp = d_wire_cmp(wire, graph->wires[middle]);

            if (cmp > 0) {
                left = middle + 1;
            } else if (cmp < 0) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (d_wire_cmp(wire, graph->wires[middle]) > 0) {
            middle++;
        }

        graph->numWires++;
        graph->wires = d_realloc(graph->wires, graph->numWires * sizeof(Wire));

        if (middle < (int)graph->numWires - 1) {
            memmove(graph->wires + middle + 1, graph->wires + middle,
                    (graph->numWires - middle - 1) * sizeof(Wire));
        }

        graph->wires[middle] = wire;
    }

    size_t fromNodeIndex = wire.socketFrom.nodeIndex;
    size_t lineNum       = graph->nodes[fromNodeIndex].lineNum;

    // Build up a string of the connection's line numbers, in case we error.
    char connLineNums[MAX_ERROR_SIZE];
    size_t lineNumIndex = 0;

    int wireStart = d_wire_find_first(*graph, wire.socketFrom);
    int wireIndex = wireStart;

    while (IS_WIRE_FROM(*graph, wireIndex, wire.socketFrom)) {
        size_t connNodeIndex = graph->wires[wireIndex].socketTo.nodeIndex;

        if (d_is_node_index_valid(*graph, connNodeIndex)) {
            size_t connLineNum = graph->nodes[connNodeIndex].lineNum;

            if (wireIndex > wireStart) {
                sprintf(connLineNums + lineNumIndex, ", ");
                lineNumIndex += 2;

                if (lineNumIndex >= MAX_ERROR_SIZE) {
                    break;
                }
            }

            sprintf(connLineNums + lineNumIndex, "%zu", connLineNum);

            while (connLineNums[lineNumIndex] != 0 &&
                   lineNumIndex < MAX_ERROR_SIZE) {
                lineNumIndex++;
            }

            if (lineNumIndex >= MAX_ERROR_SIZE) {
                break;
            }
        }

        wireIndex++;
    }

    size_t numConnections = d_socket_num_connections(*graph, wire.socketFrom);
    SocketMeta meta       = d_get_socket_meta(*graph, wire.socketFrom);
    bool isInputSocket    = d_is_input_socket(*graph, wire.socketFrom);
    DType socketType      = meta.type;

    // If the socket is non-execution, an input socket, and we have more than
    // one connection...
    if (numConnections > 1 && socketType != TYPE_EXECUTION && isInputSocket) {
        ERROR_COMPILER(filePath, lineNum, true,
                       "Input non-execution socket (#%zu) has more than one "
                       "connection (has %zu, on lines %s)",
                       wire.socketFrom.socketIndex, numConnections,
                       connLineNums);
    }

    // If the socket is execution, an output socket, and we have more than one
    // connection...
    else if (numConnections > 1 && socketType == TYPE_EXECUTION &&
             !isInputSocket) {
        ERROR_COMPILER(
            filePath, lineNum, true,
            "Output execution socket (#%zu) has more than one connection "
            "(has %zu, on lines %s)",
            wire.socketFrom.socketIndex, numConnections, connLineNums);
    }
}

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
bool d_graph_add_wire(Graph *graph, Wire wire, const char *filePath) {
    NodeSocket from = wire.socketFrom;
    NodeSocket to   = wire.socketTo;

    if (!d_is_node_socket_valid(*graph, from)) {
        return false;
    }

    if (!d_is_node_socket_valid(*graph, to)) {
        return false;
    }

    Wire oppositeDir;
    oppositeDir.socketFrom = to;
    oppositeDir.socketTo   = from;

    add_edge(graph, wire, filePath);
    add_edge(graph, oppositeDir, filePath);

    // We need to check that the data types of both ends are the same!
    SocketMeta fromMeta = d_get_socket_meta(*graph, from);
    SocketMeta toMeta   = d_get_socket_meta(*graph, to);

    if ((fromMeta.type & toMeta.type) == TYPE_NONE) {
        Node fromNode = graph->nodes[from.nodeIndex];
        Node toNode   = graph->nodes[to.nodeIndex];

        const NodeDefinition *fromDef = fromNode.definition;
        const NodeDefinition *toDef   = toNode.definition;

        ERROR_COMPILER(filePath, toNode.lineNum, true,
                       "Wire data type mismatch between socket of type %s "
                       "(Output %zu/%zu of node %s on line %zu) and socket "
                       "of type %s (Input %zu/%zu of node %s on line %zu)",
                       d_type_name(fromMeta.type), from.socketIndex + 1,
                       fromDef->numSockets, fromDef->name, fromNode.lineNum,
                       d_type_name(toMeta.type), to.socketIndex + 1,
                       toDef->numSockets, toDef->name, toNode.lineNum);
    }

    return true;
}

/**
 * \fn size_t d_graph_add_node(Graph *graph, Node node)
 * \brief Add a node to a graph.
 *
 * \return The new node index.
 *
 * \param graph The graph to add the node to.
 * \param node The node to add.
 */
size_t d_graph_add_node(Graph *graph, Node node) {
    node._stackPositions = NULL;

    const size_t newNumNodes = graph->numNodes + 1;
    const size_t newAlloc    = newNumNodes * sizeof(Node);

    if (graph->nodes == NULL) {
        graph->nodes = d_calloc(newNumNodes, sizeof(Node));
    } else {
        graph->nodes = d_realloc(graph->nodes, newAlloc);
    }

    graph->numNodes = newNumNodes;

    graph->nodes[newNumNodes - 1] = node;

    return newNumNodes - 1;
}

/**
 * \fn void d_graph_dump(Graph graph)
 * \brief Dump the contents of a Graph to `stdout`.
 *
 * \param graph The graph to dump.
 */
void d_graph_dump(Graph graph) {
    // Dump the nodes.
    printf("# Nodes: %zu\n", graph.numNodes);

    if (graph.nodes != NULL && graph.numNodes > 0) {
        for (size_t i = 0; i < graph.numNodes; i++) {
            Node node                 = graph.nodes[i];
            const NodeDefinition *def = node.definition;

            printf("[%zu] %s (Line %zu)\n", i, def->name, node.lineNum);

            size_t numInputs     = def->startOutputIndex;
            size_t maxNumSockets = def->numSockets;

            if (def->infiniteInputs) {
                numInputs = node.startOutputIndex;
                maxNumSockets +=
                    (node.startOutputIndex - def->startOutputIndex);
            }

            if (def->sockets != NULL && def->numSockets > 0) {
                for (size_t j = 0; j < maxNumSockets; j++) {
                    NodeSocket socket;
                    socket.nodeIndex   = i;
                    socket.socketIndex = j;
                    SocketMeta meta    = d_get_socket_meta(graph, socket);

                    bool isInput = (j < numInputs);

                    printf("\t[%zu|%s] %s (%s)\n", j,
                           (isInput) ? "Input" : "Output", meta.name,
                           d_type_name(meta.type));
                }
            }
        }
    }

    // Dump the wires.
    printf("\n# Wires: %zu\n", graph.numWires);

    if (graph.wires != NULL && graph.numWires > 0) {
        for (size_t i = 0; i < graph.numWires; i++) {
            Wire wire = graph.wires[i];

            NodeSocket from = wire.socketFrom;
            NodeSocket to   = wire.socketTo;

            printf("%zu: Node %zu Socket %zu\t->\tNode %zu Socket %zu\n", i,
                   from.nodeIndex, from.socketIndex, to.nodeIndex,
                   to.socketIndex);
        }
    }
}

/**
 * \fn void d_graph_free(Graph *graph)
 * \brief Free the malloc'd elements of a Graph structure. Note that you may
 * also need to free the Graph itself if you malloc'd it.
 *
 * \param graph The graph to free.
 */
void d_graph_free(Graph *graph) {
    if (graph == NULL) {
        return;
    }

    for (size_t i = 0; i < graph->numNodes; i++) {
        Node node = graph->nodes[i];

        if (node.reducedTypes != NULL) {
            free(node.reducedTypes);
        }

        if (node.literalValues != NULL) {
            free(node.literalValues);
        }

        if (node._stackPositions != NULL) {
            free(node._stackPositions);
        }
    }

    free(graph->nodes);
    graph->nodes    = NULL;
    graph->numNodes = 0;

    if (graph->wires != NULL) {
        free(graph->wires);
    }

    graph->wires    = NULL;
    graph->numWires = 0;
}

/**
 * \fn void d_definition_free(const NodeDefinition nodeDef, bool freeSocketStrs)
 * \brief Free the malloc'd elements of a NodeDefinition.
 *
 * \param nodeDef The definition whose elements free from memory.
 * \param freeSocketStrs If true, free the names and descriptions of sockets.
 */
void d_definition_free(const NodeDefinition nodeDef, bool freeSocketStrs) {
    if (nodeDef.name != NULL) {
        free((char *)nodeDef.name);
    }

    if (nodeDef.description != NULL) {
        free((char *)nodeDef.description);
    }

    if (nodeDef.sockets != NULL) {
        if (freeSocketStrs) {
            for (size_t i = 0; i < nodeDef.numSockets; i++) {
                SocketMeta meta = nodeDef.sockets[i];

                free((char *)meta.name);
                free((char *)meta.description);
            }
        }

        free((SocketMeta *)nodeDef.sockets);
    }
}