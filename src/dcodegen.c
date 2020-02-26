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

#include "dcodegen.h"

#include "dcfunc.h"
#include "dcore.h"
#include "decision.h"
#include "derror.h"
#include "dlink.h"
#include "dmalloc.h"
#include "dsemantic.h"

#include <stdlib.h>
#include <string.h>

/* Functions to set the stack index of a socket at a given time. */

static int get_stack_index(BuildContext *context, NodeSocket socket) {
    Node node = context->graph.nodes[socket.nodeIndex];

    if (node._stackPositions == NULL) {
        return -1;
    }

    return node._stackPositions[socket.socketIndex];
}

static void set_stack_index(BuildContext *context, NodeSocket socket,
                            int index) {
    Node *node = context->graph.nodes + socket.nodeIndex;

    if (node->_stackPositions == NULL) {
        size_t numSockets =
            d_node_num_inputs(context->graph, socket.nodeIndex) +
            d_node_num_outputs(context->graph, socket.nodeIndex);

        node->_stackPositions = d_malloc(numSockets * sizeof(int));

        for (size_t i = 0; i < numSockets; i++) {
            node->_stackPositions[i] = -1;
        }
    }

    node->_stackPositions[socket.socketIndex] = index;
}

/*
=== LINKING FUNCTIONS =====================================
*/

/**
 * \fn void d_add_link_to_ins(BuildContext *context, BCode *bcode,
 *                            size_t insIndex, LinkMeta linkMeta,
 *                            size_t *indexInList, bool *wasDuplicate)
 * \brief Add a future link to an instruction in bytecode.
 *
 * \param context The context needed to store the link.
 * \param bcode The bytecode containing the instruction to link.
 * \param insIndex The index of the instruction to edit when linking is taking
 * place.
 * \param linkMeta The link metadata.
 * \param indexInList Stores in the reference the index of the new metadata
 * in the list.
 * \param wasDuplicate Sets the reference to true if a matching linkMeta was
 * already found in the array, false otherwise.
 */
void d_add_link_to_ins(BuildContext *context, BCode *bcode, size_t insIndex,
                       LinkMeta linkMeta, size_t *indexInList,
                       bool *wasDuplicate) {
    VERBOSE(5, "Linking instruction %zu to link of type %u and name %s...\n",
            insIndex, linkMeta.type, linkMeta.name);

    // Firstly, does the link already exist in the list? We don't want
    // duplicates - we want instructions to point to the same thing.
    bool duplicateFound = false;
    size_t linkIndex    = 0;

    for (size_t i = 0; i < context->linkMetaList.size; i++) {
        LinkMeta testAgainst = context->linkMetaList.list[i];

        if (linkMeta.type == testAgainst.type &&
            strcmp(linkMeta.name, testAgainst.name) == 0) {
            duplicateFound = true;
            linkIndex      = i;
            break;
        }
    }

    *wasDuplicate = duplicateFound;

    if (!duplicateFound) {
        // Before we store the LinkMeta object, we first need to copy the name.
        // This prevents the name being free'd from 2 seperate places: once
        // from a node name, and the other being in the LinkMetaList.
        size_t nameLen = strlen(linkMeta.name);
        char *cpyName  = d_calloc(nameLen + 1, sizeof(char));
        memcpy(cpyName, linkMeta.name, nameLen);
        cpyName[nameLen] = 0;
        linkMeta.name    = (const char *)cpyName;

        // Store the link metadata into the build context if it's not already
        // in.
        d_link_meta_list_push(&(context->linkMetaList), linkMeta);
        linkIndex = context->linkMetaList.size - 1;
    }

    // Let the outer function know where the linkMeta is in the list, but note
    // that it may not point to the last element in the list, if a copy of
    // linkMeta was already found in the list.
    *indexInList = linkIndex;

    // Now we need the BCode to know of this.
    if (bcode != NULL) {
        InstructionToLink itl;
        itl.ins  = insIndex;
        itl.link = linkIndex;

        if (bcode->linkList != NULL && bcode->linkListSize > 0) {
            bcode->linkList =
                d_realloc(bcode->linkList, (bcode->linkListSize + 1) *
                                               sizeof(InstructionToLink));
        } else {
            bcode->linkList =
                d_calloc((bcode->linkListSize + 1), sizeof(InstructionToLink));
        }

        // linkListSize hasn't changed yet!
        bcode->linkList[bcode->linkListSize] = itl;

        // I spoke too soon...
        bcode->linkListSize += 1;
    }
}

/**
 * \fn char *d_allocate_from_data_section(BuildContext *context,
 *                                        size_t size, size_t *index)
 * \brief Allocate a number of bytes from the data section for some data.
 * That data could be a string literal, variable, etc.
 *
 * \return A pointer to the start of the new allocation. Returns NULL if size
 * is 0.
 *
 * \param context The context that contains the built-up data section.
 * \param size The size of the allocation in bytes.
 * \param index Overwrites with the index of the start of the allocation from
 * the start of the data section.
 */
char *d_allocate_from_data_section(BuildContext *context, size_t size,
                                   size_t *index) {
    VERBOSE(5, "Allocating %zu bytes for data...\n", size);
    size_t oldSize = context->dataSectionSize;
    size_t newSize = oldSize + size;

    if (newSize > 0) {
        context->dataSectionSize = newSize;

        if (context->dataSection != NULL && context->dataSectionSize > 0) {
            context->dataSection = d_realloc(context->dataSection, newSize);
            *index               = oldSize;
            return context->dataSection + oldSize;
        } else {
            context->dataSection = d_calloc(newSize, sizeof(char));
            *index               = 0;
            return context->dataSection;
        }
    } else
        return NULL;
}

/**
 * \fn size_t d_allocate_string_literal_in_data(BuildContext *context,
 *                                              BCode *linkCode,
 *                                              size_t insIndex,
 *                                              char *stringLiteral)
 * \brief Given a string literal, allocate memory from the data section to
 * store the literal.
 *
 * If there is a duplicate string literal found, the links to string literal
 * passed are pointed to the literal already stored in the data section.
 * **NOTE:** If the string was a duplicate, it is freed!
 *
 * \return The index of the data section where the string literal starts.
 *
 * \param context The context containing the built-up data section.
 * \param linkCode The bytecode to link to the string literal in the data
 * section.
 * \param insIndex The index of the LOADUI instruction to replace when linking
 * is taking place.
 * \param stringLiteral The string literal to place in the data section.
 */
size_t d_allocate_string_literal_in_data(BuildContext *context, BCode *linkCode,
                                         size_t insIndex, char *stringLiteral) {
    VERBOSE(5, "Allocating string \"%s\" to data...\n", stringLiteral);

    // We need this code to remember to link to the string literal.
    LinkMeta meta =
        d_link_new_meta(LINK_DATA_STRING_LITERAL, stringLiteral, NULL);

    // Now we add the link metadata to the list in the context, but we
    // want to look out for duplicate string literals that may already
    // have been allocated.
    size_t indexInList;
    bool wasDuplicate = false;
    d_add_link_to_ins(context, linkCode, insIndex, meta, &indexInList,
                      &wasDuplicate);

    size_t dataIndex;

    // If a copy was found, then we don't need to worry about setting
    // the link's pointer. Only if it's a completely new string literal
    // do we need to allocate data from the data section to copy it
    // into.
    if (!wasDuplicate) {
        size_t strLiteralSize = strlen(stringLiteral);

        // The +1 is for the NULL at the end of the string.
        char *dataPtr = d_allocate_from_data_section(
            context, strLiteralSize + 1, &dataIndex);

        memcpy(dataPtr, stringLiteral, strLiteralSize + 1);

        // We need the linker to know where this string literal is in
        // the data section.
        context->linkMetaList.list[indexInList]._ptr = (char *)dataIndex;
    } else {
        // We just need to return the index of where the original string is.
        dataIndex = (size_t)context->linkMetaList.list[indexInList]._ptr;
    }

    // Now that the string has been copied into the LinkMetaList (see
    // d_add_link_to_ins), we can free the string literal!
    // free(stringLiteral);

    return dataIndex;
}

/**
 * \fn void d_allocate_variable(BuildContext *context, SheetVariable *variable,
 *                              size_t size, size_t indexInLinkMeta)
 * \brief Allocate memory from the data section to store a variable.
 *
 * \param context The context containing the built-up data section.
 * \param variable The variable data.
 * \param size How many bytes to allocate for the variable.
 * \param indexInLinkMeta The index of the variable's LinkMeta entry in the
 * build context.
 */
void d_allocate_variable(BuildContext *context, SheetVariable *variable,
                         size_t size, size_t indexInLinkMeta) {
    const SocketMeta varMeta = variable->variableMeta;

    VERBOSE(5, "Allocating variable %s to data...\n", varMeta.name);
    size_t dataIndex;

    char *dataPtr = d_allocate_from_data_section(context, size, &dataIndex);

    // We need to initialize the value we've just allocated with the
    // default value of the variable.
    switch (varMeta.type) {
        case TYPE_INT:;
            memcpy(dataPtr, &(varMeta.defaultValue.integerValue), size);
            break;
        case TYPE_FLOAT:;
            memcpy(dataPtr, &(varMeta.defaultValue.floatValue), size);
            break;

        // Store the default value of the string as a string literal in the
        // data section. Then when linking, malloc a new string of the same
        // length, copy the string literal over, and place the pointer in
        // the data section.
        case TYPE_STRING:;
            // We just need to add a custom InstructionToLink to tell the
            // string variable where the default value is in the data.
            LinkMeta strDefaultValueLink = d_link_new_meta(
                LINK_VARIABLE_STRING_DEFAULT_VALUE, varMeta.name, variable);

            size_t defValMetaIndex;
            bool metaDuplicate;

            // We're not trying to replace any bytecode to load this
            // default value.
            d_add_link_to_ins(context, NULL, 0, strDefaultValueLink,
                              &defValMetaIndex, &metaDuplicate);

            if (!metaDuplicate) {
                // Add the string literal to the data section, and link it.
                size_t dataStart = d_allocate_string_literal_in_data(
                    context, NULL, 0, varMeta.defaultValue.stringValue);

                context->linkMetaList.list[defValMetaIndex]._ptr =
                    (char *)dataStart;
            }
            break;

        case TYPE_BOOL:;
            *dataPtr = varMeta.defaultValue.booleanValue;
            break;
        default:
            break;
    }

    // We need the linker to know where this variable is in the data
    // section.
    context->linkMetaList.list[indexInLinkMeta]._ptr = (char *)dataIndex;
}

/*
=== GENERATOR FUNCTIONS ===================================
*/

/**
 * \fn BCode d_push_literal(BuildContext *context, NodeSocket socket
 *                          bool cvtFloat)
 * \brief Generate bytecode to push a literal onto the stack.
 *
 * \return Bytecode to push the socket's literal onto the stack.
 *
 * \param context The context needed to build the bytecode.
 * \param socket The socket of the literal to push onto the stack.
 * \param cvtFloat Converts the literal to a float if possible.
 */
BCode d_push_literal(BuildContext *context, NodeSocket socket, bool cvtFloat) {
    SocketMeta meta = d_get_socket_meta(context->graph, socket);

    VERBOSE(
        5,
        "Generating bytecode to get literal value of type %s from node %s...\n",
        d_type_name(meta.type), meta.name);

    // Even for floats, we want the integer representation of them.
    dint intLiteral = meta.defaultValue.integerValue;

    BCode out = d_bytecode_ins(OP_PUSHF);
    d_bytecode_set_fimmediate(out, 1, (fimmediate_t)intLiteral);

    if (meta.type == TYPE_INT && cvtFloat) {
        BCode cvtf = d_bytecode_ins(OP_CVTF);
        d_concat_bytecode(&out, &cvtf);
        d_free_bytecode(&cvtf);
    } else if (meta.type == TYPE_STRING) {
        // The literal string needs to go into the data section.
        d_allocate_string_literal_in_data(context, &out, 0,
                                          meta.defaultValue.stringValue);
    }

    // Set the socket's stack index so we know where the value lives.
    set_stack_index(context, socket, ++context->stackTop);

    return out;
}

/**
 * \fn BCode d_push_variable(BuildContext *context, size_t nodeIndex)
 * \brief Given a node that is the getter of a variable, generate bytecode to
 * push the value of the variable onto the stack.
 *
 * \return Bytecode to push the variable's value onto the stack.
 *
 * \param context The context needed to build the bytecode.
 * \param nodeIndex The index of the node that is the getter of a variable.
 */
BCode d_push_variable(BuildContext *context, size_t nodeIndex) {
    Node node                     = context->graph.nodes[nodeIndex];
    SheetVariable *variable       = node.nameDefinition.definition.variable;
    const SocketMeta variableMeta = variable->variableMeta;

    VERBOSE(5, "Generating bytecode to get the value of variable %s...\n",
            variableMeta.name);

    // If the variable is a boolean, the variable is 1 byte instead of
    // sizeof(dint).
    DIns opcode = (variableMeta.type == TYPE_BOOL) ? OP_DEREFBI : OP_DEREFI;

    BCode out = d_bytecode_ins(opcode);
    d_bytecode_set_fimmediate(out, 1, 0); // The address will be linked later.

    // Set the socket's stack index so we know where the value lives.
    NodeSocket outSocket;
    outSocket.nodeIndex   = nodeIndex;
    outSocket.socketIndex = 0;
    set_stack_index(context, outSocket, ++context->stackTop);

    LinkType linkType = (variableMeta.type == TYPE_STRING)
                            ? LINK_VARIABLE_POINTER
                            : LINK_VARIABLE;

    // We need this code to remember to link to the variable.
    LinkMeta meta = d_link_new_meta(linkType, variableMeta.name, variable);

    // Now we add the link metadata to the list in the context, but we
    // want to look out for duplicate variables that may have already been
    // allocated.
    size_t metaIndexInList;
    bool wasDuplicate = false;
    d_add_link_to_ins(context, &out, 0, meta, &metaIndexInList, &wasDuplicate);

    // Set the first instruction to represent activating the node.
    if (context->debug) {
        InsNodeInfo varNodeInfo;
        varNodeInfo.node = nodeIndex;

        d_debug_add_node_info(&(out.debugInfo), 0, varNodeInfo);
    }

    return out;
}

/**
 * \fn BCode d_push_input(BuildContext *context, NodeSocket socket,
 *                        bool forceFloat)
 * \brief Given an input socket, generate bytecode to push the value of the
 * input to the top of the stack.
 *
 * \return Bytecode to push the input's value onto the stack.
 *
 * \param context The context needed to build the bytecode.
 * \param socket The input socket to get the value for.
 * \param forceFloat Force integers to be converted to floats.
 */
BCode d_push_input(BuildContext *context, NodeSocket socket, bool forceFloat) {
    SocketMeta meta = d_get_socket_meta(context->graph, socket);

    VERBOSE(5,
            "Generating bytecode to get the value of input socket %zd of node "
            "%zd...\n",
            socket.socketIndex, socket.nodeIndex);

    BCode out = d_malloc_bytecode(0);

    if (d_is_input_socket(context->graph, socket) &&
        meta.type != TYPE_EXECUTION) {

        // We could use d_socket_num_connections here, but if there is a
        // connection we will need the wire index.
        int wireIndex = d_wire_find_first(context->graph, socket);

        if (wireIndex < 0) {
            // The socket input is a literal.
            out = d_push_literal(context, socket, forceFloat);
        } else {
            // The socket has a connected socket.
            NodeSocket connSocket = context->graph.wires[wireIndex].socketTo;
            Node connNode         = context->graph.nodes[connSocket.nodeIndex];

            const NodeDefinition *connDef = connNode.definition;

            // Determine if the node is an execution node.
            bool isExecutionNode = d_is_execution_definition(connDef);

            // Set to false if you can determine socket->_stackIndex.
            bool checkIfOnTop = true;

            // Set to true if you want the value to be pushed on top, even if
            // it already is on top.
            bool forceOnTop = false;

            // Has this output not already been generated, or has it been poped
            // off?
            int connIndex = get_stack_index(context, connSocket);

            if (connIndex < 0 || connIndex > context->stackTop) {

                // If this is the Define node, we need to push the argument.
                if (strcmp(connDef->name, "Define") == 0) {
                    out = d_push_argument(context, connSocket);

                    // Since we're copying the argument, set the socket's
                    // stack index now.
                    set_stack_index(context, socket, context->stackTop);
                    checkIfOnTop = false;
                } else if (!isExecutionNode) {
                    out = d_generate_nonexecution_node(context,
                                                       connSocket.nodeIndex);
                }
            } else if (isExecutionNode) {
                // Semantic Analysis should have confirmed that there
                // are no loops in the program, which implies that the
                // value we want should have already been generated.
                // We just need to get it and push it to the top of the
                // stack like an argument.
                forceOnTop = true;
            } else if (connNode.nameDefinition.type == NAME_VARIABLE) {
                // This getter of a variable has already been used, and the
                // variable's value is somewhere in the stack. But what if
                // the value of the variable has changed?
                out = d_push_variable(context, connSocket.nodeIndex);
            }

            if (checkIfOnTop || forceOnTop) {
                // If the value is not at the top of the stack, make sure it is.
                int inputIndex = get_stack_index(context, connSocket);

                if (!IS_INDEX_TOP(context, inputIndex) || forceOnTop) {
                    BCode get = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(
                        get, 1,
                        (fimmediate_t)STACK_INDEX_TOP(context, inputIndex));
                    d_concat_bytecode(&out, &get);
                    d_free_bytecode(&get);

                    if (!forceOnTop) {
                        inputIndex = context->stackTop;
                        set_stack_index(context, connSocket, inputIndex);
                    }

                    context->stackTop++;
                }

                set_stack_index(context, socket, inputIndex);
            }

            // Say that the first instruction after this bytecode represents
            // sending the value over the wire (in the other direction).
            if (context->debug) {
                Wire forward;
                forward.socketFrom = connSocket;
                forward.socketTo   = socket;

                InsValueInfo valueInfo;
                valueInfo.valueWire  = forward;
                valueInfo.stackIndex = 0;

                d_debug_add_value_info(&(out.debugInfo), out.size, valueInfo);
            }
        }
    }

    return out;
}

/**
 * \fn BCode d_push_node_inputs(BuildContext *context, size_t nodeIndex,
 *                              bool order, bool ignoreLiterals,
 *                              bool forceFloat)
 * \brief Given a node, generate bytecode to push the values of the
 * inputs to the top of the stack.
 *
 * \return Bytecode to push all input's values onto the stack.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The index of the node whose input sockets to generate
 * bytecode for.
 * \param order If true, the inputs are pushed in order, such that the last
 * input is at the top. If false, the inputs are pushed in reverse order, such
 * that the first input is at the top.
 * \param ignoreLiterals Do not generate bytecode for non-float literal inputs.
 * \param forceFloat Force integers to be converted to floats.
 */
BCode d_push_node_inputs(BuildContext *context, size_t nodeIndex, bool order,
                         bool ignoreLiterals, bool forceFloat) {
    const NodeDefinition *nodeDef =
        d_get_node_definition(context->graph, nodeIndex);

    VERBOSE(5, "Generating bytecode to get the inputs for node %s...\n",
            nodeDef->name);

    const size_t numInputs = d_node_num_inputs(context->graph, nodeIndex);

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    BCode out = d_malloc_bytecode(0);

    int start = numInputs - 1;
    int end   = 0;
    int step  = -1;

    if (order) {
        start = 0;
        end   = numInputs - 1;
        step  = 1;
    }

    // Push the inputs in the order we want.
    int i = start;

    bool inLoop = (i >= end);
    if (order) {
        inLoop = (i <= end);
    }

    while (inLoop) {
        socket.socketIndex = i;
        SocketMeta meta    = d_get_socket_meta(context->graph, socket);

        if ((meta.type & TYPE_VAR_ANY) != 0) {
            size_t numCons = d_socket_num_connections(context->graph, socket);
            if (numCons >= 1 || meta.type == TYPE_FLOAT || !ignoreLiterals) {
                BCode input = d_push_input(context, socket, forceFloat);
                d_concat_bytecode(&out, &input);
                d_free_bytecode(&input);
            }
        }

        i += step;

        if (order) {
            inLoop = (i <= end);
        } else {
            inLoop = (i >= end);
        }
    }

    // Now the bytecode for the inputs has been generated, we need to verify
    // that the inputs are in order from the top of the stack. If not, we need
    // to get them there.
    int numCheckedInputs  = 0;
    bool positionsCorrect = true;

    i = end;

    inLoop = (i <= end);
    if (order) {
        inLoop = (i >= end);
    }

    while (inLoop) {
        socket.socketIndex = i;
        SocketMeta meta    = d_get_socket_meta(context->graph, socket);

        if ((meta.type & TYPE_VAR_ANY) != 0) {
            size_t numCons = d_socket_num_connections(context->graph, socket);
            if (numCons >= 1 || meta.type == TYPE_FLOAT || !ignoreLiterals) {
                int socketIndex = get_stack_index(context, socket);
                // Is this input the correct amount from the top?
                if (STACK_INDEX_TOP(context, socketIndex) !=
                    -numCheckedInputs) {
                    positionsCorrect = false;
                    break;
                }
                numCheckedInputs++;
            }
        }

        i -= step;

        if (order) {
            inLoop = (i >= end);
        } else {
            inLoop = (i <= end);
        }
    }

    // If the positions of the inputs are not correct, make them correct.
    if (!positionsCorrect) {
        i = start;

        inLoop = (i >= end);
        if (order) {
            inLoop = (i <= end);
        }

        while (inLoop) {
            socket.socketIndex = i;
            SocketMeta meta    = d_get_socket_meta(context->graph, socket);

            if ((meta.type & TYPE_VAR_ANY) != 0) {
                size_t numCons =
                    d_socket_num_connections(context->graph, socket);
                if (numCons >= 1 || meta.type == TYPE_FLOAT ||
                    !ignoreLiterals) {

                    int socketIndex = get_stack_index(context, socket);

                    BCode get = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(
                        get, 1,
                        (fimmediate_t)STACK_INDEX_TOP(context, socketIndex));
                    d_concat_bytecode(&out, &get);
                    d_free_bytecode(&get);

                    set_stack_index(context, socket, context->stackTop);
                }
            }

            i += step;

            if (order) {
                inLoop = (i <= end);
            } else {
                inLoop = (i >= end);
            }
        }
    }

    return out;
}

/**
 * \fn BCode d_generate_operator(BuildContext *context, size_t nodeIndex,
 *                               DIns opcode, DIns fopcode, DIns fiopcode,
 *                               bool forceFloat)
 * \brief Given an operator node, generate the bytecode for it.
 *
 * \return Bytecode to get the output of an operator.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The index of the operator node to get the result for.
 * \param opcode The operator instruction.
 * \param fopcode The float variant of the instruction.
 * \param fiopcode The full immediate variant of the instruction.
 * \param forceFloat Should the output always be a float?
 */
BCode d_generate_operator(BuildContext *context, size_t nodeIndex, DIns opcode,
                          DIns fopcode, DIns fiopcode, bool forceFloat) {
    const NodeDefinition *nodeDef =
        d_get_node_definition(context->graph, nodeIndex);

    VERBOSE(5, "Generate bytecode for operator %s...\n", nodeDef->name);

    BCode out       = d_malloc_bytecode(0);
    BCode subaction = d_malloc_bytecode(0);

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    // Do we need to convert integer inputs to floats?
    bool convertFloat = forceFloat;

    const size_t numInputs = d_node_num_inputs(context->graph, nodeIndex);

    // If we're not forcing it, we may still need to if any of the inputs are
    // floats.
    if (!convertFloat) {
        for (size_t j = 0; j < numInputs; j++) {
            socket.socketIndex = j;
            SocketMeta meta    = d_get_socket_meta(context->graph, socket);

            if (meta.type == TYPE_FLOAT) {
                convertFloat = true;
                break;
            }
        }
    }

    int initStackTop = context->stackTop;

    socket.socketIndex = 0;
    size_t numCons     = d_socket_num_connections(context->graph, socket);

    // Generate the bytecode for the inputs without literals - but we want to
    // start working from the first input, even if it is a literal.
    // NOTE: If we need to convert inputs to floats, we need to generate all
    // of the inputs, including literals, as we always generate the first
    // input.
    out = d_push_node_inputs(context, nodeIndex, false, !convertFloat,
                             convertFloat);
    if (!convertFloat && numCons == 0) {
        subaction = d_push_literal(context, socket, convertFloat);
        d_concat_bytecode(&out, &subaction);
        d_free_bytecode(&subaction);
    }

    // Say that the first instruction after the known inputs is the activation
    // of this node.
    if (context->debug) {
        InsNodeInfo opNodeInfo;
        opNodeInfo.node = nodeIndex;

        d_debug_add_node_info(&(out.debugInfo), out.size, opNodeInfo);
    }

    DIns nonImmediateOpcode = (convertFloat) ? fopcode : opcode;

    while (socket.socketIndex < numInputs) {
        socket.socketIndex++;

        if (d_is_input_socket(context->graph, socket)) {
            SocketMeta meta = d_get_socket_meta(context->graph, socket);

            numCons = d_socket_num_connections(context->graph, socket);

            if (!convertFloat && meta.type != TYPE_FLOAT && numCons == 0) {
                // This input is a literal, so we can use the immediate opcode.
                subaction = d_bytecode_ins(fiopcode);
                d_bytecode_set_fimmediate(
                    subaction, 1, (fimmediate_t)meta.defaultValue.integerValue);
            } else {
                // The two inputs are both on the top of the stack, so just
                // use the non-immediate opcode.
                subaction = d_bytecode_ins(nonImmediateOpcode);
            }
            d_concat_bytecode(&out, &subaction);
            d_free_bytecode(&subaction);
        } else {
            if (socket.socketIndex == 1) {
                // The node only has one input, so apply the opcode to it.
                subaction = d_bytecode_ins(nonImmediateOpcode);
                d_concat_bytecode(&out, &subaction);
                d_free_bytecode(&subaction);
            }

            // Set the socket's stack index.
            context->stackTop = initStackTop + 1;
            set_stack_index(context, socket, context->stackTop);
        }
    }

    return out;
}

/**
 * \fn BCode d_generate_comparator(BuildContext *context, size_t nodeIndex,
 *                                 DIns opcode, DIns fopcode,
 *                                 fimmediate_t strCmpArg, bool notAfter)
 * \brief Given a comparator node, generate the bytecode for it.
 *
 * \return Bytecode to get the output of a comparator.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The index of the comparator node to get the result for.
 * \param opcode The comparator instruction.
 * \param fopcode The float variant of the instruction.
 * \param strCmpArg The SYS_STRCMP argument to use to compare strings.
 * \param notAfter Do we invert the answer at the end?
 */
BCode d_generate_comparator(BuildContext *context, size_t nodeIndex,
                            DIns opcode, DIns fopcode, fimmediate_t strCmpArg,
                            bool notAfter) {
    const NodeDefinition *nodeDef =
        d_get_node_definition(context->graph, nodeIndex);

    VERBOSE(5, "Generating bytecode for comparator %s...\n", nodeDef->name);

    // First, we need to check what types our inputs are.
    bool isString = false;
    bool isFloat  = false;

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    const size_t numInputs = d_node_num_inputs(context->graph, nodeIndex);

    for (size_t i = 0; i < numInputs; i++) {
        socket.socketIndex = i;
        SocketMeta meta    = d_get_socket_meta(context->graph, socket);

        if (meta.type == TYPE_STRING) {
            isString = true;
            break;
        } else if (meta.type == TYPE_FLOAT) {
            isFloat = true;
            break;
        }
    }

    BCode out = d_push_node_inputs(context, nodeIndex, false, false, isFloat);

    // Say that the first instruction after the inputs is the activation of
    // this node.
    if (context->debug) {
        InsNodeInfo cmpNodeInfo;
        cmpNodeInfo.node = nodeIndex;

        d_debug_add_node_info(&(out.debugInfo), out.size, cmpNodeInfo);
    }

    if (isString) {
        // The SYS_STRCMP syscall will be used here.
        // Add on the given syscall argument.
        BCode arg = d_bytecode_ins(OP_PUSHF);
        d_bytecode_set_fimmediate(arg, 1, strCmpArg);
        d_concat_bytecode(&out, &arg);
        d_free_bytecode(&arg);

        // Use the syscall.
        BCode syscall = d_bytecode_ins(OP_SYSCALL);
        d_bytecode_set_byte(syscall, 1, SYS_STRCMP);
        d_concat_bytecode(&out, &syscall);
        d_free_bytecode(&syscall);
    } else {
        DIns compOpcode = (isFloat) ? fopcode : opcode;

        BCode comp = d_bytecode_ins(compOpcode);
        d_concat_bytecode(&out, &comp);
        d_free_bytecode(&comp);
    }

    if (notAfter) {
        BCode not = d_bytecode_ins(OP_NOT);
        d_concat_bytecode(&out, &not);
        d_free_bytecode(&not);
    }

    socket.socketIndex = 2;
    set_stack_index(context, socket, --context->stackTop);

    return out;
}

/**
 * \fn BCode d_generate_call(BuildContext *context, size_t nodeIndex)
 * \brief Given a node that calls a function or subroutine, generate the
 * bytecode to call it.
 *
 * \return Bytecode to call the function or subroutine.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The index of the node to generate the bytecode for.
 */
BCode d_generate_call(BuildContext *context, size_t nodeIndex) {
    Node node                     = context->graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;
    NameDefinition nameDef        = node.nameDefinition;

    VERBOSE(5, "Generating bytecode for call %s...\n", nodeDef->name);

    // Get information about the call from the node's definition.
    size_t numArgs    = 0;
    size_t numRets    = 0;
    DIns opcode       = OP_CALLCI;
    LinkType linkType = LINK_CFUNCTION;
    void *metaData    = (void *)nameDef.definition.cFunction;
    bool isSubroutine = false;

    if (nameDef.type == NAME_FUNCTION) {
        const NodeDefinition funcDef =
            nameDef.definition.function->functionDefinition;

        numArgs      = d_definition_num_inputs(&funcDef);
        numRets      = d_definition_num_outputs(&funcDef);
        opcode       = OP_CALLI;
        linkType     = LINK_FUNCTION;
        metaData     = (void *)nameDef.definition.function;
        isSubroutine = d_is_execution_definition(&funcDef);
    } else if (nameDef.type == NAME_CFUNCTION) {
        const NodeDefinition funcDef = nameDef.definition.cFunction->definition;

        numArgs      = d_definition_num_inputs(&funcDef);
        numRets      = d_definition_num_outputs(&funcDef);
        isSubroutine = d_is_execution_definition(&funcDef);
    }

    size_t numInputs  = numArgs;
    size_t numOutputs = numRets;

    // If it is a subroutine, we don't want to count the execution sockets.
    if (isSubroutine) {
        numArgs--;
        numRets--;
    }

    // Push the arguments in order, such that the first argument gets pushed
    // first, then the second, etc.
    BCode out = d_push_node_inputs(context, nodeIndex, true, false, false);

    // Call the function/subroutine, and link to it later.
    BCode call = d_bytecode_ins(opcode);

    d_bytecode_set_byte(call, FIMMEDIATE_SIZE + 1, (char)numArgs);

    // We don't care if there's a duplicate entry of the metadata. It won't
    // affect how we allocate data in the data section or anything like that.
    size_t _dummyMeta;
    bool _dummyBool;

    LinkMeta meta = d_link_new_meta(linkType, nodeDef->name, metaData);
    d_add_link_to_ins(context, &call, 0, meta, &_dummyMeta, &_dummyBool);

    // Set the call instruction to represent activating the node.
    if (context->debug) {
        InsNodeInfo callNodeInfo;
        callNodeInfo.node = nodeIndex;

        d_debug_add_node_info(&(call.debugInfo), 0, callNodeInfo);
    }

    d_concat_bytecode(&out, &call);
    d_free_bytecode(&call);

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    // Assign the stack indicies to the return sockets - the return values
    // should be at the top of the stack such that the first return value is
    // at the top.
    // TODO: Think about if we need to copy the values when they are used...
    int stackTop = context->stackTop;

    for (size_t i = numInputs; i < numInputs + numOutputs; i++) {
        socket.socketIndex = i;
        SocketMeta outMeta = d_get_socket_meta(context->graph, socket);

        if (outMeta.type != TYPE_EXECUTION) {
            set_stack_index(context, socket, stackTop--);
        }
    }

    return out;
}

/**
 * \fn BCode d_push_argument(BuildContext *context, NodeSocket socket)
 * \brief Given an output socket that is a function/subroutine argument,
 * generate bytecode to push the value of the argument to the top of the stack.
 *
 * \return Bytecode to push the argument.
 *
 * \param context The context needed to generate the bytecode.
 * \param socket The output socket representing the function argument.
 */
BCode d_push_argument(BuildContext *context, NodeSocket socket) {
    VERBOSE(5, "Generating bytecode for argument socket #%zd in node #%zd...\n",
            socket.socketIndex, socket.nodeIndex);

    BCode out = d_malloc_bytecode(0);

    if (!d_is_input_socket(context->graph, socket)) {

        // Due to the calling convention, the argument should already be on the
        // stack, but relative to the frame pointer. We just need to figure out
        // which index.
        fimmediate_t index = socket.socketIndex;

        // Ignore the execution socket if it exists.
        if (d_is_execution_node(context->graph, socket.nodeIndex)) {
            index--;
        }

        out = d_bytecode_ins(OP_GETFI);
        d_bytecode_set_fimmediate(out, 1, index);

        // It is up to the caller to set the socket's stack index,
        // as it should not be set here, since it should be copied.

        context->stackTop++;
    }

    return out;
}

/**
 * \fn BCode d_generate_return(BuildContext *context, Node *returnNode)
 * \brief Given a Return node, generate the bytecode to return from the
 * function/subroutine with the return values.
 *
 * \return Bytecode to return from the function/subroutine.
 *
 * \param context The context needed to generate the bytecode.
 * \param returnNode The Return node to return with.
 */
BCode d_generate_return(BuildContext *context, size_t returnNodeIndex) {
    Node node                    = context->graph.nodes[returnNodeIndex];
    SheetFunction *function      = node.nameDefinition.definition.function;
    const NodeDefinition funcDef = function->functionDefinition;

    VERBOSE(5, "Generating bytecode to return from %s...\n", funcDef.name);

    BCode out = d_malloc_bytecode(0);

    size_t numReturns = d_definition_num_outputs(&funcDef);

    if (d_is_execution_definition(&funcDef)) {
        numReturns--;
    }

    if (numReturns > 0) {
        out = d_push_node_inputs(context, returnNodeIndex, false, false, false);
    }

    BCode ret;

    if (numReturns == 0) {
        ret = d_bytecode_ins(OP_RET);
    } else {
        ret = d_bytecode_ins(OP_RETN);
        d_bytecode_set_byte(ret, 1, (char)numReturns);
    }

    // Set the return instruction to represent activating the return node.
    if (context->debug) {
        InsNodeInfo returnNodeInfo;
        returnNodeInfo.node = returnNodeIndex;

        d_debug_add_node_info(&(ret.debugInfo), 0, returnNodeInfo);
    }

    d_concat_bytecode(&out, &ret);
    d_free_bytecode(&ret);

    return out;
}

/**
 * \fn BCode d_generate_nonexecution_node(BuildContext *context,
 *                                        size_t nodeIndex)
 * \brief Given a non-execution node, generate the bytecode to get the output.
 *
 * \return Bytecode to run the nonexecution node's function.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The index of the non-execution node.
 */
BCode d_generate_nonexecution_node(BuildContext *context, size_t nodeIndex) {
    Node node                     = context->graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;
    NameDefinition nameDef        = node.nameDefinition;

    VERBOSE(5, "- Generating bytecode for non-execution node %s...\n",
            nodeDef->name);

    BCode out = d_malloc_bytecode(0);

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    // Firstly, we need to check if the node is a particular function -
    // spoiler alert, one of them is not like the others...
    const CoreFunction coreFunc = d_core_find_name(nodeDef->name);

    if (coreFunc == CORE_TERNARY) {
        // Hi. This is the story of why this if statement exists.
        // I was a dumbass. I just thought, "oh, I should probably add a
        // Ternary node, and it doesn't seem too bad, it's just one JRCON,
        // right?". WRONG. SO WRONG. Turns out for LITERALLY ANY OTHER
        // non-execution node you can just call d_push_node_inputs, but NOOO
        // here you need to get seperate bytecode for each of the inputs since
        // you don't want to run the bytecode for ALL inputs only to just use
        // one, do you??? Nooo, no one wants that...
        // P.S. This is why d_push_input exists.
        //     - drwhut

        // Firstly, generate the bytecode for the boolean input.
        socket.socketIndex  = 0;
        SocketMeta boolMeta = d_get_socket_meta(context->graph, socket);

        // If the boolean input is a literal, we only need to generate bytecode
        // for the input that is active, since that is all that is ever gonna
        // be picked!
        bool boolIsLiteral =
            (d_socket_num_connections(context->graph, socket) == 0);
        bool boolLiteralValue = boolMeta.defaultValue.booleanValue;

        BCode boolCode = d_push_input(context, socket, false);

        // The problem with this node is that either the true bytecode or false
        // bytecode will get run, which means the top of the stack can be in
        // different positions by the end of the node's bytecode.
        // This means if one of the true or false bytecodes has a lesser stack
        // top than the other, we need to add pushes such that both bytecodes
        // end with the same stack top.
        int stackTopBefore = context->stackTop;

        // Next, get the bytecode for the true input.
        NodeSocket trueSocket  = socket;
        trueSocket.socketIndex = 1;
        BCode trueCode         = d_malloc_bytecode(0);

        if (!boolIsLiteral || boolLiteralValue) {
            trueCode = d_push_input(context, trueSocket, false);
        }

        int stackTopTrue = context->stackTop;

        // Reset the stack top for the false bytecode.
        context->stackTop = stackTopBefore;

        // Finally, get the bytecode for the false input.
        NodeSocket falseSocket  = socket;
        falseSocket.socketIndex = 2;
        BCode falseCode         = d_malloc_bytecode(0);

        if (!boolIsLiteral || !boolLiteralValue) {
            falseCode = d_push_input(context, falseSocket, false);
        }

        int stackTopFalse = context->stackTop;

        // Get what the final stack top will be, and get the other bytecode to
        // push as many times as needed to make up the difference.
        int finalStackTop = stackTopFalse;
        BCode pushn       = d_malloc_bytecode(0);

        if (stackTopTrue > stackTopFalse) {
            finalStackTop = stackTopTrue;

            fimmediate_t diff = stackTopTrue - stackTopFalse;

            // Prepend the PUSHNF instruction.
            pushn = d_bytecode_ins(OP_PUSHNF);
            d_bytecode_set_fimmediate(pushn, 1, diff);
            d_concat_bytecode(&pushn, &falseCode);
            d_free_bytecode(&falseCode);

            falseCode = pushn;
        } else if (stackTopTrue < stackTopFalse) {
            finalStackTop = stackTopFalse;

            fimmediate_t diff = stackTopFalse - stackTopTrue;

            // Prepend the PUSHNF instruction.
            pushn = d_bytecode_ins(OP_PUSHNF);
            d_bytecode_set_fimmediate(pushn, 1, diff);
            d_concat_bytecode(&pushn, &trueCode);
            d_free_bytecode(&trueCode);

            trueCode = pushn;
        }

        context->stackTop = finalStackTop;

        socket.socketIndex = 3;
        set_stack_index(context, socket, finalStackTop);

        // If the boolean input is a literal, then it's easy! We just output
        // the bytecode that gives us the output corresponding to if the
        // literal is true or false.
        if (boolIsLiteral) {
            BCode *append = (boolLiteralValue) ? &trueCode : &falseCode;
            d_concat_bytecode(&out, append);
            d_free_bytecode(append);

            // Setting the output socket stack index.
            NodeSocket inputSocket =
                (boolLiteralValue) ? trueSocket : falseSocket;

            int inputIndex = get_stack_index(context, inputSocket);
            set_stack_index(context, socket, inputIndex);

        } else {
            // Wise guy, eh? So we need to include the bytecode for both the
            // true AND the false inputs, and only run the correct one
            // depending on the value of the boolean input.
            d_concat_bytecode(&out, &boolCode);
            d_free_bytecode(&boolCode);

            // Since the boolean is going to get poped off by the JRCONFI
            // instruction, copy it on the stack in case other nodes still need
            // its value.
            socket.socketIndex = 0;
            int wireIndex      = d_wire_find_first(context->graph, socket);

            if (IS_WIRE_FROM(context->graph, wireIndex, socket)) {
                NodeSocket connSocket =
                    context->graph.wires[wireIndex].socketTo;

                if (d_socket_num_connections(context->graph, connSocket) > 1) {
                    BCode copyBool = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(copyBool, 1, 0);
                    d_concat_bytecode(&out, &copyBool);
                    d_free_bytecode(&boolCode);
                }
            }

            // At the end of the false code, we need to add a JRFI to jump over
            // the true code.
            fimmediate_t jmpAmt = d_vm_ins_size(OP_JRFI) + trueCode.size;
            BCode jumpOverTrue  = d_bytecode_ins(OP_JRFI);
            d_bytecode_set_fimmediate(jumpOverTrue, 1, jmpAmt);
            d_concat_bytecode(&falseCode, &jumpOverTrue);
            d_free_bytecode(&jumpOverTrue);

            // After we have evaluated the boolean input, we need to add a
            // JRCONFI instruction to jump to the true code if the boolean is
            // true, and run the false code otherwise.
            jmpAmt = d_vm_ins_size(OP_JRCONFI) + falseCode.size;

            BCode jumpToCode = d_bytecode_ins(OP_JRCONFI);
            d_bytecode_set_fimmediate(jumpToCode, 1, jmpAmt);

            // Say that the first instruction after the boolean input is the
            // activation of this node.
            if (context->debug) {
                InsNodeInfo ternaryNodeInfo;
                ternaryNodeInfo.node = nodeIndex;

                d_debug_add_node_info(&(jumpToCode.debugInfo), 0, ternaryNodeInfo);
            }

            d_concat_bytecode(&out, &jumpToCode);
            d_free_bytecode(&jumpToCode);

            // Add the false code, and the true code afterwards as well.
            d_concat_bytecode(&out, &falseCode);
            d_free_bytecode(&falseCode);

            d_concat_bytecode(&out, &trueCode);
            d_free_bytecode(&trueCode);
        }
    } else {
        BCode action = d_malloc_bytecode(0);

        if ((int)coreFunc >= 0) {
            // Generate bytecode depending on the core function.
            // Remember that it's only non-execution functions we care about.
            switch (coreFunc) {
                case CORE_ADD:;
                    action = d_generate_operator(context, nodeIndex, OP_ADD,
                                                 OP_ADDF, OP_ADDFI, false);
                    break;
                case CORE_AND:;
                    action = d_generate_operator(context, nodeIndex, OP_AND, 0,
                                                 OP_ANDFI, false);
                    break;
                case CORE_DIV:
                case CORE_DIVIDE:;
                    action =
                        d_generate_operator(context, nodeIndex, OP_DIV, OP_DIVF,
                                            OP_DIVFI, coreFunc == CORE_DIVIDE);

                    if (coreFunc == CORE_DIV) {
                        NodeSocket socket1;
                        socket1.nodeIndex   = nodeIndex;
                        socket1.socketIndex = 0;

                        NodeSocket socket2  = socket1;
                        socket2.socketIndex = 1;

                        SocketMeta meta1 =
                            d_get_socket_meta(context->graph, socket1);
                        SocketMeta meta2 =
                            d_get_socket_meta(context->graph, socket2);

                        if (meta1.type == TYPE_FLOAT ||
                            meta2.type == TYPE_FLOAT) {
                            // If this is Div, and either of the inputs are
                            // floats, we need to turn the answer back into
                            // an integer.
                            BCode cvti = d_bytecode_ins(OP_CVTI);
                            d_concat_bytecode(&action, &cvti);
                            d_free_bytecode(&cvti);
                        }
                    }
                    break;
                case CORE_EQUAL:;
                    action = d_generate_comparator(context, nodeIndex, OP_CEQ,
                                                   OP_CEQF, 0, false);
                    break;
                case CORE_MULTIPLY:;
                    action = d_generate_operator(context, nodeIndex, OP_MUL,
                                                 OP_MULF, OP_MULFI, false);
                    break;
                case CORE_LENGTH:;
                    socket.socketIndex = 0;
                    action             = d_push_input(context, socket, false);

                    // Here we will use the SYS_STRLEN syscall.
                    BCode pushArgs = d_bytecode_ins(OP_PUSHNF);
                    d_bytecode_set_fimmediate(pushArgs, 1, 2);
                    d_concat_bytecode(&action, &pushArgs);
                    d_free_bytecode(&pushArgs);

                    BCode syscall = d_bytecode_ins(OP_SYSCALL);
                    d_bytecode_set_byte(syscall, 1, SYS_STRLEN);
                    d_concat_bytecode(&action, &syscall);
                    d_free_bytecode(&syscall);

                    socket.socketIndex = 1;
                    set_stack_index(context, socket, context->stackTop);
                    break;
                case CORE_LESS_THAN:;
                    action = d_generate_comparator(context, nodeIndex, OP_CLT,
                                                   OP_CLTF, 2, false);
                    break;
                case CORE_LESS_THAN_OR_EQUAL:;
                    action = d_generate_comparator(context, nodeIndex, OP_CLEQ,
                                                   OP_CLEQF, 1, false);
                    break;
                case CORE_MOD:;
                    action = d_generate_operator(context, nodeIndex, OP_MOD, 0,
                                                 OP_MODFI, false);
                    break;
                case CORE_MORE_THAN:;
                    action = d_generate_comparator(context, nodeIndex, OP_CMT,
                                                   OP_CMTF, 4, false);
                    break;
                case CORE_MORE_THAN_OR_EQUAL:;
                    action = d_generate_comparator(context, nodeIndex, OP_CMEQ,
                                                   OP_CMEQF, 3, false);
                    break;
                case CORE_NOT:;
                    // This can mean 2 different thing depending on the data
                    // types.
                    socket.socketIndex = 0;
                    const SocketMeta meta =
                        d_get_socket_meta(context->graph, socket);
                    DIns notOp = (meta.type == TYPE_INT) ? OP_INV : OP_NOT;

                    action = d_generate_operator(context, nodeIndex, notOp, 0,
                                                 0, false);
                    break;
                case CORE_NOT_EQUAL:;
                    action = d_generate_comparator(context, nodeIndex, OP_CEQ,
                                                   OP_CEQF, 0, true);
                    break;
                case CORE_OR:;
                    action = d_generate_operator(context, nodeIndex, OP_OR, 0,
                                                 OP_ORFI, false);
                    break;
                case CORE_SUBTRACT:;
                    action = d_generate_operator(context, nodeIndex, OP_SUB,
                                                 OP_SUBF, OP_SUBFI, false);
                    break;
                case CORE_TERNARY:;
                    // Ternary is a special snowflake when it comes to
                    // non-execution nodes...
                    // See above.
                    break;
                case CORE_XOR:;
                    action = d_generate_operator(context, nodeIndex, OP_XOR, 0,
                                                 OP_XORFI, false);
                    break;
                default:
                    break;
            }
        }
        // TODO: Add Define and Return here for safety.
        else {
            // So it's a custom-named node. Because this is a non-execution
            // node, this could either mean the getter of a variable, or a
            // function.

            // We can tell where it came from and what it is, since semantic
            // analysis put the information in the nodes "definition" property.
            // TODO: Error if things below are not defined.
            if (nameDef.type == NAME_VARIABLE) {
                action = d_push_variable(context, nodeIndex);
            } else if (nameDef.type == NAME_FUNCTION) {
                action = d_generate_call(context, nodeIndex);
            } else if (nameDef.type == NAME_CFUNCTION) {
                action = d_generate_call(context, nodeIndex);
            }
        }

        d_concat_bytecode(&out, &action);
        d_free_bytecode(&action);
    }

    return out;
}

/**
 * \fn BCode d_generate_execution_node(BuildContext* context, size_t nodeIndex,
 *                                     bool retAtEnd)
 * \brief Given an execution node, generate the bytecode to get the output.
 *
 * \return Bytecode to run the execution node's subroutine.
 *
 * \param context The context needed to generate the bytecode.
 * \param nodeIndex The execution node.
 * \param retAtEnd Should the bytecode return at the end?
 */
BCode d_generate_execution_node(BuildContext *context, size_t nodeIndex,
                                bool retAtEnd) {
    Node node                     = context->graph.nodes[nodeIndex];
    const NodeDefinition *nodeDef = node.definition;
    NameDefinition nameDef        = node.nameDefinition;

    VERBOSE(5, "- Generating bytecode for execution node %s...\n",
            nodeDef->name);

    const CoreFunction coreFunc = d_core_find_name(nodeDef->name);

    NodeSocket socket;
    socket.nodeIndex   = nodeIndex;
    socket.socketIndex = 0;

    bool forceFloats = false;

    if (coreFunc == CORE_FOR) {
        // If the output is going to be a float, we need the inputs to be
        // floats.
        socket.socketIndex = 5;
        SocketMeta meta    = d_get_socket_meta(context->graph, socket);

        if (meta.type == TYPE_FLOAT) {
            forceFloats = true;
        }
    }

    int stackTopBefore = context->stackTop;
    BCode out          = d_malloc_bytecode(0);

    // Usually we will want to pop all of the stack that got us the input,
    // but sometimes we need to keep things on the stack.
    bool popAfter = true;

    // We don't want to have multiple returns at the end, so if we know we've
    // added one, then there is no need to add another.
    bool addedReturn = false;

    if ((int)coreFunc >= 0) {
        // Generate bytecode depending on the core function.
        // Remember that it's only execution functions we care about.
        size_t firstNodeIndex;
        int wireIndex;

        // Generate bytecode to get the inputs, such that the first input is at
        // the top of the stack.
        out = d_push_node_inputs(context, nodeIndex, false, false, forceFloats);

        BCode action = d_malloc_bytecode(0);

        int stackTopAfterInputs = context->stackTop;

        switch (coreFunc) {
            case CORE_FOR:;
                // We will be using the "start" input as the index.
                NodeSocket indexSocket  = socket;
                indexSocket.socketIndex = 5;
                set_stack_index(context, indexSocket, context->stackTop);

                // If the step input is an immediate, we can determine here what
                // instruction to use to compare the index with the stop value.
                // If not, we need to insert bytecode to decide.
                NodeSocket stepSocket  = socket;
                stepSocket.socketIndex = 3;
                SocketMeta stepMeta =
                    d_get_socket_meta(context->graph, stepSocket);

                bool isStepImmediate =
                    (d_socket_num_connections(context->graph, stepSocket) == 0);
                LexData stepValue = stepMeta.defaultValue;

                // Start off the loop by getting the stop value, then getting
                // the current index value.
                BCode loop = d_bytecode_ins(OP_GETFI);
                d_bytecode_set_fimmediate(loop, 1, -1);

                BCode getIndex = d_bytecode_ins(OP_GETFI);
                d_bytecode_set_fimmediate(getIndex, 1, -1);
                d_concat_bytecode(&loop, &getIndex);
                d_free_bytecode(&getIndex);

                // Bytecode for the comparison between the index and stop value.
                // NOTE: We want this to push 1 if we want to exit the loop,
                // and 0 if we want to stay in it!
                BCode cmp = d_malloc_bytecode(0);

                if (isStepImmediate) {
                    DIns cmpOp;

                    // We already know from above if we should use a floating
                    // point comparison instruction.
                    if (forceFloats) {
                        if (stepValue.floatValue > 0) {
                            cmpOp = OP_CMTF;
                        } else {
                            cmpOp = OP_CLTF;
                        }
                    } else {
                        if (stepValue.integerValue > 0) {
                            cmpOp = OP_CMT;
                        } else {
                            cmpOp = OP_CLT;
                        }
                    }

                    // Add the comparison instruction to the loop.
                    cmp = d_bytecode_ins(cmpOp);
                } else {
                    // We need to put in bytecode to determine the instruction
                    // to use depending on the step value.

                    // Push the value 0.
                    BCode determineCmp = d_bytecode_ins(OP_PUSHF);
                    d_bytecode_set_fimmediate(determineCmp, 1, 0);

                    // Push the step value.
                    BCode pushStep = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(pushStep, 1, -5);
                    d_concat_bytecode(&determineCmp, &pushStep);
                    d_free_bytecode(&pushStep);

                    // Check if the step value is > 0.
                    DIns opMoreThan0 = (forceFloats) ? OP_CMTF : OP_CMT;
                    BCode moreThan0  = d_bytecode_ins(opMoreThan0);
                    d_concat_bytecode(&determineCmp, &moreThan0);
                    d_free_bytecode(&moreThan0);

                    // If step > 0.
                    DIns cmpIfStepMore = (forceFloats) ? OP_CMTF : OP_CMT;
                    BCode stepPos      = d_bytecode_ins(cmpIfStepMore);

                    // If step <= 0.
                    DIns cmpIfStepLess = (forceFloats) ? OP_CLTF : OP_CLT;
                    BCode stepNeg      = d_bytecode_ins(cmpIfStepLess);

                    // Add a jump after stepNeg to jump over the stepPos.
                    fimmediate_t jmpAmt =
                        (fimmediate_t)(d_vm_ins_size(OP_JRFI) + stepPos.size);
                    BCode jmpOverPos = d_bytecode_ins(OP_JRFI);
                    d_bytecode_set_fimmediate(jmpOverPos, 1, jmpAmt);
                    d_concat_bytecode(&stepNeg, &jmpOverPos);
                    d_free_bytecode(&jmpOverPos);

                    // Add a jump instruction if step > 0 to go to stepPos.
                    jmpAmt = (fimmediate_t)(d_vm_ins_size(OP_JRCONFI) +
                                            stepNeg.size);

                    BCode jmpToPos = d_bytecode_ins(OP_JRCONFI);
                    d_bytecode_set_fimmediate(jmpToPos, 1, jmpAmt);
                    d_concat_bytecode(&determineCmp, &jmpToPos);
                    d_free_bytecode(&jmpToPos);

                    // Add stepNeg and then stepPos.
                    d_concat_bytecode(&determineCmp, &stepNeg);
                    d_concat_bytecode(&determineCmp, &stepPos);

                    d_free_bytecode(&stepNeg);
                    d_free_bytecode(&stepPos);
                }

                d_concat_bytecode(&loop, &cmp);
                d_free_bytecode(&cmp);

                // If the condition is true, jump over the code we're making.
                // We will concatenate this later once we know how much to jump.
                BCode jmpOverLoop = d_bytecode_ins(OP_JRCONFI);

                // Get the bytecode for the loop.
                int stackTopBeforeLoop = context->stackTop;
                NodeSocket loopSocket  = socket;
                loopSocket.socketIndex = 4;

                BCode loopAfterJump = d_malloc_bytecode(0);

                wireIndex = d_wire_find_first(context->graph, loopSocket);

                if (IS_WIRE_FROM(context->graph, wireIndex, loopSocket)) {
                    NodeSocket connectedTo =
                        context->graph.wires[wireIndex].socketTo;
                    firstNodeIndex = connectedTo.nodeIndex;

                    loopAfterJump = d_generate_execution_node(
                        context, firstNodeIndex, false);

                    // Say the first instruction of this bytecode activates the
                    // execution wire between the For node and this node.
                    if (context->debug) {
                        InsExecInfo forExecInfo;
                        forExecInfo.execWire = context->graph.wires[wireIndex];

                        d_debug_add_exec_info(&(loopAfterJump.debugInfo), 0,
                                              forExecInfo);
                    }
                }

                // Pop back to the index value.
                fimmediate_t numPopFromLoop =
                    context->stackTop - stackTopBeforeLoop;
                if (numPopFromLoop < 0) {
                    numPopFromLoop = 0;
                }

                BCode popLoop = d_bytecode_ins(OP_POPF);
                d_bytecode_set_fimmediate(popLoop, 1, numPopFromLoop);
                d_concat_bytecode(&loopAfterJump, &popLoop);
                d_free_bytecode(&popLoop);

                context->stackTop = stackTopBeforeLoop;

                // Get the step value, and add it on to the index value.v
                bool addImmediate = false;
                DIns addOp;

                if (forceFloats) {
                    addOp = OP_ADDF;
                } else {
                    if (isStepImmediate) {
                        addOp        = OP_ADDFI;
                        addImmediate = true;
                    } else {
                        addOp = OP_ADD;
                    }
                }

                if (!addImmediate) {
                    BCode getStep = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(getStep, 1, -2);
                    d_concat_bytecode(&loopAfterJump, &getStep);
                    d_free_bytecode(&getStep);
                }

                BCode addStep = d_bytecode_ins(addOp);

                if (addImmediate) {
                    d_bytecode_set_fimmediate(
                        addStep, 1, (fimmediate_t)stepValue.integerValue);
                }

                d_concat_bytecode(&loopAfterJump, &addStep);
                d_free_bytecode(&addStep);

                // Finally, loop back to the start of the loop to check the
                // condition again.
                fimmediate_t jmpAmt = -(fimmediate_t)(
                    loop.size + jmpOverLoop.size + loopAfterJump.size);

                BCode loopBack = d_bytecode_ins(OP_JRFI);
                d_bytecode_set_fimmediate(loopBack, 1, jmpAmt);
                d_concat_bytecode(&loopAfterJump, &loopBack);
                d_free_bytecode(&loopBack);

                // Now we know how much to jump once the index has reached the
                // stop value.
                jmpAmt = (fimmediate_t)(jmpOverLoop.size + loopAfterJump.size);
                d_bytecode_set_fimmediate(jmpOverLoop, 1, jmpAmt);
                d_concat_bytecode(&loop, &jmpOverLoop);
                d_free_bytecode(&jmpOverLoop);

                d_concat_bytecode(&loop, &loopAfterJump);
                d_free_bytecode(&loopAfterJump);

                action = loop;

                break;

            case CORE_IF_THEN:
            case CORE_IF_THEN_ELSE:;
                BCode thenBranch = d_malloc_bytecode(0);
                BCode elseBranch = d_malloc_bytecode(0);

                int initStackTop = context->stackTop;

                // Get the individual branches compiled.
                socket.nodeIndex   = nodeIndex;
                socket.socketIndex = 2;

                wireIndex = d_wire_find_first(context->graph, socket);

                if (IS_WIRE_FROM(context->graph, wireIndex, socket)) {
                    NodeSocket connectedTo =
                        context->graph.wires[wireIndex].socketTo;
                    firstNodeIndex = connectedTo.nodeIndex;

                    thenBranch = d_generate_execution_node(
                        context, firstNodeIndex, false);

                    // Say the first instruction in this bytecode activates the
                    // execution wire.
                    if (context->debug) {
                        InsExecInfo thenExecInfo;
                        thenExecInfo.execWire = context->graph.wires[wireIndex];

                        d_debug_add_exec_info(&(thenBranch.debugInfo), 0,
                                              thenExecInfo);
                    }
                }

                int thenTopDiff = context->stackTop - initStackTop;

                if (coreFunc == CORE_IF_THEN_ELSE) {
                    // Reset the context stack top.
                    context->stackTop = initStackTop;

                    socket.socketIndex = 3;

                    wireIndex = d_wire_find_first(context->graph, socket);

                    if (IS_WIRE_FROM(context->graph, wireIndex, socket)) {
                        NodeSocket connectedTo =
                            context->graph.wires[wireIndex].socketTo;
                        firstNodeIndex = connectedTo.nodeIndex;

                        elseBranch = d_generate_execution_node(
                            context, firstNodeIndex, false);

                        // Say the first instruction in this bytecode activates
                        // the execution wire.
                        if (context->debug) {
                            InsExecInfo elseExecInfo;
                            elseExecInfo.execWire =
                                context->graph.wires[wireIndex];

                            d_debug_add_exec_info(&(elseBranch.debugInfo), 0,
                                                  elseExecInfo);
                        }
                    }
                }

                // Reset the context stack top.
                context->stackTop = initStackTop;

                int elseTopDiff = context->stackTop - initStackTop;

                // We want both branches to pop whatever they push onto the
                // stack.
                fimmediate_t numPopThen = thenTopDiff;
                if (numPopThen < 0) {
                    numPopThen = 0;
                }

                BCode popThen = d_bytecode_ins(OP_POPF);
                d_bytecode_set_fimmediate(popThen, 1, numPopThen);
                d_concat_bytecode(&thenBranch, &popThen);
                d_free_bytecode(&popThen);

                if (coreFunc == CORE_IF_THEN_ELSE) {
                    fimmediate_t numPopElse = elseTopDiff;
                    if (numPopElse < 0) {
                        numPopElse = 0;
                    }

                    BCode popElse = d_bytecode_ins(OP_POPF);
                    d_bytecode_set_fimmediate(popElse, 1, numPopElse);
                    d_concat_bytecode(&elseBranch, &popElse);
                    d_free_bytecode(&popElse);
                }

                // We put a JRCON at the start to jump to the THEN branch if
                // the condition is true, which comes after the ELSE branch.
                // If the JRCON fails, the next bit is the ELSE section, which
                // is followed by a JR to go past the THEN branch.
                // [JRCON][ELSE][JR][THEN][...]
                //    \-----------\-/     /
                //                 \-----/

                fimmediate_t jmpToThen =
                    d_vm_ins_size(OP_JRCONFI) + elseBranch.size;
                fimmediate_t jmpToEnd =
                    d_vm_ins_size(OP_JRFI) + thenBranch.size;

                // There's a possibility that the then branch doesn't have any
                // code, which means the jump instruction that goes over the
                // then branch doesn't need to exist.
                bool optimizeJmpToEnd = thenBranch.size == 0;

                if (!optimizeJmpToEnd) {
                    jmpToThen += d_vm_ins_size(OP_JRFI);
                }

                BCode conAtStart = d_bytecode_ins(OP_JRCONFI);
                d_bytecode_set_fimmediate(conAtStart, 1, jmpToThen);

                // This JRCONFI will pop the condition from the top of the
                // stack!
                context->stackTop--;

                BCode conAtEndElse = d_malloc_bytecode(0);

                if (!optimizeJmpToEnd) {
                    conAtEndElse = d_bytecode_ins(OP_JRFI);
                    d_bytecode_set_fimmediate(conAtEndElse, 1, jmpToEnd);
                }

                // Now to combine the bytecode in the correct order.
                d_concat_bytecode(&action, &conAtStart);
                d_concat_bytecode(&action, &elseBranch);
                d_concat_bytecode(&action, &conAtEndElse);
                d_concat_bytecode(&action, &thenBranch);

                d_free_bytecode(&conAtStart);
                d_free_bytecode(&elseBranch);
                d_free_bytecode(&conAtEndElse);
                d_free_bytecode(&thenBranch);

                break;

            case CORE_PRINT:;
                socket.nodeIndex   = nodeIndex;
                socket.socketIndex = 1;

                // The value to print should already be at the top of the stack.

                // Setup the arguments for this syscall.
                // TODO: Add a way to not print a newline.
                action = d_bytecode_ins(OP_PUSHF);
                d_bytecode_set_fimmediate(action, 1, 1);

                SocketMeta meta = d_get_socket_meta(context->graph, socket);

                fimmediate_t typeArg = 0;
                switch (meta.type) {
                    case TYPE_INT:;
                        typeArg = 0;
                        break;
                    case TYPE_FLOAT:;
                        typeArg = 1;
                        break;
                    case TYPE_STRING:;
                        typeArg = 2;
                        break;
                    case TYPE_BOOL:;
                        typeArg = 3;
                        break;
                    default:
                        break;
                }

                // Push the type of the variable.
                BCode printType = d_bytecode_ins(OP_PUSHF);
                d_bytecode_set_fimmediate(printType, 1, typeArg);
                d_concat_bytecode(&action, &printType);
                d_free_bytecode(&printType);

                // Call the syscall.
                BCode print = d_bytecode_ins(OP_SYSCALL);
                d_bytecode_set_byte(print, 1, SYS_PRINT);
                d_concat_bytecode(&action, &print);
                d_free_bytecode(&print);

                // The return value should have replaced the original value,
                // so there is no need to change the stack top.

                break;

            case CORE_SET:;
                // We need the variable's metadata!
                // NOTE: The definition set in the node for Set nodes is the
                // definition of the variable!
                SheetVariable *var = nameDef.definition.variable;

                const SocketMeta varMeta = var->variableMeta;

                // Depending on the variable type, we need to use different
                // opcodes and link types.
                size_t varSize = (varMeta.type == TYPE_BOOL) ? 1 : sizeof(dint);
                DIns storeAdr  = (varSize == 1) ? OP_SETADRB : OP_SETADR;
                LinkType linkType = (varMeta.type == TYPE_STRING)
                                        ? LINK_VARIABLE_POINTER
                                        : LINK_VARIABLE;

                // Push the address of the variable onto the stack.
                // This will be linked later.
                action = d_bytecode_ins(OP_PUSHF);

                // TODO: Implement copying strings.
                if (varMeta.type == TYPE_STRING) {
                }

                // At this point the address of the variable should be at the
                // top of the stack, and the new value should be one below.
                BCode store = d_bytecode_ins(storeAdr);
                d_concat_bytecode(&action, &store);
                d_free_bytecode(&store);

                context->stackTop--;

                // Now set up the link to the variable.
                LinkMeta varLinkMeta =
                    d_link_new_meta(linkType, varMeta.name, var);

                size_t _index;
                bool _wasDuplicate;
                d_add_link_to_ins(context, &action, 0, varLinkMeta, &_index,
                                  &_wasDuplicate);

                break;

            case CORE_WHILE:;
                // Get the boolean socket.
                socket.nodeIndex   = nodeIndex;
                socket.socketIndex = 1;

                const SocketMeta boolMeta =
                    d_get_socket_meta(context->graph, socket);

                // Check to see if it is a false literal. If it is, then this
                // execution node is useless.
                if (d_socket_num_connections(context->graph, socket) == 0 &&
                    boolMeta.defaultValue.booleanValue == false) {
                    break;
                }

                // Now generate the bytecode for the code that executes if the
                // condition is true.
                socket.socketIndex = 2;

                // TODO: What if there isn't a connecting node?
                wireIndex = d_wire_find_first(context->graph, socket);

                BCode trueCode = d_malloc_bytecode(0);

                if (IS_WIRE_FROM(context->graph, wireIndex, socket)) {
                    NodeSocket connectedTo =
                        context->graph.wires[wireIndex].socketTo;
                    firstNodeIndex = connectedTo.nodeIndex;

                    // We need to simulate the boolean getting poped from the
                    // stack by JRCONFI.
                    context->stackTop--;
                    trueCode = d_generate_execution_node(context,
                                                         firstNodeIndex, false);

                    // Say the first instruction of this bytecode activates the
                    // execution wire between the While node and this node.
                    if (context->debug) {
                        InsExecInfo whileExecInfo;
                        whileExecInfo.execWire =
                            context->graph.wires[wireIndex];

                        d_debug_add_exec_info(&(trueCode.debugInfo), 0,
                                              whileExecInfo);
                    }
                }

                // After the loop has executed, we want to pop from the stack
                // to the point before the boolean variable got calculated,
                // and jump back far enough as to recalculate the boolean value.
                fimmediate_t numPop = context->stackTop - stackTopBefore;
                if (numPop < 0) {
                    numPop = 0;
                }

                BCode popExtra = d_bytecode_ins(OP_POPF);
                d_bytecode_set_fimmediate(popExtra, 1, numPop);
                d_concat_bytecode(&trueCode, &popExtra);
                d_free_bytecode(&popExtra);

                fimmediate_t loopBackAmt = -(fimmediate_t)(
                    out.size + trueCode.size + d_vm_ins_size(OP_NOT) +
                    d_vm_ins_size(OP_JRCONFI));

                BCode jmpBack = d_bytecode_ins(OP_JRFI);
                d_bytecode_set_fimmediate(jmpBack, 1, loopBackAmt);
                d_concat_bytecode(&trueCode, &jmpBack);
                d_free_bytecode(&jmpBack);

                // When we get our input, we want to skip the loop code if it
                // is false.
                action = d_bytecode_ins(OP_NOT);

                fimmediate_t jmpOverAmt =
                    trueCode.size + d_vm_ins_size(OP_JRCONFI);

                BCode jmpOver = d_bytecode_ins(OP_JRCONFI);
                d_bytecode_set_fimmediate(jmpOver, 1, jmpOverAmt);
                d_concat_bytecode(&action, &jmpOver);
                d_free_bytecode(&jmpOver);

                d_concat_bytecode(&action, &trueCode);
                d_free_bytecode(&trueCode);

                // No matter if the while loop didn't activate once, or it
                // activated multiple times, there should be only one "instance"
                // of the boolean being calculated on the stack.
                // The -1 is because JRCONFI will pop the condition from the
                // top of the stack.
                context->stackTop = stackTopAfterInputs - 1;

                break;

            default:
                break;
        }

        // Say that the first instruction in action is when the execution node
        // gets activated.
        if (context->debug) {
            InsNodeInfo execNodeInfo;
            execNodeInfo.node = nodeIndex;

            d_debug_add_node_info(&(action.debugInfo), 0, execNodeInfo);
        }

        d_concat_bytecode(&out, &action);
        d_free_bytecode(&action);
    }
    // TODO: Add define here for safety.
    else if (strcmp(nodeDef->name, "Return") == 0) {
        out         = d_generate_return(context, nodeIndex);
        addedReturn = true;
    } else {
        // Put arguments into the stack and call the subroutine.
        out = d_generate_call(context, nodeIndex);

        // This will push return values we will want to keep!
        popAfter = false;
    }

    // Now, in order to optimise the usage of the stack, we pop from the stack
    // back to before the inputs were generated. We've used the inputs in this
    // node, we don't need them anymore!
    if (popAfter) {
        fimmediate_t numPop = context->stackTop - stackTopBefore;
        if (numPop < 0) {
            numPop = 0;
        }

        BCode popBack = d_bytecode_ins(OP_POPF);
        d_bytecode_set_fimmediate(popBack, 1, numPop);
        d_concat_bytecode(&out, &popBack);
        d_free_bytecode(&popBack);

        context->stackTop = stackTopBefore;
    }

    // Now, we generate the bytecode for the next execution node.
    BCode nextCode = d_malloc_bytecode(0);

    const size_t numInputs = d_node_num_inputs(context->graph, nodeIndex);

    NodeSocket lastExecSocket  = socket;
    lastExecSocket.socketIndex = 0;
    bool lastExecFound         = false;
    for (size_t i = 0; i < d_node_num_outputs(context->graph, nodeIndex); i++) {

        socket.socketIndex = numInputs + i;
        SocketMeta meta    = d_get_socket_meta(context->graph, socket);

        // We only care about the LAST execution socket.
        if (meta.type == TYPE_EXECUTION) {
            lastExecSocket.socketIndex = socket.socketIndex;
            lastExecFound              = true;
        }
    }

    bool isNextNode = false;
    if (lastExecFound) {
        int wireIndex = d_wire_find_first(context->graph, lastExecSocket);

        if (IS_WIRE_FROM(context->graph, wireIndex, lastExecSocket)) {
            isNextNode = true;

            NodeSocket connectedTo = context->graph.wires[wireIndex].socketTo;
            size_t nextNodeIndex   = connectedTo.nodeIndex;

            // Recursively call d_generate_execution_node to generate the
            // bytecode after this node's bytecode.
            nextCode =
                d_generate_execution_node(context, nextNodeIndex, retAtEnd);

            // Say the first instruction of this bytecode represents activating
            // the execution wire.
            if (context->debug) {
                InsExecInfo execInfo;
                execInfo.execWire = context->graph.wires[wireIndex];

                d_debug_add_exec_info(&(nextCode.debugInfo), 0, execInfo);
            }
        }
    }

    if (!(isNextNode || addedReturn) && retAtEnd) {
        // Add a RET instruction here, so the VM returns.
        nextCode = d_bytecode_ins(OP_RET);
    }

    d_concat_bytecode(&out, &nextCode);
    d_free_bytecode(&nextCode);

    return out;
}

/**
 * \fn BCode d_generate_start(BuildContext *context, Node *startNode)
 * \brief Given a Start node, generate the bytecode for the sequence starting
 * from this node.
 *
 * \return The bytecode generated for the Start function.
 *
 * \param context The context needed to generate the bytecode.
 * \param startNode A pointer to the Start node.
 */
BCode d_generate_start(BuildContext *context, size_t startNodeIndex) {
    // As the first instruction, place a RET, which acts as a safety barrier
    // for any function that is defined before this one (this ensures the
    // previous function doesn't "leak" into this one).
    BCode out = d_bytecode_ins(OP_RET);

    // In terms of generating the bytecode from the Start node, this is easy.
    // We just need to call the bytecode generation functions starting from the
    // first node after Start.

    if (d_node_num_outputs(context->graph, startNodeIndex) == 1) {
        NodeSocket socket;
        socket.nodeIndex   = startNodeIndex;
        socket.socketIndex = 0;

        int wireIndex = d_wire_find_first(context->graph, socket);

        if (wireIndex >= 0) {
            VERBOSE(5, "-- Generating bytecode for Start function...\n");

            socket = context->graph.wires[wireIndex].socketTo;

            BCode exe =
                d_generate_execution_node(context, socket.nodeIndex, true);

            // Say that the first instruction in this bytecode represents
            // activating the execution socket.
            if (context->debug) {
                InsExecInfo execInfo;
                execInfo.execWire = context->graph.wires[wireIndex];

                d_debug_add_exec_info(&(exe.debugInfo), 0, execInfo);
            }

            d_concat_bytecode(&out, &exe);
            d_free_bytecode(&exe);
        }
    }

    return out;
}

/**
 * \fn BCode d_generate_function(BuildContext *context, SheetFunction func)
 * \brief Given a function, generate the bytecode for it.
 *
 * \return The bytecode generated for the function.
 *
 * \param context The context needed to generate the bytecode.
 * \param func The function to generate the bytecode for.
 */
BCode d_generate_function(BuildContext *context, SheetFunction func) {
    // As the first instruction, place a RET, which allows linkers to link to
    // this instruction, since the program counter of the virtual machine will
    // ALWAYS increment, it will increment to the actual first instruction of
    // the function.
    // TODO: Is this still true?
    BCode out = d_bytecode_ins(OP_RET);

    // Set the top of the stack to be accurate, i.e. there will be arguments
    // pushed in before the function runs.
    context->stackTop = d_definition_num_inputs(&(func.functionDefinition));

    if (d_is_subroutine(func)) {
        if (func.numDefineNodes == 1) {
            VERBOSE(5, "-- Generating bytecode for subroutine %s...\n",
                    func.functionDefinition.name);

            // If it's a subroutine, we can just recursively go through the
            // sequence to the Return.
            // NOTE: The first argument should be the name of the function.
            NodeSocket socket;
            socket.nodeIndex   = func.defineNodeIndex;
            socket.socketIndex = 1;

            int wireIndex = d_wire_find_first(context->graph, socket);

            if (wireIndex >= 0) {
                socket = context->graph.wires[wireIndex].socketTo;

                BCode exe =
                    d_generate_execution_node(context, socket.nodeIndex, true);

                // Say that the first instruction in this bytecode represents
                // activating the execution wire after the Define node.
                if (context->debug) {
                    InsExecInfo execInfo;
                    execInfo.execWire = context->graph.wires[wireIndex];

                    d_debug_add_exec_info(&(exe.debugInfo), 0, execInfo);
                }

                d_concat_bytecode(&out, &exe);
                d_free_bytecode(&exe);
            }
        }
    } else {
        VERBOSE(5, "-- Generating bytecode for function %s...\n",
                func.functionDefinition.name);

        // Otherwise it's a function. In which case we need to find the
        // Return node for it and recurse backwards through the inputs.
        // NOTE: Functions (unlike subroutines) should only have one Return
        // node.
        size_t returnNode = func.lastReturnNodeIndex;

        if (func.numReturnNodes == 1) {
            // Now we recursively generate the bytecode for the inputs
            // of the Return node, so the final Return values are
            // calculated.
            BCode funcCode = d_generate_return(context, returnNode);

            d_concat_bytecode(&out, &funcCode);
            d_free_bytecode(&funcCode);
        }
    }

    return out;
}

/**
 * \fn void d_codegen_compile(Sheet *sheet, bool debug)
 * \brief Given that Semantic Analysis has taken place, generate the bytecode
 * for a given sheet.
 *
 * \param sheet The sheet to generate the bytecode for.
 * \param debug If true, we include debug information as well.
 */
void d_codegen_compile(Sheet *sheet, bool debug) {

    BCode text = d_malloc_bytecode(0);

    // Create a context object for the build.
    BuildContext context;
    context.graph    = sheet->graph;
    context.stackTop = -1;

    context.linkMetaList = d_link_new_meta_list();

    context.dataSection     = NULL;
    context.dataSectionSize = 0;

    context.debug = debug; // So we don't waste time generating debug info.

    // Start off by allocating space for the variables defined in the sheet.
    for (size_t i = 0; i < sheet->numVariables; i++) {
        SheetVariable *var       = sheet->variables + i;
        const SocketMeta varMeta = var->variableMeta;

        // Copy the name so d_link_free_list doesn't go balistic.
        size_t nameLengthPlusNull = strlen(varMeta.name) + 1;
        char *varName             = d_calloc(nameLengthPlusNull, sizeof(char));
        memcpy(varName, varMeta.name, nameLengthPlusNull);

        // Create a LinkMeta entry so we know it exists.
        LinkMeta linkMeta = d_link_new_meta((varMeta.type == TYPE_STRING)
                                                ? LINK_VARIABLE_POINTER
                                                : LINK_VARIABLE,
                                            varName, var);

        d_link_meta_list_push(&(context.linkMetaList), linkMeta);

        // Allocate space in the data section for the variable to fit in.
        d_allocate_variable(&context, var,
                            (varMeta.type == TYPE_BOOL) ? 1 : sizeof(dint),
                            context.linkMetaList.size - 1);
    }

    // Next, generate bytecode for the custom functions!
    for (size_t i = 0; i < sheet->numFunctions; i++) {
        SheetFunction *func          = sheet->functions + i;
        const NodeDefinition funcDef = func->functionDefinition;

        BCode code = d_generate_function(&context, *func);

        // Before we concatenate the bytecode, we need to take a note of where
        // it will go in the text section, so the linker knows where to call to
        // when other functions call this one.

        // Check that the metadata doesn't already exist.
        LinkMeta meta = d_link_new_meta(LINK_FUNCTION, funcDef.name, func);

        LinkMeta *metaInList = NULL;

        for (size_t j = 0; j < context.linkMetaList.size; j++) {
            LinkMeta testAgainst = context.linkMetaList.list[j];

            if (meta.type == testAgainst.type &&
                strcmp(meta.name, testAgainst.name) == 0) {
                metaInList = &(context.linkMetaList.list[j]);
                break;
            }
        }

        // If it was not found in the list, add it.
        if (metaInList == NULL) {
            // But first, copy the name.
            size_t nameLen = strlen(meta.name) + 1;
            char *newName  = d_calloc(nameLen, sizeof(char));
            memcpy(newName, meta.name, nameLen);
            meta.name = newName;

            d_link_meta_list_push(&context.linkMetaList, meta);
            metaInList =
                &(context.linkMetaList.list[context.linkMetaList.size - 1]);
        }

        // Now set the pointer of the metadata to the index of the
        // function in the code, +1 because of the RET at the start of the
        // function.
        metaInList->_ptr = (char *)(text.size + 1);

        d_concat_bytecode(&text, &code);
        d_free_bytecode(&code);
    }

    // And end with the Start function.
    if (sheet->startNodeIndex >= 0) {
        BCode start = d_generate_start(&context, sheet->startNodeIndex);

        // We need to set the .main value of the sheet to the index of the
        // first instruction (not the RET before!), but if there is only one
        // instruction, that is the RET instruction, then we need to add
        // another RET instruction to make sure the VM runs it, even though
        // it doesn't do anything.
        sheet->_main = text.size + 1;

        if (start.size == 1) {
            BCode extraRet = d_malloc_bytecode(1);
            d_bytecode_set_byte(extraRet, 0, OP_RET);
            d_concat_bytecode(&start, &extraRet);
            d_free_bytecode(&extraRet);
        }

        d_concat_bytecode(&text, &start);
        d_free_bytecode(&start);
    }

    // Put the code into the sheet.
    sheet->_text     = text.code;
    sheet->_textSize = text.size;

    // Put the data section into the sheet.
    sheet->_data     = context.dataSection;
    sheet->_dataSize = context.dataSectionSize;

    // Only put the debugging information in if we generated it.
    if (debug) {
        sheet->_debugInfo = text.debugInfo;
    } else {
        d_debug_free_info(&(text.debugInfo));
    }

    // Put the link metadata into the sheet.
    sheet->_link = context.linkMetaList;

    // Put the relational records of instructions to links into the sheet.
    sheet->_insLinkList     = text.linkList;
    sheet->_insLinkListSize = text.linkListSize;

    // TODO: Put checks in other functions to make sure the sheet has
    // bytecode in first.
    sheet->_isCompiled = true;
}
