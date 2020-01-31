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

#include "dcodegen.h"

#include "dasm.h"
#include "dcore.h"
#include "decision.h"
#include "derror.h"
#include "dlink.h"
#include "dmalloc.h"
#include "dsemantic.h"
#include "dsheet.h"

#include <stdlib.h>
#include <string.h>

/*
=== STRUCTURE FUNCTIONS ===================================
*/

/**
 * \fn BCode d_malloc_bytecode(size_t size)
 * \brief Create a malloc'd BCode object, with a set number of bytes.
 *
 * \return The BCode object with malloc'd elements.
 *
 * \param size The number of bytes.
 */
BCode d_malloc_bytecode(size_t size) {
    BCode out;

    out.code = (char *)d_malloc(size);
    out.size = size;

    out.linkList     = NULL;
    out.linkListSize = 0;

    return out;
}

/**
 * \fn BCode d_bytecode_ins(DIns opcode)
 * \brief Quickly create bytecode that is the size of an opcode, which also has
 * its first byte set as the opcode itself.
 *
 * \return The opcode-initialised bytecode.
 *
 * \param opcode The opcode to initialise with.
 */
BCode d_bytecode_ins(DIns opcode) {
    BCode out = d_malloc_bytecode(d_vm_ins_size(opcode));
    d_bytecode_set_byte(out, 0, (char)opcode);
    return out;
}

/**
 * \fn void d_bytecode_set_byte(BCode bcode, size_t index, char byte)
 * \brief Given some bytecode, set a byte in the bytecode to a given value.
 *
 * \param bcode The bytecode to edit.
 * \param index The index of the byte in the bytecode to set.
 * \param byte The value to set.
 */
void d_bytecode_set_byte(BCode bcode, size_t index, char byte) {
    if (index < bcode.size) {
        bcode.code[index] = byte;
    }
}

/**
 * \fn void d_bytecode_set_fimmediate(BCode bcode, size_t index,
 *                                   fimmediate_t fimmediate)
 * \brief Given some bytecode, set a full immediate value into the bytecode.
 *
 * **NOTE:** There are no functions to set byte or half immediates for a good
 * reason: Mixing immediate sizes during code generation is a bad idea, as
 * inserting bytecode in the middle of another bit of bytecode could make some
 * smaller immediates invalid, and they would have to increase in size, which
 * would be a pain. Instead, we only work with full immediates during code
 * generation, and reduce down the full immediate instructions to byte or
 * half immediate instructions in the optimisation stage.
 *
 * \param bcode The bytecode to edit.
 * \param index The starting index of the section of the bytecode to edit.
 * \param fimmediate The full immediate value to set.
 */
void d_bytecode_set_fimmediate(BCode bcode, size_t index,
                               fimmediate_t fimmediate) {
    if (index < bcode.size - FIMMEDIATE_SIZE + 1) {
        fimmediate_t *ptr = (fimmediate_t *)(bcode.code + index);
        *ptr              = fimmediate;
    }
}

/**
 * \fn void d_free_bytecode(BCode *bcode)
 * \brief Free malloc'd elements of bytecode.
 *
 * \param bcode The bytecode to free.
 */
void d_free_bytecode(BCode *bcode) {
    if (bcode->code != NULL) {
        free(bcode->code);
    }

    if (bcode->linkList != NULL) {
        free(bcode->linkList);
    }

    bcode->code         = NULL;
    bcode->size         = 0;
    bcode->linkList     = NULL;
    bcode->linkListSize = 0;
}

/**
 * \fn void d_concat_bytecode(BCode *base, BCode *after)
 * \brief Append bytecode to the end of another set of bytecode.
 *
 * \param base The bytecode to be added to.
 * \param after The bytecode to append. Not changed.
 */
void d_concat_bytecode(BCode *base, BCode *after) {
    if (!(after->code == NULL || after->size == 0)) {
        size_t totalSize = base->size + after->size;

        if (base->code != NULL) {
            base->code = (char *)d_realloc(base->code, totalSize);
        } else {
            base->code = (char *)d_malloc(totalSize);
        }

        // Pointer to the place we want to start adding "after".
        // We haven't changed base->size yet!
        char *addPtr = base->code + base->size;

        memcpy(addPtr, after->code, after->size);

        // Now we've concatenated the bytecode, we need to add the links as
        // well, but change the index of the instruction they point to, since
        // it's now changed.
        if (after->linkList != NULL && after->linkListSize > 0) {
            size_t totalLinkSize = base->linkListSize + after->linkListSize;

            if (base->linkList != NULL) {
                base->linkList = (InstructionToLink *)d_realloc(
                    base->linkList, totalLinkSize * sizeof(InstructionToLink));
            } else {
                base->linkList = (InstructionToLink *)d_malloc(
                    totalLinkSize * sizeof(InstructionToLink));
            }

            // i is the index in the base list.
            // j is the index in the after list.
            for (size_t i = base->linkListSize; i < totalLinkSize; i++) {
                size_t j = i - base->linkListSize;

                InstructionToLink newIns = after->linkList[j];
                newIns.ins += base->size;
                base->linkList[i] = newIns;
            }

            base->linkListSize = totalLinkSize;
        }

        base->size = totalSize;
    }
}

/**
 * \fn void d_insert_bytecode(BCode *base, BCode *insertCode,
 *                            size_t insertIndex)
 * \brief Insert some bytecode into another set of bytecode at a particular
 * point.
 *
 * This is a modification of d_optimize_remove_bytecode()
 *
 * \param base The bytecode to insert into.
 * \param insertCode The bytecode to insert.
 * \param insertIndex The index to insert indexCode into base, i.e. when the
 * operation is complete, this index will contain the start of insertCode.
 */
void d_insert_bytecode(BCode *base, BCode *insertCode, size_t insertIndex) {
    if (insertCode->size > 0) {
        // Resize the base bytecode.
        if (base->code != NULL && base->size > 0) {
            base->code =
                (char *)d_realloc(base->code, base->size + insertCode->size);
        } else {
            base->code = (char *)d_malloc(insertCode->size);
        }

        // Move everything at and after the index in the base bytecode along to
        // make room for the new bytecode.
        char *moveFrom = base->code + insertIndex;
        char *moveTo   = moveFrom + insertCode->size;
        memmove(moveTo, moveFrom, base->size - insertIndex);

        // Copy the new bytecode into the correct position.
        memcpy(moveFrom, insertCode->code, insertCode->size);

        // Set the new size of the base bytecode.
        base->size = base->size + insertCode->size;

        // By doing this we may have made some instructions that use relative
        // addressing overshoot or undershoot. We need to fix those
        // instructions.
        for (size_t i = 0; i < base->size;) {
            DIns opcode = base->code[i];

            // Remember in code generation we only work with full immediates.
            if (opcode == OP_CALLRF || opcode == OP_JRFI ||
                opcode == OP_JRCONFI) {

                fimmediate_t *jmpPtr = (fimmediate_t *)(base->code + i + 1);
                fimmediate_t jmpAmt  = *jmpPtr;

                // TODO: Deal with the case that it jumped INTO the inserted
                // region.

                // If the instruction was going backwards, and it was after the
                // inserted region, we may have a problem.
                if (i >= insertIndex + insertCode->size && jmpAmt < 0) {
                    // Did it jump over the inserted region?
                    if (i - insertCode->size + jmpAmt < insertIndex) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt - (fimmediate_t)insertCode->size;
                        *jmpPtr = jmpAmt;
                    }
                }

                // If the instruction was going forwards, and it was before the
                // inserted region, we may have a problem.
                else if (i < insertIndex && jmpAmt > 0) {
                    // Did it jump over the inserted region?
                    if (i + jmpAmt >= insertIndex + insertCode->size) {
                        // Then fix the jump amount.
                        jmpAmt  = jmpAmt + (fimmediate_t)insertCode->size;
                        *jmpPtr = jmpAmt;
                    }
                }
            }

            i += d_vm_ins_size(opcode);
        }

        // Next, we need to fix the original InstructionToLinks before adding
        // the new ones in, otherwise we won't know which is which.
        for (size_t i = 0; i < base->linkListSize; i++) {
            size_t insIndex = base->linkList[i].ins;

            if (insIndex >= insertIndex) {
                base->linkList[i].ins = insIndex + insertCode->size;
            }
        }

        // Next, we'll insert the InstructionToLinks from the inserted bytecode,
        // but we need to remember to modify their instruction indexes, since
        // they will have changed.
        if (insertCode->linkListSize > 0) {
            if (base->linkList != NULL && base->linkListSize > 0) {
                base->linkList = (InstructionToLink *)d_realloc(
                    base->linkList,
                    (base->linkListSize + insertCode->linkListSize) *
                        sizeof(InstructionToLink));
            } else {
                base->linkList = (InstructionToLink *)d_malloc(
                    (insertCode->linkListSize) * sizeof(InstructionToLink));
            }

            for (size_t i = 0; i < insertCode->linkListSize; i++) {
                InstructionToLink itl = insertCode->linkList[i];
                itl.ins               = itl.ins + insertIndex;

                // linkListSize hasn't changed yet!
                base->linkList[base->linkListSize + i] = itl;
            }

            // I spoke too soon...
            base->linkListSize = base->linkListSize + insertCode->linkListSize;
        }
    }
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
    VERBOSE(5, "Linking instruction %zu to link of type %d and name %s...\n",
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
        char *cpyName  = (char *)d_malloc(nameLen + 1);
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
            bcode->linkList = (InstructionToLink *)d_realloc(
                bcode->linkList,
                (bcode->linkListSize + 1) * sizeof(InstructionToLink));
        } else {
            bcode->linkList = (InstructionToLink *)d_malloc(
                (bcode->linkListSize + 1) * sizeof(InstructionToLink));
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
            context->dataSection =
                (char *)d_realloc(context->dataSection, newSize);
            *index = oldSize;
            return context->dataSection + oldSize;
        } else {
            context->dataSection = (char *)d_malloc(newSize);
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
    VERBOSE(5, "Allocating variable %s to data...\n", variable->name);
    size_t dataIndex;

    char *dataPtr = d_allocate_from_data_section(context, size, &dataIndex);

    // We need to initialize the value we've just allocated with the
    // default value of the variable.
    switch (variable->dataType) {
        case TYPE_INT:;
            memcpy(dataPtr, &(variable->defaultValue.integerValue), size);
            break;
        case TYPE_FLOAT:;
            memcpy(dataPtr, &(variable->defaultValue.floatValue), size);
            break;

        // Store the default value of the string as a string literal in the
        // data section. Then when linking, malloc a new string of the same
        // length, copy the string literal over, and place the pointer in
        // the data section.
        case TYPE_STRING:;
            // We just need to add a custom InstructionToLink to tell the
            // string variable where the default value is in the data.
            LinkMeta strDefaultValueLink = d_link_new_meta(
                LINK_VARIABLE_STRING_DEFAULT_VALUE, variable->name, variable);

            size_t defValMetaIndex;
            bool metaDuplicate;

            // We're not trying to replace any bytecode to load this
            // default value.
            d_add_link_to_ins(context, NULL, 0, strDefaultValueLink,
                              &defValMetaIndex, &metaDuplicate);

            if (!metaDuplicate) {
                // Add the string literal to the data section, and link it.
                size_t dataStart = d_allocate_string_literal_in_data(
                    context, NULL, 0, variable->defaultValue.stringValue);

                context->linkMetaList.list[defValMetaIndex]._ptr =
                    (char *)dataStart;
            }
            break;

        case TYPE_BOOL:;
            *dataPtr = variable->defaultValue.booleanValue;
            break;
        default:
            break;
    }

    // We need the linker to know where this variable is in the data
    // section.
    context->linkMetaList.list[indexInLinkMeta]._ptr = (char *)dataIndex;
}

/*
=== CALLING CONVENTION FUNCTIONS ==========================
*/

/**
 * \fn bool d_is_node_call(SheetNode *node)
 * \brief Is the node a function that needs to be called?
 *
 * \return The answer to the question above.
 *
 * \param node The node to query.
 */
bool d_is_node_call(SheetNode *node) {
    if (node == NULL)
        return false;

    // To answer the question, we just want to check if it's a function that
    // isn't a core function, and it isn't Start, Define or Return (which
    // aren't in the list of core functions)
    NameDefinition definition = node->definition;

    if (definition.type == NAME_FUNCTION && definition.coreFunc == -1)
        if (strcmp(node->name, "Start") != 0 &&
            strcmp(node->name, "Define") != 0 &&
            strcmp(node->name, "Return") != 0)
            return true;

    return false;
}

/**
 * \fn bool d_does_input_involve_call(SheetNode *node)
 * \brief Does getting the input of a node require calling another function?
 *
 * \return The answer to the above question.
 *
 * \param node The node whose inputs to query.
 */
bool d_does_input_involve_call(SheetNode *node) {
    if (node == NULL)
        return false;

    // Firstly, is this node a calling node?
    if (d_is_node_call(node))
        return true;

    // Alright, so let's try our inputs.
    if (node->sockets != NULL && node->numSockets > 0) {
        for (size_t i = 0; i < node->numSockets; i++) {
            SheetSocket *socket = node->sockets[i];

            // We only care about inputs with the variable data types.
            if (socket->isInput &&
                (socket->type & TYPE_VAR_ANY) == socket->type) {
                // Is this socket connected to a node?
                if (socket->numConnections == 1) {
                    SheetSocket *socketConnectedTo = socket->connections[0];
                    SheetNode *nodeConnectedTo     = socketConnectedTo->node;

                    if (!nodeConnectedTo->isExecution) {
                        if (d_does_input_involve_call(nodeConnectedTo))
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

/**
 * \fn bool d_does_output_involve_call(SheetNode *node)
 * \brief Does the execution sequence starting from node require a call?
 *
 * \return The answer to the above question.
 *
 * \param node The node to start querying from.
 */
bool d_does_output_involve_call(SheetNode *node) {
    if (node == NULL)
        return false;

    // Firstly, is this node a calling node?
    if (d_is_node_call(node))
        return true;

    // What about our inputs?
    if (d_does_input_involve_call(node))
        return true;

    // Alright, so let's try our outputs.
    if (node->sockets != NULL && node->numSockets > 0) {
        for (size_t i = 0; i < node->numSockets; i++) {
            SheetSocket *socket = node->sockets[i];

            // We only care about output execution sockets.
            if (!socket->isInput && socket->type == TYPE_EXECUTION) {
                // Is this socket connected to a node?
                if (socket->numConnections == 1) {
                    SheetSocket *socketConnectedTo = socket->connections[0];
                    SheetNode *nodeConnectedTo     = socketConnectedTo->node;

                    if (d_does_output_involve_call(nodeConnectedTo))
                        return true;
                }
            }
        }
    }

    return false;
}

/*
=== GENERATOR FUNCTIONS ===================================
*/

/**
 * \fn BCode d_push_literal(SheetSocket *socket, BuildContext *context,
 *                          bool cvtFloat)
 * \brief Generate bytecode to push a literal onto the stack.
 *
 * \return Bytecode to push the socket's literal onto the stack.
 *
 * \param socket The socket of the literal to push onto the stack.
 * \param context The context needed to build the bytecode.
 * \param cvtFloat Converts the literal to a float if possible.
 */
BCode d_push_literal(SheetSocket *socket, BuildContext *context,
                     bool cvtFloat) {
    VERBOSE(
        5,
        "Generating bytecode to get literal value of type %u from node %s...\n",
        socket->type, socket->node->name);

    // Even for floats, we want the integer representation of them.
    dint intLiteral = socket->defaultValue.integerValue;

    BCode out = d_bytecode_ins(OP_PUSHF);
    d_bytecode_set_fimmediate(out, 1, (fimmediate_t)intLiteral);

    if (socket->type == TYPE_INT && cvtFloat) {
        BCode cvtf = d_bytecode_ins(OP_CVTF);
        d_concat_bytecode(&out, &cvtf);
        d_free_bytecode(&cvtf);
    } else if (socket->type == TYPE_STRING) {
        // The literal string needs to go into the data section.
        d_allocate_string_literal_in_data(context, &out, 0,
                                          socket->defaultValue.stringValue);
    }

    // Set the socket's stack index so we know where the value lives.
    socket->_stackIndex = ++context->stackTop;

    return out;
}

/**
 * \fn BCode d_push_variable(SheetNode *node, BuildContext *context)
 * \brief Given a node that is the getter of a variable, generate bytecode to
 * push the value of the variable onto the stack.
 *
 * \return Bytecode to push the variable's value onto the stack.
 *
 * \param node The node that is the getter of a variable.
 * \param context The context needed to build the bytecode.
 */
BCode d_push_variable(SheetNode *node, BuildContext *context) {
    VERBOSE(5, "Generating bytecode to get the value of variable %s...\n",
            node->name);

    SheetVariable *variable = node->definition.variable;

    // If the variable is a boolean, the variable is 1 byte instead of
    // sizeof(dint).
    DIns opcode = (variable->dataType == TYPE_BOOL) ? OP_DEREFBI : OP_DEREFI;

    BCode out = d_bytecode_ins(opcode);
    d_bytecode_set_fimmediate(out, 1, 0); // The address will be linked later.

    // Set the socket's stack index so we know where the value lives.
    node->sockets[0]->_stackIndex = ++context->stackTop;

    LinkType linkType = (variable->dataType == TYPE_STRING)
                            ? LINK_VARIABLE_POINTER
                            : LINK_VARIABLE;

    // We need this code to remember to link to the variable.
    LinkMeta meta = d_link_new_meta(linkType, node->name, variable);

    // Now we add the link metadata to the list in the context, but we
    // want to look out for duplicate variables that may have already been
    // allocated.
    size_t metaIndexInList;
    bool wasDuplicate = false;
    d_add_link_to_ins(context, &out, 0, meta, &metaIndexInList, &wasDuplicate);

    return out;
}

/**
 * \fn BCode d_push_input(SheetSocket *socket, BuildContext *context,
 *                        bool forceFloat)
 * \brief Given an input socket, generate bytecode to push the value of the
 * input to the top of the stack.
 *
 * \return Bytecode to push the input's value onto the stack.
 *
 * \param socket The input socket to get the value for.
 * \param context The context needed to build the bytecode.
 * \param forceFloat Force integers to be converted to floats.
 */
BCode d_push_input(SheetSocket *socket, BuildContext *context,
                   bool forceFloat) {
    VERBOSE(5, "Generating bytecode to get the value of input socket %p...\n",
            socket);

    BCode out = {NULL, 0};

    if (socket->isInput && socket->type != TYPE_EXECUTION) {
        if (socket->numConnections == 0) {
            // The socket input is a literal.
            out = d_push_literal(socket, context, forceFloat);
        } else {
            // The socket has a connected socket.
            SheetSocket *connSocket = socket->connections[0];

            // Set to false if you can determine socket->_stackIndex.
            bool checkIfOnTop = true;

            // Has this output not already been generated, or has it been poped
            // off?
            if (connSocket->_stackIndex < 0 ||
                connSocket->_stackIndex > context->stackTop) {
                SheetNode *connNode = connSocket->node;

                // If this is the Define node, we need to push the argument.
                if (strcmp(connNode->name, "Define") == 0) {
                    out = d_push_argument(connSocket, context);

                    // Since we're copying the argument, set the socket's
                    // stack index now.
                    socket->_stackIndex = context->stackTop;
                    checkIfOnTop        = false;
                } else {
                    // Determine if the node is an execution node.
                    bool isExecutionNode = false;

                    for (size_t i = 0; i < connNode->numSockets; i++) {
                        SheetSocket *testSocket = connNode->sockets[i];

                        if (testSocket->type == TYPE_EXECUTION) {
                            isExecutionNode = true;
                            break;
                        }
                    }

                    if (isExecutionNode) {
                        // TODO: This.
                    } else {
                        out = d_generate_nonexecution_node(connNode, context);
                    }
                }
            }

            if (checkIfOnTop) {
                // If the value is not at the top of the stack, make sure it is.
                int inputIndex = connSocket->_stackIndex;

                if (!IS_INDEX_TOP(context, inputIndex)) {
                    BCode get = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(
                        get, 1, (fimmediate_t)STACK_INDEX_TOP(context, inputIndex));
                    d_concat_bytecode(&out, &get);
                    d_free_bytecode(&get);

                    connSocket->_stackIndex = context->stackTop;
                }

                socket->_stackIndex = connSocket->_stackIndex;
            }
        }
    }

    return out;
}

/**
 * \fn BCode d_push_node_inputs(SheetNode *node, BuildContext *context,
 *                              bool order, bool ignoreLiterals,
 *                              bool forceFloat)
 * \brief Given a node, generate bytecode to push the values of the
 * inputs to the top of the stack.
 *
 * \return Bytecode to push all input's values onto the stack.
 *
 * \param node The node whose input sockets to generate bytecode for.
 * \param context The context needed to generate the bytecode.
 * \param order If true, the inputs are pushed in order, such that the last
 * input is at the top. If false, the inputs are pushed in reverse order, such
 * that the first input is at the top.
 * \param ignoreLiterals Do not generate bytecode for non-float literal inputs.
 * \param forceFloat Force integers to be converted to floats.
 */
BCode d_push_node_inputs(SheetNode *node, BuildContext *context, bool order,
                         bool ignoreLiterals, bool forceFloat) {
    VERBOSE(5, "Generating bytecode to get the inputs for node %s...\n",
            node->name);

    BCode out = {NULL, 0};

    int start = node->numSockets - 1;
    int end   = 0;
    int step  = -1;

    if (order) {
        start = 0;
        end   = node->numSockets - 1;
        step  = 1;
    }

    // Push the inputs in the order we want.
    int i = start;

    bool inLoop = (i >= end);
    if (order) {
        inLoop = (i <= end);
    }

    while (inLoop) {
        SheetSocket *socket = node->sockets[i];

        if (socket->isInput && (socket->type & TYPE_VAR_ANY) != 0) {
            if (socket->numConnections > 0 || socket->type == TYPE_FLOAT ||
                !ignoreLiterals) {

                BCode input = d_push_input(socket, context, forceFloat);
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
    // int verifyIndex       = context->stackTop;
    int numInputs         = 0;
    bool positionsCorrect = true;

    i = end;

    inLoop = (i <= end);
    if (order) {
        inLoop = (i >= end);
    }

    while (inLoop) {
        SheetSocket *socket = node->sockets[i];

        if (socket->isInput && (socket->type & TYPE_VAR_ANY) != 0) {
            if (socket->numConnections > 0 || socket->type == TYPE_FLOAT ||
                !ignoreLiterals) {

                // Is this input the correct amount from the top?
                if (STACK_INDEX_TOP(context, socket->_stackIndex) !=
                    -numInputs) {
                    positionsCorrect = false;
                    break;
                }
                numInputs++;
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
            SheetSocket *socket = node->sockets[i];

            if (socket->isInput && (socket->type & TYPE_VAR_ANY) != 0) {
                if (socket->numConnections > 0 || socket->type == TYPE_FLOAT ||
                    !ignoreLiterals) {

                    BCode get = d_bytecode_ins(OP_GETFI);
                    d_bytecode_set_fimmediate(
                        get, 1,
                        (fimmediate_t)STACK_INDEX_TOP(context,
                                                      socket->_stackIndex));
                    d_concat_bytecode(&out, &get);
                    d_free_bytecode(&get);

                    socket->_stackIndex = context->stackTop;
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
 * \fn BCode d_generate_operator(SheetNode *node, BuildContext *context,
 *                               DIns opcode, DIns fopcode, DIns fiopcode,
 *                               bool oneInput, bool infiniteInputs,
 *                               bool forceFloat)
 * \brief Given an operator node, generate the bytecode for it.
 *
 * \return Bytecode to get the output of an operator.
 *
 * \param node The operator node to get the result for.
 * \param context The context needed to generate the bytecode.
 * \param opcode The operator instruction.
 * \param fopcode The float variant of the instruction.
 * \param fiopcode The full immediate variant of the instruction.
 * \param forceFloat Should the output always be a float?
 */
BCode d_generate_operator(SheetNode *node, BuildContext *context, DIns opcode,
                          DIns fopcode, DIns fiopcode, bool forceFloat) {
    VERBOSE(5, "Generate bytecode for operator %s...\n", node->name);

    BCode out       = {NULL, 0};
    BCode subaction = {NULL, 0};

    // Do we need to convert integer inputs to floats?
    bool convertFloat = forceFloat;

    // If we're not forcing it, we may still need to if any of the inputs are
    // floats.
    if (!convertFloat) {
        for (size_t j = 0; j < node->numSockets - 1; j++) {
            SheetSocket *inputSocket = node->sockets[j];

            if (inputSocket->type == TYPE_FLOAT) {
                convertFloat = true;
                break;
            }
        }
    }

    int initStackTop = context->stackTop;

    SheetSocket *socket = node->sockets[0];

    // Generate the bytecode for the inputs without literals - but we want to
    // start working from the first input, even if it is a literal.
    // NOTE: If we need to convert inputs to floats, we need to generate all
    // of the inputs, including literals, as we always generate the first
    // input.
    out = d_push_node_inputs(node, context, false, !convertFloat, convertFloat);
    if (!convertFloat && socket->numConnections == 0) {
        subaction = d_push_literal(socket, context, convertFloat);
        d_concat_bytecode(&out, &subaction);
        d_free_bytecode(&subaction);
    }

    DIns nonImmediateOpcode = (convertFloat) ? fopcode : opcode;

    size_t i = 0;

    while (socket->isInput && i < node->numSockets) {
        socket = node->sockets[++i];

        if (socket->isInput) {
            if (!convertFloat && socket->type != TYPE_FLOAT &&
                socket->numConnections == 0) {
                // This input is a literal, so we can use the immediate opcode.
                subaction = d_bytecode_ins(fiopcode);
                d_bytecode_set_fimmediate(
                    subaction, 1,
                    (fimmediate_t)socket->defaultValue.integerValue);
            } else {
                // The two inputs are both on the top of the stack, so just
                // use the non-immediate opcode.
                subaction = d_bytecode_ins(nonImmediateOpcode);
            }
            d_concat_bytecode(&out, &subaction);
            d_free_bytecode(&subaction);
        } else {
            if (i == 1) {
                // The node only has one input, so apply the opcode to it.
                subaction = d_bytecode_ins(nonImmediateOpcode);
                d_concat_bytecode(&out, &subaction);
                d_free_bytecode(&subaction);
            }

            // Set the socket's stack index.
            context->stackTop   = initStackTop + 1;
            socket->_stackIndex = context->stackTop;
        }
    }

    return out;
}

/**
 * \fn BCode d_generate_comparator(SheetNode *node, BuildContext *context,
 *                                 DIns opcode, DIns fopcode,
 *                                 fimmediate_t strCmpArg, bool notAfter)
 * \brief Given a comparator node, generate the bytecode for it.
 *
 * \return Bytecode to get the output of a comparator.
 *
 * \param node The comparator node to get the result for.
 * \param context The context needed to generate the bytecode.
 * \param opcode The comparator instruction.
 * \param fopcode The float variant of the instruction.
 * \param strCmpArg The SYS_STRCMP argument to use to compare strings.
 * \param notAfter Do we invert the answer at the end?
 */
BCode d_generate_comparator(SheetNode *node, BuildContext *context, DIns opcode,
                            DIns fopcode, fimmediate_t strCmpArg,
                            bool notAfter) {
    VERBOSE(5, "Generating bytecode for comparator %s...\n", node->name);

    // First, we need to check what types our inputs are.
    bool isString = false;
    bool isFloat  = false;

    for (size_t i = 0; i < node->numSockets; i++) {
        SheetSocket *socket = node->sockets[i];

        if (socket->isInput) {
            if (socket->type == TYPE_STRING) {
                isString = true;
                break;
            } else if (socket->type == TYPE_FLOAT) {
                isFloat = true;
                break;
            }
        }
    }

    BCode out = d_push_node_inputs(node, context, false, false, isFloat);

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

    SheetSocket *outputSocket = node->sockets[2];
    outputSocket->_stackIndex = --context->stackTop;

    return out;
}

/**
 * \fn BCode d_generate_call(SheetNode *node, BuildContext *context)
 * \brief Given a node that calls a function or subroutine, generate the
 * bytecode to call it.
 *
 * \return Bytecode to call the function or subroutine.
 *
 * \param node The node to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 */
BCode d_generate_call(SheetNode *node, BuildContext *context) {
    VERBOSE(5, "Generating bytecode for call %s...\n", node->name);

    // Get information about the call from the node's definition.
    size_t numArgs    = 0;
    size_t numRets    = 0;
    DIns opcode       = OP_CALLCI;
    LinkType linkType = LINK_CFUNCTION;
    void *metaData    = (void *)node->definition.cFunction;

    if (node->definition.type == NAME_FUNCTION) {
        numArgs  = node->definition.function->numArguments;
        numRets  = node->definition.function->numReturns;
        opcode   = OP_CALLI;
        linkType = LINK_FUNCTION;
        metaData = (void *)node->definition.function;
    }

    // Push the arguments in order, such that the first argument gets pushed
    // first, then the second, etc.
    BCode out = d_push_node_inputs(node, context, true, false, false);

    // Call the function/subroutine, and link to it later.
    BCode call = d_bytecode_ins(opcode);

    if (opcode == OP_CALLI) {
        d_bytecode_set_byte(call, FIMMEDIATE_SIZE + 1, (char)numArgs);
    }

    // We don't care if there's a duplicate entry of the metadata. It won't
    // affect how we allocate data in the data section or anything like that.
    size_t _dummyMeta;
    bool _dummyBool;

    LinkMeta meta = d_link_new_meta(linkType, node->name, metaData);
    d_add_link_to_ins(context, &call, 0, meta, &_dummyMeta, &_dummyBool);

    d_concat_bytecode(&out, &call);
    d_free_bytecode(&call);

    // Assign the stack indicies to the return sockets - the return values
    // should be at the top of the stack such that the first return value is
    // at the top.
    // TODO: Think about if we need to copy the values when they are used...
    int stackTop = context->stackTop;
    for (size_t i = 0; i < node->numSockets; i++) {
        SheetSocket *socket = node->sockets[i];

        if (!(socket->isInput || socket->type == TYPE_EXECUTION)) {
            socket->_stackIndex = stackTop--;
        }
    }

    return out;
}

/**
 * \fn BCode d_push_argument(SheetSocket *socket, BuildContext *context)
 * \brief Given an output socket that is a function/subroutine argument,
 * generate bytecode to push the value of the argument to the top of the stack.
 *
 * \return Bytecode to push the argument.
 *
 * \param socket The output socket representing the function argument.
 * \param context The context needed to generate the bytecode.
 */
BCode d_push_argument(SheetSocket *socket, BuildContext *context) {
    VERBOSE(5, "Generating bytecode for argument socket %p...\n", socket);

    BCode out = {NULL, 0};

    SheetNode *node = socket->node;

    if (!socket->isInput) {

        // Due to the calling convention, the argument should already be on the
        // stack, but relative to the frame pointer. We just need to figure out
        // which index.
        fimmediate_t index      = 0;
        fimmediate_t numOutputs = 0;

        for (size_t i = 0; i < node->numSockets; i++) {
            SheetSocket *testSocket = node->sockets[i];

            if (!testSocket->isInput) {
                // Sockets are separately malloc'd.
                if (socket == testSocket) {
                    index = numOutputs;
                    break;
                }

                numOutputs++;
            }
        }

        out = d_bytecode_ins(OP_GETFI);
        d_bytecode_set_fimmediate(out, 1, index + 1);

        // It is up to the caller to set the socket's stack index,
        // as it should not be set here, since it should be copied.

        context->stackTop++;
    }

    return out;
}

/**
 * \fn BCode d_generate_return(SheetNode *returnNode, BuildContext *context)
 * \brief Given a Return node, generate the bytecode to return from the
 * function/subroutine with the return values.
 *
 * \return Bytecode to return from the function/subroutine.
 *
 * \param returnNode The Return node to return with.
 * \param context The context needed to generate the bytecode.
 */
BCode d_generate_return(SheetNode *returnNode, BuildContext *context) {
    SheetFunction *function = returnNode->definition.function;

    VERBOSE(5, "Generating bytecode to return from %s...\n", function->name);

    BCode out = {NULL, 0};

    if (function->numReturns == 0) {
        out = d_bytecode_ins(OP_RET);
    } else {
        out = d_push_node_inputs(returnNode, context, false, false, false);

        BCode ret = d_bytecode_ins(OP_RETN);
        d_bytecode_set_byte(ret, 1, (char)function->numReturns);
        d_concat_bytecode(&out, &ret);
        d_free_bytecode(&ret);
    }

    return out;
}

/**
 * \fn BCode d_generate_nonexecution_node(SheetNode *node,
 *                                        BuildContext *context)
 * \brief Given a non-execution node, generate the bytecode to get the output.
 *
 * \return Bytecode to run the nonexecution node's function.
 *
 * \param node The non-execution node.
 * \param context The context needed to generate the bytecode.
 */
BCode d_generate_nonexecution_node(SheetNode *node, BuildContext *context) {
    VERBOSE(5, "- Generating bytecode for non-execution node %s...\n",
            node->name);

    BCode out = {NULL, 0};

    // Firstly, we need to check if the node is a particular function -
    // spoiler alert, one of them is not like the others...
    const CoreFunction coreFunc = d_core_find_name(node->name);

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
        SheetSocket *boolSocket = node->sockets[0];

        // If the boolean input is a literal, we only need to generate bytecode
        // for the input that is active, since that is all that is ever gonna
        // be picked!
        bool boolIsLiteral    = (boolSocket->numConnections == 0);
        bool boolLiteralValue = boolSocket->defaultValue.booleanValue;

        BCode boolCode = d_push_input(boolSocket, context, false);

        // The problem with this node is that either the true bytecode or false
        // bytecode will get run, which means the top of the stack can be in
        // different positions by the end of the node's bytecode.
        // This means if one of the true or false bytecodes has a lesser stack
        // top than the other, we need to add pushes such that both bytecodes
        // end with the same stack top.
        int stackTopBefore = context->stackTop;

        // Next, get the bytecode for the true input.
        SheetSocket *trueSocket = node->sockets[1];
        BCode trueCode          = {NULL, 0};

        if (!boolIsLiteral || boolLiteralValue) {
            trueCode = d_push_input(trueSocket, context, false);
        }

        int stackTopTrue = context->stackTop;

        // Reset the stack top for the false bytecode.
        context->stackTop = stackTopBefore;

        // Finally, get the bytecode for the false input.
        SheetSocket *falseSocket = node->sockets[2];
        BCode falseCode          = {NULL, 0};

        if (!boolIsLiteral || !boolLiteralValue) {
            falseCode = d_push_input(falseSocket, context, false);
        }

        int stackTopFalse = context->stackTop;

        // Get what the final stack top will be, and get the other bytecode to
        // push as many times as needed to make up the difference.
        int finalStackTop = stackTopFalse;
        BCode pushn       = {NULL, 0};

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

        SheetSocket *outputSocket = node->sockets[3];
        outputSocket->_stackIndex = finalStackTop;

        // If the boolean input is a literal, then it's easy! We just output
        // the bytecode that gives us the output corresponding to if the
        // literal is true or false.
        if (boolIsLiteral) {
            BCode *append = (boolLiteralValue) ? &trueCode : &falseCode;
            d_concat_bytecode(&out, append);
            d_free_bytecode(append);

            // Setting the output socket stack index.
            SheetSocket *inputSocket =
                (boolLiteralValue) ? trueSocket : falseSocket;
            outputSocket->_stackIndex = inputSocket->_stackIndex;

        } else {
            // Wise guy, eh? So we need to include the bytecode for both the
            // true AND the false inputs, and only run the correct one
            // depending on the value of the boolean input.
            d_concat_bytecode(&out, &boolCode);
            d_free_bytecode(&boolCode);

            // Since the boolean is going to get poped off by the JRCONFI
            // instruction, copy it on the stack in case other nodes still need
            // its value.
            if (node->sockets[0]->connections[0]->numConnections > 1) {
                BCode copyBool = d_bytecode_ins(OP_GETFI);
                d_bytecode_set_fimmediate(copyBool, 1, 0);
                d_concat_bytecode(&out, &copyBool);
                d_free_bytecode(&boolCode);
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

            d_concat_bytecode(&out, &jumpToCode);
            d_free_bytecode(&jumpToCode);

            // Add the false code, and the true code afterwards as well.
            d_concat_bytecode(&out, &falseCode);
            d_free_bytecode(&falseCode);

            d_concat_bytecode(&out, &trueCode);
            d_free_bytecode(&trueCode);
        }
    } else {
        BCode action = {NULL, 0};

        if ((int)coreFunc >= 0) {
            // Generate bytecode depending on the core function.
            // Remember that it's only non-execution functions we care about.
            switch (coreFunc) {
                case CORE_ADD:;
                    action = d_generate_operator(node, context, OP_ADD, OP_ADDF,
                                                 OP_ADDFI, false);
                    break;
                case CORE_AND:;
                    action = d_generate_operator(node, context, OP_AND, 0,
                                                 OP_ANDFI, false);
                    break;
                case CORE_DIV:
                case CORE_DIVIDE:;
                    action =
                        d_generate_operator(node, context, OP_DIV, OP_DIVF,
                                            OP_DIVFI, coreFunc == CORE_DIVIDE);

                    if (coreFunc == CORE_DIV) {
                        if (node->sockets[0]->type == TYPE_FLOAT ||
                            node->sockets[1]->type == TYPE_FLOAT) {
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
                    action = d_generate_comparator(node, context, OP_CEQ,
                                                   OP_CEQF, 0, false);
                    break;
                case CORE_MULTIPLY:;
                    action = d_generate_operator(node, context, OP_MUL, OP_MULF,
                                                 OP_MULFI, false);
                    break;
                case CORE_LENGTH:;
                    action = d_push_input(node->sockets[0], context, false);

                    // Here we will use the SYS_STRLEN syscall.
                    BCode pushArgs = d_bytecode_ins(OP_PUSHNF);
                    d_bytecode_set_fimmediate(pushArgs, 1, 2);
                    d_concat_bytecode(&action, &pushArgs);
                    d_free_bytecode(&pushArgs);

                    BCode syscall = d_bytecode_ins(OP_SYSCALL);
                    d_bytecode_set_byte(syscall, 1, SYS_STRLEN);
                    d_concat_bytecode(&action, &syscall);
                    d_free_bytecode(&syscall);

                    node->sockets[1]->_stackIndex = context->stackTop;
                    break;
                case CORE_LESS_THAN:;
                    action = d_generate_comparator(node, context, OP_CLT,
                                                   OP_CLTF, 2, false);
                    break;
                case CORE_LESS_THAN_OR_EQUAL:;
                    action = d_generate_comparator(node, context, OP_CLEQ,
                                                   OP_CLEQF, 1, false);
                    break;
                case CORE_MOD:;
                    action = d_generate_operator(node, context, OP_MOD, 0,
                                                 OP_MODFI, false);
                    break;
                case CORE_MORE_THAN:;
                    action = d_generate_comparator(node, context, OP_CMT,
                                                   OP_CMTF, 4, false);
                    break;
                case CORE_MORE_THAN_OR_EQUAL:;
                    action = d_generate_comparator(node, context, OP_CMEQ,
                                                   OP_CMEQF, 3, false);
                    break;
                case CORE_NOT:;
                    action =
                        d_generate_operator(node, context, OP_NOT, 0, 0, false);
                    break;
                case CORE_NOT_EQUAL:;
                    action = d_generate_comparator(node, context, OP_CEQ,
                                                   OP_CEQF, 0, true);
                    break;
                case CORE_OR:;
                    action = d_generate_operator(node, context, OP_OR, 0,
                                                 OP_ORFI, false);
                    break;
                case CORE_SUBTRACT:;
                    action = d_generate_operator(node, context, OP_SUB, OP_SUBF,
                                                 OP_SUBFI, false);
                    break;
                case CORE_TERNARY:;
                    // Ternary is a special snowflake when it comes to
                    // non-execution nodes...
                    // See above.
                    break;
                case CORE_XOR:;
                    action = d_generate_operator(node, context, OP_XOR, 0,
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
            if (node->definition.type == NAME_VARIABLE) {
                action = d_push_variable(node, context);
            } else if (node->definition.type == NAME_FUNCTION) {
                action = d_generate_call(node, context);
            } else if (node->definition.type == NAME_CFUNCTION) {
                action = d_generate_call(node, context);
            }
        }

        d_concat_bytecode(&out, &action);
        d_free_bytecode(&action);
    }

    return out;
}

/**
 * \fn BCode d_generate_execution_node(SheetNode *node, BuildContext* context,
 *                                     bool retAtEnd)
 * \brief Given an execution node, generate the bytecode to get the output.
 *
 * \return Bytecode to run the execution node's subroutine.
 *
 * \param node The execution node.
 * \param context The context needed to generate the bytecode.
 * \param retAtEnd Should the bytecode return at the end?
 */
BCode d_generate_execution_node(SheetNode *node, BuildContext *context,
                                bool retAtEnd) {
    VERBOSE(5, "- Generating bytecode for execution node %s...\n", node->name);

    int stackTopBeforeInputs = context->stackTop;

    // Firstly, we need the bytecode to get the inputs, such that the first
    // input is at the top of the stack.
    BCode out = d_push_node_inputs(node, context, false, false, false);

    int stackTopAfterInputs = context->stackTop;

    // Secondly, we use the inputs to perform an action.
    BCode action = {NULL, 0};

    const CoreFunction coreFunc = d_core_find_name(node->name);

    // We don't want to have multiple returns at the end, so if we know we've
    // added one, then there is no need to add another.
    bool addedReturn = false;

    if ((int)coreFunc >= 0) {
        // Generate bytecode depending on the core function.
        // Remember that it's only execution functions we care about.
        SheetSocket *socket;
        SheetNode *firstNode;

        switch (coreFunc) {
            case CORE_FOR:;
                // TODO: This.
                /*
                // The "index" output.
                SheetSocket *indexSocket = node->sockets[5];

                // We just need to check if our output is a float.
                bool indexIsFloat = (indexSocket->type == TYPE_FLOAT);

                // We also want to make sure our registers are "safe", if the
                // iteration uses calls.
                socket                        = node->sockets[4];
                SheetNode *iterationStartNode = NULL;
                bool useSafeRegisters         = false;

                if (socket->numConnections == 1) {
                    socket             = socket->connections[0];
                    iterationStartNode = socket->node;

                    useSafeRegisters =
                        d_does_output_involve_call(iterationStartNode);
                }

                // The "start" input.
                socket = node->sockets[1];
                d_setup_input(socket, context, indexIsFloat, useSafeRegisters,
                              &action);
                reg_t indexReg = socket->_reg;

                // Set the register of the "index" output, so other nodes
                // can get it's value.
                indexSocket->_reg = indexReg;

                // The "end" input.
                socket = node->sockets[2];
                d_setup_input(socket, context, indexIsFloat, useSafeRegisters,
                              &action);
                reg_t endReg = socket->_reg;

                // The "step" input.
                // Optimize: If the step is constant, don't bother putting it
                // into a register. It's only going to get used in one ADD
                // instruction. If it's a float, or it can't fit into an
                // immediate, then we don't have a choice but to put it into a
                // register.
                SheetSocket *stepSocket = node->sockets[3];
                bool stepImmediate =
                    stepSocket->numConnections == 0 && !indexIsFloat &&
                    ((stepSocket->defaultValue.integerValue &
                      IMMEDIATE_UPPER_MASK) == 0 ||
                     (stepSocket->defaultValue.integerValue &
                      IMMEDIATE_UPPER_MASK) == IMMEDIATE_UPPER_MASK);

                dint stepImmediateVal =
                    0;             // Only defined if it's a literal int.
                reg_t stepReg = 0; // Only defined otherwise.

                if (stepImmediate) {
                    stepImmediateVal = stepSocket->defaultValue.integerValue;
                } else {
                    d_setup_input(stepSocket, context, indexIsFloat,
                                  useSafeRegisters, &action);
                    stepReg = stepSocket->_reg;
                }

                // The "iteration" socket, which gets executed each pass.
                BCode pass = (BCode){NULL, 0};
                if (iterationStartNode != NULL) {
                    pass = d_generate_bytecode_for_execution_node(
                        iterationStartNode, context, false, true);
                }

                d_concat_bytecode(&action, &pass);

                // We need another register to determine if we are still in
                // the for loop or not.
                // We can immediately free it again since it's only going to
                // be used for this node only.
                reg_t inLoopReg = d_next_general_reg(context, false);
                d_free_reg(context, inLoopReg);

                // Which operator do we use to determine if we're done?
                // If the step value is variable, then we'll need to hard-code
                // which to use into the bytecode.
                DIns compareOp             = (indexIsFloat) ? OP_CMTF : OP_CMT;
                bool canDetermineCompareOp = false;

                if (stepSocket->numConnections == 0) {
                    canDetermineCompareOp = true;

                    if (stepSocket->type == TYPE_INT) {
                        if (stepSocket->defaultValue.integerValue < 0)
                            compareOp = (indexIsFloat) ? OP_CLTF : OP_CLT;
                    } else {
                        if (stepSocket->defaultValue.floatValue < 0.0)
                            compareOp = (indexIsFloat) ? OP_CLTF : OP_CLT;
                    }
                }

                // TODO: Check if the jump amount is too big, i.e. we cannot
                // add to the index with an immediate.

                DIns addOpcode = OP_ADDI;
                if (!stepImmediate) {
                    addOpcode = (indexIsFloat) ? OP_ADDF : OP_ADD;
                }

                BCode afterPass = d_malloc_bytecode(d_vm_ins_size(addOpcode));

                // Add the step value onto the index.
                d_bytecode_set_byte(afterPass, 0, addOpcode);

                if (stepImmediate) {
                    d_bytecode_set_byte(afterPass, 1, (char)indexReg);
                    d_bytecode_set_immediate(
                        afterPass, 2,
                        (immediate_t)(stepImmediateVal & IMMEDIATE_MASK));
                } else {
                    d_bytecode_set_byte(afterPass, 1, (char)indexReg);
                    d_bytecode_set_byte(afterPass, 2, (char)stepReg);
                }

                dint jumpBackAmt =
                    -((dint)pass.size + d_vm_ins_size(addOpcode));

                DIns firstCmpOp   = compareOp;
                BCode firstGoBack = d_malloc_bytecode(
                    d_vm_ins_size(firstCmpOp) + d_vm_ins_size(OP_JRCON) +
                    d_vm_ins_size(OP_JR));

                if (!canDetermineCompareOp) {

                    // NOTE: firstCmpOp is the opcode used if the step counter
                    // is >= 0, otherwise secondCmpOp is the opcode used.
                    DIns secondCmpOp   = (indexIsFloat) ? OP_CLTF : OP_CLT;
                    BCode secondGoBack = d_malloc_bytecode(
                        d_vm_ins_size(secondCmpOp) + d_vm_ins_size(OP_JRCON) +
                        d_vm_ins_size(OP_JR));

                    DIns decideOp   = (indexIsFloat) ? OP_CMEQF : OP_CMEQ;
                    BCode decideCmp = d_malloc_bytecode(
                        d_vm_ins_size(OP_LOADI) + d_vm_ins_size(decideOp) +
                        d_vm_ins_size(OP_JRCON));

                    // NOTE: We are using the inLoopReg to both determine which
                    // comparison to use, as well as to check if we are still
                    // in the loop.

                    // decideCmp (LOADI)
                    d_bytecode_set_byte(decideCmp, 0, OP_LOADI);
                    d_bytecode_set_byte(decideCmp, 1, (char)inLoopReg);
                    d_bytecode_set_immediate(decideCmp, 2, 0);

                    // decideCmp (decideOp)
                    d_bytecode_set_byte(decideCmp, d_vm_ins_size(OP_LOADI),
                                        decideOp);
                    d_bytecode_set_byte(decideCmp, d_vm_ins_size(OP_LOADI) + 1,
                                        (char)inLoopReg);
                    d_bytecode_set_byte(decideCmp, d_vm_ins_size(OP_LOADI) + 2,
                                        (char)stepReg);
                    d_bytecode_set_byte(decideCmp, d_vm_ins_size(OP_LOADI) + 3,
                                        (char)inLoopReg);

                    // decideCmp (JRCON)
                    d_bytecode_set_byte(decideCmp,
                                        d_vm_ins_size(OP_LOADI) +
                                            d_vm_ins_size(decideOp),
                                        OP_JRCON);
                    d_bytecode_set_byte(decideCmp,
                                        d_vm_ins_size(OP_LOADI) +
                                            d_vm_ins_size(decideOp) + 1,
                                        (char)inLoopReg);
                    d_bytecode_set_immediate(
                        decideCmp,
                        d_vm_ins_size(OP_LOADI) + d_vm_ins_size(decideOp) + 2,
                        d_vm_ins_size(OP_JRCON) +
                            (immediate_t)secondGoBack.size);

                    // secondGoBack (secondCmpOp)
                    d_bytecode_set_byte(secondGoBack, 0, secondCmpOp);
                    d_bytecode_set_byte(secondGoBack, 1, (char)inLoopReg);
                    d_bytecode_set_byte(secondGoBack, 2, (char)indexReg);
                    d_bytecode_set_byte(secondGoBack, 3, (char)endReg);

                    // secondGoBack (JRCON)
                    d_bytecode_set_byte(secondGoBack,
                                        d_vm_ins_size(secondCmpOp), OP_JRCON);
                    d_bytecode_set_byte(secondGoBack,
                                        d_vm_ins_size(secondCmpOp) + 1,
                                        (char)inLoopReg);
                    d_bytecode_set_immediate(
                        secondGoBack, d_vm_ins_size(secondCmpOp) + 2,
                        d_vm_ins_size(OP_JRCON) + d_vm_ins_size(OP_JR) +
                            (immediate_t)firstGoBack.size);

                    jumpBackAmt -=
                        (decideCmp.size + d_vm_ins_size(secondCmpOp) +
                         d_vm_ins_size(OP_JRCON));

                    // secondGoBack (JR)
                    d_bytecode_set_byte(secondGoBack,
                                        d_vm_ins_size(secondCmpOp) +
                                            d_vm_ins_size(OP_JRCON),
                                        OP_JR);
                    d_bytecode_set_immediate(secondGoBack,
                                             d_vm_ins_size(secondCmpOp) +
                                                 d_vm_ins_size(OP_JRCON) + 1,
                                             (jumpBackAmt & IMMEDIATE_MASK));

                    // Need to take the JR into account for when firstGoBack
                    // calculates how many bytes it needs to go back.
                    jumpBackAmt -= d_vm_ins_size(OP_JR);

                    d_concat_bytecode(&afterPass, &decideCmp);
                    d_free_bytecode(&decideCmp);

                    d_concat_bytecode(&afterPass, &secondGoBack);
                    d_free_bytecode(&secondGoBack);
                }

                // firstGoBack (firstCmpOp)
                d_bytecode_set_byte(firstGoBack, 0, firstCmpOp);
                d_bytecode_set_byte(firstGoBack, 1, (char)inLoopReg);
                d_bytecode_set_byte(firstGoBack, 2, (char)indexReg);
                d_bytecode_set_byte(firstGoBack, 3, (char)endReg);

                // firstGoBack (JRCON)
                d_bytecode_set_byte(firstGoBack, d_vm_ins_size(firstCmpOp),
                                    OP_JRCON);
                d_bytecode_set_byte(firstGoBack, d_vm_ins_size(firstCmpOp) + 1,
                                    (char)inLoopReg);
                d_bytecode_set_immediate(
                    firstGoBack, d_vm_ins_size(firstCmpOp) + 2,
                    (dint)d_vm_ins_size(OP_JRCON) + d_vm_ins_size(OP_JR));

                jumpBackAmt -=
                    (d_vm_ins_size(firstCmpOp) + d_vm_ins_size(OP_JRCON));

                // firstGoBack (JR)
                d_bytecode_set_byte(
                    firstGoBack,
                    d_vm_ins_size(firstCmpOp) + d_vm_ins_size(OP_JRCON), OP_JR);
                d_bytecode_set_immediate(firstGoBack,
                                         d_vm_ins_size(firstCmpOp) +
                                             d_vm_ins_size(OP_JRCON) + 1,
                                         (jumpBackAmt & IMMEDIATE_MASK));

                d_concat_bytecode(&afterPass, &firstGoBack);
                d_free_bytecode(&firstGoBack);

                d_concat_bytecode(&action, &afterPass);
                d_free_bytecode(&afterPass);
                d_free_bytecode(&pass);

                d_free_reg(context, indexReg);
                d_free_reg(context, endReg);
                if (!stepImmediate)
                    d_free_reg(context, stepReg);
                */
                break;

            case CORE_IF_THEN:
            case CORE_IF_THEN_ELSE:;
                BCode thenBranch = (BCode){NULL, 0};
                BCode elseBranch = (BCode){NULL, 0};

                int initStackTop = context->stackTop;

                // Get the individual branches compiled.
                socket = node->sockets[2];
                if (socket->numConnections == 1) {
                    firstNode = socket->connections[0]->node;
                    thenBranch =
                        d_generate_execution_node(firstNode, context, false);
                }

                int thenTopDiff = context->stackTop - initStackTop;

                // Reset the context stack top.
                context->stackTop = initStackTop;

                if (coreFunc == CORE_IF_THEN_ELSE) {
                    socket = node->sockets[3];
                    if (socket->numConnections == 1) {
                        firstNode  = socket->connections[0]->node;
                        elseBranch = d_generate_execution_node(firstNode,
                                                               context, false);
                    }
                }

                int elseTopDiff = context->stackTop - initStackTop;

                // We want both branches to have the same stack top. So if one
                // branch has a bigger stack top, pop it enough so it's stack
                // top is equal to the other.
                int finalTop = initStackTop + thenTopDiff;

                if (thenTopDiff > elseTopDiff) {
                    finalTop = initStackTop + elseTopDiff;

                    fimmediate_t numPop = thenTopDiff - elseTopDiff;

                    BCode popExtra = d_bytecode_ins(OP_POPF);
                    d_bytecode_set_fimmediate(popExtra, 1, numPop);
                    d_concat_bytecode(&thenBranch, &popExtra);
                    d_free_bytecode(&popExtra);
                } else if (thenTopDiff < elseTopDiff) {
                    fimmediate_t numPop = elseTopDiff - thenTopDiff;

                    BCode popExtra = d_bytecode_ins(OP_POPF);
                    d_bytecode_set_fimmediate(popExtra, 1, numPop);
                    d_concat_bytecode(&elseBranch, &popExtra);
                    d_free_bytecode(&popExtra);
                }

                context->stackTop = finalTop;

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

                BCode conAtEndElse = (BCode){NULL, 0};

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
                socket = node->sockets[1];

                // The value to print should already be at the top of the stack.

                // Setup the arguments for this syscall.
                // TODO: Add a way to not print a newline.
                action = d_bytecode_ins(OP_PUSHF);
                d_bytecode_set_fimmediate(action, 1, 1);

                fimmediate_t typeArg = 0;
                switch (socket->type) {
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
                SheetVariable *var = node->definition.variable;

                // Depending on the variable type, we need to use different
                // opcodes and link types.
                size_t varSize =
                    (var->dataType == TYPE_BOOL) ? 1 : sizeof(dint);
                DIns storeAdr     = (varSize == 1) ? OP_SETADRB : OP_SETADR;
                LinkType linkType = (var->dataType == TYPE_STRING)
                                        ? LINK_VARIABLE_POINTER
                                        : LINK_VARIABLE;

                // Push the address of the variable onto the stack.
                // This will be linked later.
                action = d_bytecode_ins(OP_PUSHF);

                // TODO: Implement copying strings.
                if (var->dataType == TYPE_STRING) {
                }

                // At this point the address of the variable should be at the
                // top of the stack, and the new value should be one below.
                BCode store = d_bytecode_ins(storeAdr);
                d_concat_bytecode(&action, &store);
                d_free_bytecode(&store);

                context->stackTop--;

                // Now set up the link to the variable.
                LinkMeta varMeta = d_link_new_meta(linkType, var->name, var);

                size_t _index;
                bool _wasDuplicate;
                d_add_link_to_ins(context, &action, 0, varMeta, &_index,
                                  &_wasDuplicate);

                break;

            case CORE_WHILE:;
                // Get the boolean socket.
                socket = node->sockets[1];

                // Check to see if it is a false literal. If it is, then this
                // execution node is useless.
                if (socket->numConnections == 0 &&
                    socket->defaultValue.booleanValue == false) {
                    break;
                }

                // Now generate the bytecode for the code that executes if the
                // condition is true.
                socket    = node->sockets[2];
                firstNode = socket->connections[0]->node;

                BCode trueCode =
                    d_generate_execution_node(firstNode, context, false);

                // After the loop has executed, we want to pop from the stack
                // to the point before the boolean variable got calculated,
                // and jump back far enough as to recalculate the boolean value.
                fimmediate_t numPop = context->stackTop - stackTopBeforeInputs;
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
                    trueCode.size + d_vm_ins_size(OP_JRCON);

                BCode jmpOver = d_bytecode_ins(OP_JRCONFI);
                d_bytecode_set_fimmediate(jmpOver, 1, jmpOverAmt);
                d_concat_bytecode(&action, &jmpOver);
                d_free_bytecode(&jmpOver);

                d_concat_bytecode(&action, &trueCode);
                d_free_bytecode(&trueCode);

                // No matter if the while loop didn't activate once, or it
                // activated multiple times, there should be only one "instance"
                // of the boolean being calculated on the stack.
                context->stackTop = stackTopAfterInputs;

                break;

            default:
                break;
        }
    }
    // TODO: Add define here for safety.
    else if (strcmp(node->name, "Return") == 0) {
        action      = d_generate_return(node, context);
        addedReturn = true;
    } else {
        // Put arguments into the stack and call the subroutine.
        action = d_generate_call(node, context);
    }

    d_concat_bytecode(&out, &action);
    d_free_bytecode(&action);

    // Now, in order to optimise the usage of the stack, we pop from the stack
    // back to before the inputs were generated. We've used the inputs in this
    // node, we don't need them anymore!
    fimmediate_t numPop = context->stackTop - stackTopBeforeInputs;
    if (numPop < 0) {
        numPop = 0;
    }

    BCode popBack = d_bytecode_ins(OP_POPF);
    d_bytecode_set_fimmediate(popBack, 1, numPop);
    d_concat_bytecode(&out, &popBack);
    d_free_bytecode(&popBack);

    context->stackTop = stackTopBeforeInputs;

    // Thirdly, we generate the bytecode for the next execution node.
    BCode nextCode = {NULL, 0};

    SheetSocket *lastExecSocket = NULL;
    if (node->sockets != NULL && node->numSockets > 0) {
        for (size_t i = 0; i < node->numSockets; i++) {
            SheetSocket *socket = node->sockets[i];

            // We're only interested in the LAST output execution socket.
            if (!socket->isInput && socket->type == TYPE_EXECUTION) {
                lastExecSocket = socket;
            }
        }
    }

    bool isNextNode = false;
    if (lastExecSocket != NULL) {
        if (lastExecSocket->numConnections == 1) {
            isNextNode                     = true;
            SheetSocket *socketConnectedTo = lastExecSocket->connections[0];
            SheetNode *nodeConnectedTo     = socketConnectedTo->node;

            // Recursively call d_generate_execution_node to generate the
            // bytecode after this node's bytecode.
            nextCode =
                d_generate_execution_node(nodeConnectedTo, context, retAtEnd);
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
 * \fn BCode d_generate_start(SheetNode *startNode, BuildContext *context)
 * \brief Given a Start node, generate the bytecode for the sequence starting
 * from this node.
 *
 * \return The bytecode generated for the Start function.
 *
 * \param startNode A pointer to the Start node.
 * \param context The context needed to generate the bytecode.
 */
BCode d_generate_start(SheetNode *startNode, BuildContext *context) {
    // As the first instruction, place a RET, which acts as a safety barrier
    // for any function that is defined before this one (this ensures the
    // previous function doesn't "leak" into this one).
    BCode out = d_bytecode_ins(OP_RET);

    // In terms of generating the bytecode from the Start node, this is easy.
    // We just need to call the bytecode generation functions starting from the
    // first node after Start.

    if (startNode->numSockets == 1) {
        SheetSocket *socket = startNode->sockets[0];

        if (socket->numConnections == 1) {
            VERBOSE(5, "-- Generating bytecode for Start function...\n");

            socket = socket->connections[0];

            BCode exe = d_generate_execution_node(socket->node, context, true);

            d_concat_bytecode(&out, &exe);
            d_free_bytecode(&exe);
        }
    }

    return out;
}

/**
 * \fn BCode d_generate_function(SheetFunction *func, BuildContext *context)
 * \brief Given a function, generate the bytecode for it.
 *
 * \return The bytecode generated for the function.
 *
 * \param func The function to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 */
BCode d_generate_function(SheetFunction *func, BuildContext *context) {
    // As the first instruction, place a RET, which allows linkers to link to
    // this instruction, since the program counter of the virtual machine will
    // ALWAYS increment, it will increment to the actual first instruction of
    // the function.
    // TODO: Is this still true?
    BCode out = d_bytecode_ins(OP_RET);

    // Set the top of the stack to be accurate, i.e. there will be arguments
    // pushed in before the function runs.
    context->stackTop = func->numArguments - 1;

    if (func->isSubroutine) {
        if (func->defineNode != NULL) {
            VERBOSE(5, "-- Generating bytecode for subroutine %s...\n",
                    func->name);

            // If it's a subroutine, we can just recursively go through the
            // sequence to the Return.
            // NOTE: The first argument should be the name of the function.
            SheetSocket *socket = func->defineNode->sockets[1];

            if (socket->numConnections == 1 && socket->type == TYPE_EXECUTION) {
                socket = socket->connections[0];

                BCode exe =
                    d_generate_execution_node(socket->node, context, true);

                d_concat_bytecode(&out, &exe);
                d_free_bytecode(&exe);
            }
        }
    } else {
        VERBOSE(5, "-- Generating bytecode for function %s...\n", func->name);

        // Otherwise it's a function. In which case we need to find the
        // Return node for it and recurse backwards through the inputs.
        // NOTE: Functions (unlike subroutines) should only have one Return
        // node.
        SheetNode *returnNode = func->lastReturnNode;

        if (returnNode != NULL) {
            // Now we recursively generate the bytecode for the inputs
            // of the Return node, so the final Return values are
            // calculated.
            BCode funcCode = d_generate_return(returnNode, context);

            d_concat_bytecode(&out, &funcCode);
            d_free_bytecode(&funcCode);
        }
    }

    return out;
}

/**
 * \fn void d_codegen_compile(Sheet *sheet)
 * \brief Given that Semantic Analysis has taken place, generate the bytecode
 * for a given sheet.
 *
 * \param sheet The sheet to generate the bytecode for.
 */
void d_codegen_compile(Sheet *sheet) {

    BCode text = (BCode){NULL, 0};

    // Create a context object for the build.
    BuildContext context;
    context.stackTop = -1;

    context.linkMetaList = d_link_new_meta_list();

    context.dataSection     = NULL;
    context.dataSectionSize = 0;

    // Start off by allocating space for the variables defined in the sheet.
    for (size_t i = 0; i < sheet->numVariables; i++) {
        SheetVariable *var = sheet->variables + i;

        // Copy the name so d_link_free_list doesn't go balistic.
        size_t nameLengthPlusNull = strlen(var->name) + 1;
        char *varName             = (char *)d_malloc(nameLengthPlusNull);
        memcpy(varName, var->name, nameLengthPlusNull);

        // Create a LinkMeta entry so we know it exists.
        LinkMeta linkMeta = d_link_new_meta((var->dataType == TYPE_STRING)
                                                ? LINK_VARIABLE_POINTER
                                                : LINK_VARIABLE,
                                            varName, var);

        d_link_meta_list_push(&(context.linkMetaList), linkMeta);

        // Allocate space in the data section for the variable to fit in.
        d_allocate_variable(&context, var,
                            (var->dataType == TYPE_BOOL) ? 1 : sizeof(dint),
                            context.linkMetaList.size - 1);
    }

    // Next, generate bytecode for the custom functions!
    for (size_t i = 0; i < sheet->numFunctions; i++) {
        SheetFunction *func = sheet->functions + i;

        BCode code = d_generate_function(func, &context);

        // Before we concatenate the bytecode, we need to take a note of where
        // it will go in the text section, so the linker knows where to call to
        // when other functions call this one.

        // Check that the metadata doesn't already exist.
        LinkMeta meta = d_link_new_meta(LINK_FUNCTION, func->name, func);

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
    if (sheet->startNode != NULL) {
        BCode start = d_generate_start(sheet->startNode, &context);

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

    // Put the link metadata into the sheet.
    sheet->_link = context.linkMetaList;

    // Put the relational records of instructions to links into the sheet.
    sheet->_insLinkList     = text.linkList;
    sheet->_insLinkListSize = text.linkListSize;

    // TODO: Put checks in other functions to make sure the sheet has
    // bytecode in first.
    sheet->_isCompiled = true;
}
