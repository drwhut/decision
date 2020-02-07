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

#include "dcfunc.h"
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
#define LIST_PUSH(array, arrayType, numCurrentItems, newItem)               \
    size_t newNumItems = (numCurrentItems) + 1;                             \
    if (newNumItems == 1)                                                   \
        array = (arrayType *)d_malloc(sizeof(arrayType));                   \
    else                                                                    \
        array =                                                             \
            (arrayType *)d_realloc(array, newNumItems * sizeof(arrayType)); \
    array[numCurrentItems++] = newItem;

/**
 * \fn size_t d_node_num_inputs(NodeDefinition *nodeDef)
 * \brief Get the number of input sockets a node has.
 *
 * \return The number of input sockets the node has.
 *
 * \param nodeDef The definition of the node.
 */
size_t d_node_num_inputs(NodeDefinition *nodeDef) {
    if (nodeDef == NULL) {
        return 0;
    }

    return nodeDef->startOutputIndex;
}

/**
 * \fn size_t d_node_num_outputs(NodeDefinition *nodeDef)
 * \brief Get the number of output sockets a node has.
 *
 * \return The number of output sockets the node has.
 *
 * \param nodeDef The definition of the node.
 */
size_t d_node_num_outputs(NodeDefinition *nodeDef) {
    if (nodeDef == NULL) {
        return 0;
    }

    return nodeDef->numSockets - nodeDef->startOutputIndex;
}

/**
 * \fn bool d_is_execution_node(NodeDefinition *nodeDef)
 * \brief Is the node an execution node, i.e. does it have at least one
 * execution socket?
 *
 * \return If the node is an execution node.
 *
 * \param nodeDef The definition of the node.
 */
bool d_is_execution_node(NodeDefinition *nodeDef) {
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
 * \def bool d_is_node_index_valid(Sheet *sheet, size_t nodeIndex)
 * \brief Given a sheet, does a given node index exist within that sheet?
 *
 * \returns If the node index exists in the sheet.
 *
 * \param sheet The sheet to query.
 * \param nodeIndex The node index to query.
 */
bool d_is_node_index_valid(Sheet *sheet, size_t nodeIndex) {
    if (sheet == NULL) {
        return false;
    }

    return nodeIndex < sheet->numNodes;
}

/**
 * \def bool d_is_socket_index_valid(NodeDefinition *nodeDef,
 *                                   size_t socketIndex)
 * \brief Given a node definition, does a given socket index exist within that
 * node?
 *
 * \return If the socket index exists in the node.
 *
 * \param nodeDef The node definition to query.
 * \param socketIndex The socket index to query.
 */
bool d_is_socket_index_valid(NodeDefinition *nodeDef, size_t socketIndex) {
    if (nodeDef == NULL) {
        return false;
    }

    return socketIndex < nodeDef->numSockets;
}

/**
 * \fn NodeDefinition *d_get_node_definition(Sheet *sheet, size_t nodeIndex)
 * \brief Given the index of a node, get the definition of the node.
 *
 * \return The definition of the node, or NULL if the index does not exist.
 *
 * \param sheet The sheet the node belongs to.
 * \param nodeIndex The node to get the definition of.
 */
NodeDefinition *d_get_node_definition(Sheet *sheet, size_t nodeIndex) {
    if (!d_is_node_index_valid(sheet, nodeIndex)) {
        return NULL;
    }

    return sheet->nodes[nodeIndex].definition;
}

/**
 * \fn bool d_is_node_socket_valid(Sheet *sheet, NodeSocket nodeSocket)
 * \brief Given a NodeSocket, does it exist in the sheet?
 *
 * \return If the node socket index exists in the given sheet.
 *
 * \param sheet The sheet to query.
 * \param nodeSocket The node socket index to query.
 */
bool d_is_node_socket_valid(Sheet *sheet, NodeSocket nodeSocket) {
    size_t nodeIndex = nodeSocket.nodeIndex;
    if (!d_is_node_index_valid(sheet, nodeSocket.nodeIndex)) {
        return false;
    }

    NodeDefinition *nodeDef = d_get_node_definition(sheet, nodeIndex);
    if (nodeDef == NULL) {
        return false;
    }

    size_t socketIndex = nodeSocket.socketIndex;
    if (!d_is_socket_index_valid(nodeDef, socketIndex)) {
        return false;
    }

    return true;
}

/**
 * \fn bool d_is_input_socket(Sheet *sheet, NodeSocket socket)
 * \brief Is the given socket an input socket?
 *
 * \return If the socket is an input socket.
 *
 * \param sheet The sheet the socket belongs to.
 * \param socket The socket to query.
 */
bool d_is_input_socket(Sheet *sheet, NodeSocket socket) {
    if (!d_is_node_socket_valid(sheet, socket)) {
        return false;
    }

    NodeDefinition *nodeDef = d_get_node_definition(sheet, socket.nodeIndex);
    return socket.socketIndex < nodeDef->startOutputIndex;
}

/**
 * \fn SocketMeta *d_get_socket_meta(Sheet *sheet, NodeSocket nodeSocket)
 * \brief Get the metadata of a node's socket.
 *
 * \return The socket's metadata, or NULL if the index does not exist.
 *
 * \param sheet The sheet the socket belongs to.
 * \param nodeSocket The socket to get the metadata for.
 */
SocketMeta *d_get_socket_meta(Sheet *sheet, NodeSocket nodeSocket) {
    if (!d_is_node_socket_valid(sheet, nodeSocket)) {
        return NULL;
    }

    size_t nodeIndex        = nodeSocket.nodeIndex;
    NodeDefinition *nodeDef = d_get_node_definition(sheet, nodeIndex);

    size_t socketIndex = nodeSocket.socketIndex;
    return nodeDef->sockets + socketIndex;
}

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
short d_wire_cmp(SheetWire wire1, SheetWire wire2) {
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
 * \fn int d_wire_find_first(Sheet *sheet, NodeSocket socket)
 * \brief Given a socket, find the first wire in a sheet that originates from
 * the given socket.
 *
 * \returns The index of the wire, or -1 if it is not found.
 *
 * \param sheet The sheet to search for the wire.
 * \param socket The "from" socket to search for.
 */
int d_wire_find_first(Sheet *sheet, NodeSocket socket) {
    int left  = 0;
    int right = sheet->numWires - 1;
    int middle;

    bool found = false;

    while (left <= right) {
        middle = (left + right) / 2;

        short cmp      = 0;
        SheetWire wire = sheet->wires[middle];

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
    SheetWire wire = sheet->wires[middle];

    while (socket.nodeIndex == wire.socketFrom.nodeIndex &&
           socket.socketIndex == wire.socketFrom.socketIndex) {
        middle--;

        wire = sheet->wires[middle];
    }

    return middle + 1;
}

/**
 * \fn size_t d_socket_num_connections(Sheet *sheet, NodeSocket socket)
 * \brief Get the number of connections via wires a socket has.
 *
 * \return The number of connected wires to a socket.
 *
 * \param sheet The sheet the socket belongs to.
 * \param socket The socket to query.
 */
size_t d_socket_num_connections(Sheet *sheet, NodeSocket socket) {
    int first = d_wire_find_first(sheet, socket);

    if (first < 0) {
        return 0;
    }

    int index = first;

    while (IS_WIRE_FROM(sheet, index, socket)) {
        index++;
    }

    return (size_t)(index - first);
}

/**
 * \fn static void add_edge(Sheet *sheet, SheetWire wire)
 * \brief Add an edge to a sheet. Detect "too many connections" errors in the
 * process.
 *
 * \param sheet The sheet to add the edge to.
 * \param wire The edge to add.
 */
static void add_edge(Sheet *sheet, SheetWire wire) {
    if (sheet == NULL) {
        return;
    }

    // We can't just add the wire to the end of the list here. The list is
    // being stored in lexicographical order, so we need to insert it into the
    // correct position.
    if (sheet->wires == NULL) {
        sheet->numWires = 1;
        sheet->wires    = (SheetWire *)d_malloc(sizeof(SheetWire));
        *(sheet->wires) = wire;
    } else {
        // Use binary insertion, since the list should be sorted!
        size_t left   = 0;
        size_t right  = sheet->numWires - 1;
        size_t middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            short cmp = d_wire_cmp(wire, sheet->wires[middle]);

            if (cmp > 0) {
                left = middle + 1;
            } else if (cmp < 0) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (d_wire_cmp(wire, sheet->wires[middle]) > 0) {
            middle++;
        }

        sheet->numWires++;
        sheet->wires = (SheetWire *)d_realloc(
            sheet->wires, sheet->numWires * sizeof(SheetWire));

        if (middle < sheet->numWires - 1) {
            memmove(sheet->wires + middle + 1, sheet->wires + middle,
                    (sheet->numWires - middle - 1) * sizeof(SheetWire));
        }

        sheet->wires[middle] = wire;
    }

    const char *filePath = NULL;
    size_t lineNum       = 0;

    size_t fromNodeIndex = wire.socketFrom.nodeIndex;

    if (sheet != NULL && d_is_node_index_valid(sheet, fromNodeIndex)) {
        filePath = sheet->filePath;
        lineNum  = sheet->nodes[fromNodeIndex].lineNum;
    }

    // Build up a string of the connection's line numbers, in case we error.
    char connLineNums[MAX_ERROR_SIZE];
    size_t lineNumIndex = 0;

    int wireStart = d_wire_find_first(sheet, wire.socketFrom);
    int wireIndex = wireStart;

    while (IS_WIRE_FROM(sheet, wireIndex, wire.socketFrom)) {
        size_t connNodeIndex = sheet->wires[wireIndex].socketTo.nodeIndex;

        if (d_is_node_index_valid(sheet, connNodeIndex)) {
            size_t connLineNum = sheet->nodes[connNodeIndex].lineNum;

            if (wireIndex > wireStart) {
#ifdef DECISION_SAFE_FUNCTIONS
                sprintf_s(connLineNums + lineNumIndex,
                          MAX_ERROR_SIZE - lineNumIndex, ", ");
#else
                sprintf(connLineNums + lineNumIndex, ", ");
#endif // DECISION_SAFE_FUNCTIONS
                lineNumIndex += 2;

                if (lineNumIndex >= MAX_ERROR_SIZE) {
                    break;
                }
            }

#ifdef DECISION_SAFE_FUNCTIONS
            sprintf_s(connLineNums + lineNumIndex,
                      MAX_ERROR_SIZE - lineNumIndex, "%zu", connLineNum);
#else
            sprintf(connLineNums + lineNumIndex, "%zu", connLineNum);
#endif // DECISION_SAFE_FUNCTIONS

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

    size_t numConnections = d_socket_num_connections(sheet, wire.socketFrom);
    SocketMeta *meta      = d_get_socket_meta(sheet, wire.socketFrom);
    bool isInputSocket    = d_is_input_socket(sheet, wire.socketFrom);
    DType socketType      = meta->type;

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
 * \fn bool d_sheet_add_wire(Sheet *sheet, SheetWire wire)
 * \brief Add a wire to a sheet, connecting two sockets.
 *
 * \return If the operation was successful.
 *
 * \param sheet The sheet to add the wire to. Both nodes have to belong to this
 * node.
 * \param wire The wire to add to the sheet.
 */
bool d_sheet_add_wire(Sheet *sheet, SheetWire wire) {
    NodeSocket from = wire.socketFrom;
    NodeSocket to   = wire.socketTo;

    if (!d_is_node_socket_valid(sheet, from)) {
        return false;
    }

    if (!d_is_node_socket_valid(sheet, to)) {
        return false;
    }

    SheetWire oppositeDir;
    oppositeDir.socketFrom = to;
    oppositeDir.socketTo   = from;

    add_edge(sheet, wire);
    add_edge(sheet, oppositeDir);

    // We need to check that the data types of both ends are the same!
    SocketMeta *fromMeta = d_get_socket_meta(sheet, from);
    SocketMeta *toMeta   = d_get_socket_meta(sheet, to);

    if ((fromMeta->type & toMeta->type) == TYPE_NONE) {
        SheetNode fromNode = sheet->nodes[from.nodeIndex];
        SheetNode toNode   = sheet->nodes[to.nodeIndex];

        NodeDefinition *fromDef = fromNode.definition;
        NodeDefinition *toDef   = toNode.definition;

        ERROR_COMPILER(sheet->filePath, toNode.lineNum, true,
                       "Wire data type mismatch between socket of type %s "
                       "(Output %zu/%zu of node %s on line %zu) and socket "
                       "of type %s (Input %zu/%zu of node %s on line %zu)",
                       d_type_name(fromMeta->type), from.socketIndex + 1,
                       fromDef->numSockets, fromDef->name, fromNode.lineNum,
                       d_type_name(toMeta->type), to.socketIndex + 1,
                       toDef->numSockets, toDef->name, toNode.lineNum);
    }

    return true;
}

/**
 * \fn size_t d_sheet_add_node(Sheet *sheet, SheetNode node)
 * \brief Add a node to a sheet.
 *
 * \return The new node index.
 *
 * \param sheet The sheet to add the node to.
 * \param node The node to add.
 */
size_t d_sheet_add_node(Sheet *sheet, SheetNode node) {
    LIST_PUSH(sheet->nodes, SheetNode, sheet->numNodes, node)
    return sheet->numNodes - 1;
}

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, SocketMeta varMeta)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param varMeta The variable metadata to add.
 */
void d_sheet_add_variable(Sheet *sheet, SocketMeta varMeta) {
    SheetVariable variable;
    variable.variableMeta = varMeta;
    variable.sheet        = sheet;

    LIST_PUSH(sheet->variables, SheetVariable, sheet->numVariables, variable)
}

/**
 * \fn void d_sheet_add_function(Sheet *sheet, NodeDefinition funcDef)
 * \brief Add a function to a sheet.
 *
 * \param sheet The sheet to add the function to.
 * \param funcDef The function definition to add.
 */
void d_sheet_add_function(Sheet *sheet, NodeDefinition funcDef) {
    SheetFunction func;
    func.functionDefinition = funcDef;
    func.defineNode         = NULL;
    func.numDefineNodes     = 0;
    func.lastReturnNode     = NULL;
    func.numReturnNodes     = 0;

    LIST_PUSH(sheet->functions, SheetFunction, sheet->numFunctions, func)
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
Sheet *d_sheet_add_include_from_path(Sheet *sheet, const char *includePath) {
    Sheet *includeSheet = NULL;

    // TODO: Implement standard library paths as well.

    // If the current sheet was accessed from a different directory,
    // we need to stick to that directory for this include as well.
    // i.e. If we ran: decision ../main.dc, we need to include ../include.dc.
    // If main.dc and include.dc were in the same directory.

    // Copy the file path of the current sheet.
    const size_t filePathLen = strlen(sheet->filePath);
    char *dir                = (char *)d_malloc(filePathLen + 1);
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

    if ((int)lastSeperator >= 0) {
        // Concatenate the dir string (with the NULL inserted) with the contents
        // of the literal string.
        const size_t newPathLength =
            (size_t)lastSeperator + 1 + strlen(includePath);
        dir = (char *)d_realloc(dir, newPathLength + 1);

#ifdef DECISION_SAFE_FUNCTIONS
        strcat_s(dir, newPathLength + 1, includePath);
#else
        strcat(dir, includePath);
#endif // DECISION_SAFE_FUNCTIONS

        includeSheet = d_load_file((const char *)dir);
    }
    // If there isn't either character, we don't need to worry about
    // changing the directory.
    else {
        includeSheet = d_load_file(includePath);
    }

    d_sheet_add_include(sheet, includeSheet);

    free(dir);

    // Set the includePath property of the included sheet, as we need to save
    // that value, instead of the directory, for if we run the sheet including
    // this sheet from a different working directory.
    const size_t includePathLen = strlen(includePath);
    char *cpyIncludePath        = (char *)d_malloc(includePathLen + 1);
    memcpy(cpyIncludePath, includePath, includePathLen + 1);

    includeSheet->includePath = (const char *)cpyIncludePath;

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
    Sheet *sheet = (Sheet *)d_malloc(sizeof(Sheet));

    // Copy the file path string into heap memory so we know for sure we can
    // free it later.
    size_t filePathLen = strlen(filePath);
    char *cpyFilePath  = (char *)d_malloc(filePathLen + 1);
    memcpy(cpyFilePath, filePath, filePathLen + 1);

    sheet->filePath         = (const char *)cpyFilePath;
    sheet->includePath      = NULL;
    sheet->hasErrors        = false;
    sheet->_isCompiled      = false;
    sheet->_isLinked        = false;
    sheet->includes         = NULL;
    sheet->numIncludes      = 0;
    sheet->variables        = NULL;
    sheet->numVariables     = 0;
    sheet->functions        = NULL;
    sheet->numFunctions     = 0;
    sheet->nodes            = NULL;
    sheet->numNodes         = 0;
    sheet->wires            = NULL;
    sheet->numWires         = 0;
    sheet->startNodeIndex   = 0;
    sheet->numStarts        = 0;
    sheet->_main            = 0;
    sheet->_text            = NULL;
    sheet->_textSize        = 0;
    sheet->_data            = NULL;
    sheet->_dataSize        = 0;
    sheet->_link            = d_link_new_meta_list();
    sheet->_insLinkList     = NULL;
    sheet->_insLinkListSize = 0;

    return sheet;
}

/**
 * \fn void d_definition_free(NodeDefinition *nodeDef)
 * \brief Free a malloc'd definition from memory.
 *
 * \param nodeDef The definition to free from memory.
 */
void d_definition_free(NodeDefinition *nodeDef) {
    if (nodeDef != NULL) {
        if (nodeDef->name != NULL) {
            free(nodeDef->name);
            nodeDef->name = NULL;
        }

        if (nodeDef->description != NULL) {
            free(nodeDef->description);
            nodeDef->description = NULL;
        }

        if (nodeDef->sockets != NULL) {
            free(nodeDef->sockets);
            nodeDef->sockets    = NULL;
            nodeDef->numSockets = 0;
        }

        free(nodeDef);
    }
}

/**
 * \fn void d_sheet_free(Sheet *sheet)
 * \brief Free malloc'd memory in a sheet.
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

        if (sheet->variables != NULL) {
            free(sheet->variables);
            sheet->variables    = NULL;
            sheet->numVariables = 0;
        }

        if (sheet->nodes != NULL) {
            free(sheet->nodes);
            sheet->nodes    = NULL;
            sheet->numNodes = 0;
        }

        if (sheet->functions != NULL) {
            for (size_t i = 0; i < sheet->numFunctions; i++) {
                SheetFunction func = sheet->functions[i];

                // Free the definition.
                d_definition_free(&(func.functionDefinition));
            }

            free(sheet->functions);
            sheet->functions    = NULL;
            sheet->numFunctions = 0;
        }

        if (sheet->includes != NULL) {
            for (size_t i = 0; i < sheet->numIncludes; i++) {
                Sheet *include = sheet->includes[i];
                d_sheet_free(include);
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
            SheetFunction function  = functions[i];
            NodeDefinition *funcDef = &(function.functionDefinition);

            size_t numInputs  = d_node_num_inputs(funcDef);
            size_t numOutputs = d_node_num_outputs(funcDef);

            printf("\tFunction %s is %s with %zu arguments:\n", funcDef->name,
                   ((d_is_execution_node(funcDef)) ? "an EXECUTION"
                                                   : "a NON-EXECUTION"),
                   numInputs);

            if (funcDef->sockets != NULL && numInputs > 0) {
                for (size_t j = 0; j < numInputs; j++) {
                    SocketMeta arg = funcDef->sockets[j];

                    printf("\t\tArgument %s (%s) is of type %s with default "
                           "value ",
                           arg.name, arg.description, d_type_name(arg.type));
                    switch (arg.type) {
                        case TYPE_INT:
                            printf("%" DINT_PRINTF_d,
                                   arg.defaultValue.integerValue);
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

            if (funcDef->sockets != NULL && numOutputs > 0) {
                for (size_t j = 0; j < numOutputs; j++) {
                    SocketMeta ret =
                        funcDef->sockets[funcDef->startOutputIndex + j];

                    printf("\t\tReturn %s (%s) is of type %s\n", ret.name,
                           ret.description, d_type_name(ret.type));
                }
            }
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

    // Dump the nodes.
    printf("# Nodes: %zu\n", sheet->numNodes);

    if (sheet->nodes != NULL && sheet->numNodes > 0) {
        for (size_t i = 0; i < sheet->numNodes; i++) {
            SheetNode node          = sheet->nodes[i];
            NodeDefinition *nodeDef = node.definition;

            printf(
                "Node #%zd named %s is %s node on line %zu with %zu sockets\n",
                i, nodeDef->name,
                d_is_execution_node(nodeDef) ? "an execution"
                                             : "a non-execution",
                node.lineNum, nodeDef->numSockets);

            if (nodeDef->sockets != NULL && nodeDef->numSockets > 0) {
                for (size_t j = 0; j < nodeDef->numSockets; j++) {
                    SocketMeta socket = nodeDef->sockets[j];

                    printf("\n\tSocket #%zd is of type %s, isInput = "
                           "%d\n",
                           j, d_type_name(socket.type),
                           j < nodeDef->startOutputIndex);
                }
            }

            printf("\n");
        }
    }

    // Dump the wires.
    printf("# Wires: %zu\n", sheet->numWires);

    if (sheet->wires != NULL && sheet->numWires > 0) {
        for (size_t i = 0; i < sheet->numWires; i++) {
            SheetWire wire = sheet->wires[i];

            NodeSocket from = wire.socketFrom;
            NodeSocket to   = wire.socketTo;

            printf("Node %zu\tSocket %zu\t->\tNode %zu\tSocket %zu\n",
                   from.nodeIndex, from.socketIndex, to.nodeIndex,
                   to.socketIndex);
        }

        printf("\n");
    }
}
