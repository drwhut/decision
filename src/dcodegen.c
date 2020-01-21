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
 * \fn void d_setup_input(SheetSocket *socket, BuildContext *context,
 *                        bool forceFloat, BCode *addTo)
 * \brief Given an input socket, do what is nessesary to set it up for use in a
 * node.
 *
 * \param socket The socket corresponding to the input.
 * \param context The context needed to build bytecode.
 * \param forceFloat Always convert the input to a float if it isn't already.
 * \param addTo If any extra bytecode is needed to setup the input, add it onto
 * this bytecode.
 */
/*
void d_setup_input(struct _sheetSocket *socket, BuildContext *context,
                   bool forceFloat, BCode *addTo) {
    // The input is a literal.
    if (socket->numConnections == 0) {
        BCode literal =
            d_generate_bytecode_for_literal(socket, context, forceFloat);
        d_concat_bytecode(addTo, &literal);
        d_free_bytecode(&literal);
    }
}
*/

/**
 * \fn void d_setup_arguments(SheetNode *defineNode, BuildContext *context,
 *                            BCode *addTo, bool isSubroutine)
 * \brief Given a Define node, generate bytecode to pop the arguments from the
 * stack.
 *
 * The node's sockets are provided with their registers.
 *
 * \param defineNode The Define node to get the arguments of.
 * \param context The context needed to generate the bytecode.
 * \param addTo Where to add the extra bytecode onto.
 * \param isSubroutine Info needed to make sure the execution socket is not
 * "poped".
 */
/*
void d_setup_arguments(SheetNode *defineNode, BuildContext *context,
                       BCode *addTo, bool isSubroutine) {
    VERBOSE(5, "Generating bytecode to setup arguments...\n");
    // The first socket is the name, so skip that.
    for (size_t i = 1 + (size_t)isSubroutine; i < defineNode->numSockets; i++) {
        SheetSocket *argSocket = defineNode->sockets[i];
        reg_t argReg           = d_next_general_reg(context, false);

        BCode argPop = d_malloc_bytecode(d_vm_ins_size(OP_POP));
        d_bytecode_set_byte(argPop, 0, OP_POP);
        d_bytecode_set_byte(argPop, 1, (char)argReg);

        // Set the socket's register value.
        argSocket->_reg = argReg;

        // Do we need to move the value into a float register?
        if (argSocket->type == TYPE_FLOAT) {
            BCode mvtf = d_convert_between_number_types(argSocket, context,
                                                        OP_MVTF, false);

            d_concat_bytecode(&argPop, &mvtf);
            d_free_bytecode(&mvtf);
        }

        d_concat_bytecode(addTo, &argPop);
        d_free_bytecode(&argPop);
    }
}
*/

/**
 * \fn void d_setup_returns(SheetNode *returnNode, BuildContext *context,
 *                          BCode *addTo, bool isSubroutine, bool retAtEnd)
 * \brief Given a Return node, generate bytecode to push the return values onto
 * the stack.
 *
 * If you want, a RET instruction is also added at the end.
 *
 * \param returnNode The Return node to get the return values from.
 * \param context The context needed to generate the bytecode.
 * \param addTo Where to add the extra bytecode onto.
 * \param isSubroutine Info needed to make sure the execution socket is not
 * "pushed".
 * \param retAtEnd If true, this adds a RET instruction after.
 */
/*
void d_setup_returns(SheetNode *returnNode, BuildContext *context, BCode *addTo,
                     bool isSubroutine, bool retAtEnd) {
    VERBOSE(5, "Generating bytecode to return values...\n");

    // We want to ensure that when we push return values onto the stack, they
    // are pushed from unsafe registers - this is because safe register contents
    // were pushed onto the stack after we poped the arguments. So, we need to
    // pop those contents back to their respective safe registers BEFORE we push
    // the return values. So for safety, we are yeeting the return values that
    // live in safe registers to unsafe ones.
    for (size_t i = returnNode->numSockets - 1; i >= 1 + (size_t)isSubroutine;
         i--) {
        SheetSocket *retSocket = returnNode->sockets[i];

        if (IS_REG_SAFE(retSocket->_reg) && retSocket->numConnections > 0) {
            reg_t newUnsafeReg = 0;
            BCode moveCode;

            if (IS_REG_SAFE_GENERAL(retSocket->_reg)) {
                newUnsafeReg = d_next_general_reg(context, false);

                moveCode = d_malloc_bytecode(d_vm_ins_size(OP_LOAD));
                d_bytecode_set_byte(moveCode, 0, OP_LOAD);
                d_bytecode_set_byte(moveCode, 1, (char)newUnsafeReg);
                d_bytecode_set_byte(moveCode, 2, (char)retSocket->_reg);

                d_concat_bytecode(addTo, &moveCode);
                d_free_bytecode(&moveCode);
            } else if (IS_REG_SAFE_FLOAT(retSocket->_reg)) {
                newUnsafeReg = d_next_float_reg(context, false);

                moveCode = d_malloc_bytecode(d_vm_ins_size(OP_LOADF));
                d_bytecode_set_byte(moveCode, 0, OP_LOADF);
                d_bytecode_set_byte(moveCode, 1, (char)newUnsafeReg);
                d_bytecode_set_byte(moveCode, 2, (char)retSocket->_reg);

                d_concat_bytecode(addTo, &moveCode);
                d_free_bytecode(&moveCode);
            }

            d_free_reg(context, retSocket->_reg);
            retSocket->_reg = newUnsafeReg;
        }
    }

    // We want to add a marker afterwards for where the setup of the return
    // values starts.
    size_t startMarker = addTo->size;

    for (size_t i = returnNode->numSockets - 1; i >= 1 + (size_t)isSubroutine;
         i--) {
        SheetSocket *retSocket = returnNode->sockets[i];

        // Has it got a connection?
        if (retSocket->numConnections == 0) {
            // If not, we're going to have to generate the bytecode for
            // the literal.
            BCode literal = d_generate_bytecode_for_literal(retSocket, context,
                                                            false, false);
            d_concat_bytecode(addTo, &literal);
            d_free_bytecode(&literal);
        }

        // One slight problem... the stack is for integers only, so if the
        // socket holds a float, we need to move it to an integer register.
        if (retSocket->type == TYPE_FLOAT && VM_IS_FLOAT_REG(retSocket->_reg)) {
            BCode mv = d_convert_between_number_types(retSocket, context,
                                                      OP_MVTI, false);
            d_concat_bytecode(addTo, &mv);
            d_free_bytecode(&mv);
        }

        reg_t retReg = retSocket->_reg;

        // Now add the argument onto the stack.
        BCode push = d_malloc_bytecode(d_vm_ins_size(OP_PUSH));
        d_bytecode_set_byte(push, 0, OP_PUSH);
        d_bytecode_set_byte(push, 1, (char)retReg);

        d_concat_bytecode(addTo, &push);
        d_free_bytecode(&push);

        // And free the register, now that our value is safe and sound
        // in the stack.
        d_free_reg(context, retReg);
    }

    // Add a return marker.
    d_insert_return_marker(addTo, startMarker);

    if (retAtEnd) {
        BCode codeForRetAtEnd = d_malloc_bytecode(1);
        d_bytecode_set_byte(codeForRetAtEnd, 0, OP_RET);
        d_concat_bytecode(addTo, &codeForRetAtEnd);
        d_free_bytecode(&codeForRetAtEnd);
    }
}
*/

/**
 * \fn BCode d_generate_bytecode_for_literal(SheetSocket *socket,
 *                                           BuildContext *context,
 *                                           bool cvtFloat)
 * \brief Given a socket, generate the bytecode to load the literal value.
 *
 * \return The malloc'd bytecode generated to get the literal value.
 *
 * \param socket The socket to get the literal value from.
 * \param context The context needed to generate the bytecode.
 * \param cvtFloat If true, and if the literal is an integer, convert it into a
 * float.
 */
/*
BCode d_generate_bytecode_for_literal(SheetSocket *socket,
                                      BuildContext *context, bool cvtFloat) {
    VERBOSE(
        5,
        "Generating bytecode to get literal value of type %u from node %s...\n",
        socket->type, socket->node->name);
    BCode out = (BCode){NULL, 0};

    switch (socket->type) {
        case TYPE_INT:;
            dint literal = socket->defaultValue.integerValue;

            if (cvtFloat) {
                literal = *((dint *)((dfloat)literal));

            out = d_malloc_bytecode(d_vm_ins_size(OP_PUSHF));
            d_bytecode_set_byte(out, 0, OP_PUSHF);
            d_bytecode_set_fimmediate(out, 1, (fimmediate_t)literal);

            break;
    }

    // Store the defaultValue in the next available register.
    switch (socket->type) {
        case TYPE_INT:;
            dint literal = socket->defaultValue.integerValue;
            reg_t reg    = d_next_general_reg(context, useSafeReg);

            // Get the socket to remember which register it's using.
            socket->_reg = reg;

            // Do we need to load the integer in 2 parts?
            if ((literal & IMMEDIATE_UPPER_MASK) != 0) {
                // Optimising for if we ORI a value of 0.
                bool ignoreOr = ((literal & IMMEDIATE_MASK) == 0);

                size_t size = d_vm_ins_size(OP_LOADUI);
                if (!ignoreOr)
                    size += d_vm_ins_size(OP_ORI);

                out = d_malloc_bytecode(size);
                d_bytecode_set_byte(out, 0, OP_LOADUI);
                d_bytecode_set_byte(out, 1, (char)reg);
                d_bytecode_set_immediate(
                    out, 2,
                    ((literal & IMMEDIATE_UPPER_MASK) >> (IMMEDIATE_SIZE * 8)));

                if (!ignoreOr) {
                    d_bytecode_set_byte(out, d_vm_ins_size(OP_LOADUI), OP_ORI);
                    d_bytecode_set_byte(
                        out, (size_t)d_vm_ins_size(OP_LOADUI) + 1, (char)reg);
                    d_bytecode_set_immediate(
                        out, (size_t)d_vm_ins_size(OP_LOADUI) + 2,
                        (literal & IMMEDIATE_MASK));
                }
            } else {
                out = d_malloc_bytecode(d_vm_ins_size(OP_LOADI));
                d_bytecode_set_byte(out, 0, OP_LOADI);
                d_bytecode_set_byte(out, 1, (char)reg);
                d_bytecode_set_immediate(out, 2, (immediate_t)literal);
            }

            if (cvtFloat) {
                BCode cvt = d_convert_between_number_types(socket, context,
                                                           OP_CVTF, useSafeReg);
                d_concat_bytecode(&out, &cvt);
                d_free_bytecode(&cvt);
            }
            break;

        case TYPE_FLOAT:;
            dfloat floatLiteral = socket->defaultValue.floatValue;
            dint intRep         = *((dint *)(&floatLiteral));

            reg_t intReg = d_next_general_reg(context, false);

            // Get the socket to remember which register it's using.
            socket->_reg = intReg;

            // Optimising for if we ORI a value of 0.
            bool ignoreOr = ((intRep & IMMEDIATE_MASK) == 0);
            size_t size   = d_vm_ins_size(OP_LOADUI);

            if (!ignoreOr)
                size += d_vm_ins_size(OP_ORI);

            out = d_malloc_bytecode(size);
            d_bytecode_set_byte(out, 0, OP_LOADUI);
            d_bytecode_set_byte(out, 1, (char)intReg);
            d_bytecode_set_immediate(
                out, 2,
                ((intRep & IMMEDIATE_UPPER_MASK) >> (IMMEDIATE_SIZE * 8)));

            if (!ignoreOr) {
                d_bytecode_set_byte(out, d_vm_ins_size(OP_LOADUI), OP_ORI);
                d_bytecode_set_byte(out, (size_t)d_vm_ins_size(OP_LOADUI) + 1,
                                    (char)intReg);
                d_bytecode_set_immediate(out,
                                         (size_t)d_vm_ins_size(OP_LOADUI) + 2,
                                         (intRep & IMMEDIATE_MASK));
            }

            BCode mvtf = d_convert_between_number_types(socket, context,
                                                        OP_MVTF, useSafeReg);

            d_concat_bytecode(&out, &mvtf);
            d_free_bytecode(&mvtf);

            // We don't need the general register anymore.
            d_free_reg(context, intReg);
            break;

        case TYPE_STRING:;
            char *strLiteral = socket->defaultValue.stringValue;

            reg_t strReg = d_next_general_reg(context, useSafeReg);

            // Get the socket to remember which register it's using.
            socket->_reg = strReg;

            out = d_malloc_bytecode((size_t)d_vm_ins_size(OP_LOADUI) +
                                    d_vm_ins_size(OP_ORI));
            d_bytecode_set_byte(out, 0, OP_LOADUI);
            d_bytecode_set_byte(out, 1, (char)strReg);

            d_bytecode_set_byte(out, d_vm_ins_size(OP_LOADUI), OP_ORI);
            d_bytecode_set_byte(out, (size_t)d_vm_ins_size(OP_LOADUI) + 1,
                                (char)strReg);

            // Place the string literal into the data section.
            d_allocate_string_literal_in_data(context, &out, 0, strLiteral);

            break;

        case TYPE_BOOL:;
            dint boolLiteral = (dint)(socket->defaultValue.booleanValue);
            reg_t boolReg    = d_next_general_reg(context, useSafeReg);

            // Get the socket to remember which register it's using.
            socket->_reg = boolReg;

            // TODO: Create a LOADBI instruction?
            out = d_malloc_bytecode(d_vm_ins_size(OP_LOADI));
            d_bytecode_set_byte(out, 0, OP_LOADI);
            d_bytecode_set_byte(out, 1, (char)boolReg);
            d_bytecode_set_immediate(out, 2, (immediate_t)boolLiteral);

            break;

        default:
            break;
    }

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_input(SheetSocket *socket,
 *                                         BuildContext *context, bool inLoop,
 *                                         bool forceLiteral)
 * \brief Given an input socket with a variable data type, generate the
 * bytecode to get the value of the input.
 *
 * \return The malloc'd bytecode generated to get the inputs.
 *
 * \param socket The socket to get the input for.
 * \param context The context needed to generate the bytecode.
 * \param inLoop Are the inputs being gotten from a node that is being run
 * inside a loop?
 * \param forceLiteral Force the bytecode for literals to be generated. You
 * may not want this if you want to optimise later by using immediate
 * instructions.
 */
/*
BCode d_generate_bytecode_for_input(SheetSocket *socket, BuildContext *context,
                                    bool inLoop, bool forceLiteral) {
    VERBOSE(5, "Generating bytecode to get input for socket %p...\n", socket);
    BCode inputCode = (BCode){NULL, 0};

    if (socket->isInput && (socket->type & TYPE_VAR_ANY) == socket->type) {
        SheetNode *node = socket->node;

        // Is this socket connected to a node?
        if (socket->numConnections == 1) {
            SheetSocket *socketConnectedTo = socket->connections[0];
            SheetNode *nodeConnectedTo     = socketConnectedTo->node;

            // If a later input has a call in it, we may as well put
            // this input in a safe register.
            bool useSafeReg     = false;
            bool ourSocketFound = false;

            for (size_t j = 0; j < node->numSockets; j++) {
                SheetSocket *testSocket = node->sockets[j];

                // Only check sockets AFTER the socket we're generating input
                // for.
                if (ourSocketFound) {
                    if (testSocket->isInput &&
                        (testSocket->type & TYPE_VAR_ANY) == testSocket->type) {
                        if (testSocket->numConnections == 1) {
                            SheetSocket *testSocketConn =
                                testSocket->connections[0];
                            SheetNode *testNode = testSocketConn->node;

                            if (d_does_input_involve_call(testNode)) {
                                useSafeReg = true;
                                break;
                            }
                        }
                    }
                } else if (socket == testSocket) {
                    ourSocketFound = true;
                }
            }

            if (!nodeConnectedTo->isExecution) {
                // We just need to make sure we're not about to
                // re-generate bytecode for a Define node for a
                // function. That will have already been done in
                // d_generate_bytecode_for_definition_node().
                if (strcmp(nodeConnectedTo->name, "Define") != 0) {
                    inputCode = d_generate_bytecode_for_nonexecution_node(
                        nodeConnectedTo, context, inLoop);

                    // We need to set our input socket's register to the
                    // connected node's output socket register.
                    socket->_reg = socketConnectedTo->_reg;

                    // But do we need to move it into a safe register?
                    if (useSafeReg) {
                        reg_t safeReg;

                        if (VM_IS_FLOAT_REG(socket->_reg))
                            safeReg = d_next_float_reg(context, useSafeReg);
                        else
                            safeReg = d_next_general_reg(context, useSafeReg);

                        d_free_reg(context, socket->_reg);

                        DIns opcode = (VM_IS_FLOAT_REG(socket->_reg)) ? OP_LOADF
                                                                      : OP_LOAD;

                        BCode move = d_malloc_bytecode(d_vm_ins_size(opcode));
                        d_bytecode_set_byte(move, 0, opcode);
                        d_bytecode_set_byte(move, 1, (char)safeReg);
                        d_bytecode_set_byte(move, 2, (char)socket->_reg);

                        d_concat_bytecode(&inputCode, &move);
                        d_free_bytecode(&move);

                        socket->_reg = safeReg;
                    }
                } else {
                    // If it is a Define node, we need to copy the
                    // inputs into new registers so that the original
                    // data is preserved.
                    // Optimize: If the output socket only has one
                    // connection, and there's no chance of it getting
                    // used more than once (i.e. not in a loop), there's
                    // no need to duplicate.
                    reg_t oldReg = socketConnectedTo->_reg;
                    reg_t newReg;

                    if (socketConnectedTo->numConnections > 1 || inLoop) {
                        if (VM_IS_FLOAT_REG(oldReg))
                            newReg = d_next_float_reg(context, useSafeReg);
                        else
                            newReg = d_next_general_reg(context, useSafeReg);

                        DIns opcode =
                            (VM_IS_FLOAT_REG(oldReg)) ? OP_LOADF : OP_LOAD;

                        inputCode = d_malloc_bytecode(d_vm_ins_size(opcode));
                        d_bytecode_set_byte(inputCode, 0, opcode);
                        d_bytecode_set_byte(inputCode, 1, (char)newReg);
                        d_bytecode_set_byte(inputCode, 2, (char)oldReg);
                    } else {
                        newReg = oldReg;
                    }

                    socket->_reg = newReg;
                }
            } else {
                // For getting output values from execution nodes,
                // we need to duplicate the original value into a
                // new register.
                reg_t oldReg = socketConnectedTo->_reg;
                reg_t newReg;

                if (VM_IS_FLOAT_REG(oldReg))
                    newReg = d_next_float_reg(context, useSafeReg);
                else
                    newReg = d_next_general_reg(context, useSafeReg);

                socket->_reg = newReg;

                DIns opcode = (VM_IS_FLOAT_REG(oldReg)) ? OP_LOADF : OP_LOAD;

                inputCode = d_malloc_bytecode(d_vm_ins_size(opcode));
                d_bytecode_set_byte(inputCode, 0, opcode);
                d_bytecode_set_byte(inputCode, 1, (char)newReg);
                d_bytecode_set_byte(inputCode, 2, (char)oldReg);
            }
        } else if (forceLiteral) {
            // If it is not connected, it is a literal value, so only
            // generate bytecode if the caller wants to.
            inputCode = d_generate_bytecode_for_literal(
                socket, context, socket->type == TYPE_FLOAT, false);
        }
    }

    return inputCode;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_inputs(SheetNode *node,
 *                                          BuildContext *context, bool inLoop,
 *                                          bool forceLiterals)
 * \brief Given a node, generate the bytecode to get the values of the inputs.
 *
 * **NOTE:** This functions just calls `d_generate_bytecode_for_input` for each
 * valid input socket.
 *
 * \return The malloc'd bytecode generated to get the inputs.
 *
 * \param node The node to get the inputs for.
 * \param context The context needed to generate the bytecode.
 * \param inLoop Are the inputs being gotten from a node that is being run
 * inside a loop?
 * \param forceLiterals Force the bytecode for literals to be generated. You
 * may not want this if you want to optimise later by using immediate
 * instructions.
 */
/*
DECISION_API BCode d_generate_bytecode_for_inputs(SheetNode *node,
                                                  BuildContext *context,
                                                  bool inLoop,
                                                  bool forceLiterals) {
    VERBOSE(5, "Generating bytecode to get inputs for node %s...\n",
            node->name);
    BCode out = (BCode){NULL, 0};

    if (node->sockets != NULL && node->numSockets > 0) {
        for (size_t i = 0; i < node->numSockets; i++) {
            BCode inputCode = (BCode){NULL, 0};

            SheetSocket *socket = node->sockets[i];

            // We only care about inputs with the variable data types.
            if (socket->isInput &&
                (socket->type & TYPE_VAR_ANY) == socket->type) {

                inputCode = d_generate_bytecode_for_input(
                    socket, context, inLoop, forceLiterals);
            }

            // After we've generated the code for the individual input,
            // we need to append it to the general bytecode for the node.
            d_concat_bytecode(&out, &inputCode);
            d_free_bytecode(&inputCode);
        }
    }

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_variable(SheetNode *node,
 *                                            BuildContext *context,
 *                                            SheetVariable *variable)
 * \brief Given a node that is a getter for a variable, generate bytecode to
 * get its value.
 *
 * \return The malloc'd bytecode generated the get the value of the variable.
 *
 * \param node The node that is the getter of a variable.
 * \param context The context needed to generate the bytecode.
 * \param variable The variable data needed to generate the bytecode.
 */
/*
BCode d_generate_bytecode_for_variable(SheetNode *node, BuildContext *context,
                                       SheetVariable *variable) {
    VERBOSE(5, "Generating bytecode to get the value of variable %s...\n",
            node->name);
    bool isFloat = variable->dataType == TYPE_FLOAT;
    size_t size  = (variable->dataType == TYPE_BOOL) ? 1 : sizeof(dint);

    // Firstly, we need to generate the instructions to load the address (which
    // will be linked eventually), then to load the value at that address.
    reg_t adrReg = d_next_general_reg(context, false);
    reg_t outReg = (isFloat) ? d_next_float_reg(context, false) : adrReg;

    // Optimising by using the same register to load the address and to carry
    // the output if it's not a float variable.
    if (isFloat)
        d_free_reg(context, adrReg);

    DIns loadOpcode = (size == 1) ? OP_LOADADRB : OP_LOADADR;

    BCode out =
        d_malloc_bytecode((size_t)d_vm_ins_size(OP_LOADUI) +
                          d_vm_ins_size(OP_ORI) + d_vm_ins_size(loadOpcode));
    d_bytecode_set_byte(out, 0, OP_LOADUI);
    d_bytecode_set_byte(out, 1, (char)adrReg);

    d_bytecode_set_byte(out, d_vm_ins_size(OP_LOADUI), OP_ORI);
    d_bytecode_set_byte(out, (size_t)d_vm_ins_size(OP_LOADUI) + 1,
                        (char)adrReg);

    // NOTE: If you want to store more than just 1 or 4 byte variables, you'll
    // need to add more opcodes, or more arguments to existing opcodes.
    d_bytecode_set_byte(
        out, (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI),
        loadOpcode);
    d_bytecode_set_byte(
        out, (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI) + 1,
        (char)adrReg);
    d_bytecode_set_byte(
        out, (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI) + 2,
        (char)adrReg);

    // If it's a float, we need to add an extra instruction to move the value
    // into a float register.
    if (isFloat) {
        // We can't use d_convert_between_number_types here, since we don't
        // know anything about the sockets used.
        BCode mvtf = d_malloc_bytecode(d_vm_ins_size(OP_MVTF));
        d_bytecode_set_byte(mvtf, 0, OP_MVTF);
        d_bytecode_set_byte(mvtf, 1, (char)adrReg);
        d_bytecode_set_byte(mvtf, 2, (char)outReg);

        d_concat_bytecode(&out, &mvtf);
        d_free_bytecode(&mvtf);
    }

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

    // Getters for variable should only have one socket, which is output.
    node->sockets[0]->_reg = outReg;

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_call(SheetNode *node,
 *                                        BuildContext *context,
 *                                        bool isSubroutine, bool isCFunction)
 * \brief Given a node needs to be "called", generate the bytecode and link
 * info to call that function.
 *
 * \return The malloc'd bytecode generated to call the node.
 *
 * \param node The "unknown" function to call.
 * \param context The context needed to generate the bytecode.
 * \param isSubroutine Info needed to make sure we ignore the execution sockets.
 * \param isCFunction Is the call to a C function?
 */
/*
BCode d_generate_bytecode_for_call(SheetNode *node, BuildContext *context,
                                   bool isSubroutine, bool isCFunction) {
    VERBOSE(5, "Generating bytecode to call %s...\n", node->name);
    BCode out    = (BCode){NULL, 0};
    BCode action = (BCode){NULL, 0};

    // Firstly, generate the bytecode to add the arguments onto the stack.
    // Remember we want to add them in backwards so we pop them in the right
    // order.
    BCode argCode = (BCode){NULL, 0};
    for (int i = (int)node->numSockets - 1; i >= (int)isSubroutine; i--) {
        SheetSocket *socket = node->sockets[i];

        if (socket->isInput && (socket->type & TYPE_VAR_ANY) != 0) {
            // We've got an argument.
            // Has it got a connection?
            if (socket->numConnections == 0) {
                // If not, we're going to have to generate the bytecode for
                // the literal.
                action = d_generate_bytecode_for_literal(socket, context, false,
                                                         false);
                d_concat_bytecode(&argCode, &action);
                d_free_bytecode(&action);
            }

            // One slight problem... the stack is for integers only, so if the
            // socket holds a float, we need to move it to an integer register.
            if (socket->type == TYPE_FLOAT && VM_IS_FLOAT_REG(socket->_reg)) {
                BCode mv = d_convert_between_number_types(socket, context,
                                                          OP_MVTI, false);
                d_concat_bytecode(&argCode, &mv);
                d_free_bytecode(&mv);
            }

            reg_t argReg = socket->_reg;

            // Now add the argument onto the stack.
            action = d_malloc_bytecode(d_vm_ins_size(OP_PUSH));
            d_bytecode_set_byte(action, 0, OP_PUSH);
            d_bytecode_set_byte(action, 1, (char)argReg);

            d_concat_bytecode(&argCode, &action);
            d_free_bytecode(&action);

            // And free the register, now that our value is safe and sound
            // in the stack.
            d_free_reg(context, argReg);
        }
    }

    // Secondly, generate the bytecode to save any values we need to save
    // onto the stack.
    // NOTE: With the calling convention, we don't need to save safe registers,
    // but if there are any leftover temporary ones that haven't been free'd,
    // then we need to save those ourselves.
    // NOTE: This code will be executed BEFORE adding the arguments onto the
    // stack.

    // Before we start, we want to make sure that we pop these values back
    // into the same register after we are done with the call.
    // To help do this, we're going to copy the freeReg array now and use it
    // again later.
    char usedRegCopy[VM_NUM_REG / 8];
    memcpy(usedRegCopy, context->usedReg, VM_NUM_REG / 8);

    bool anySaved = false;

    if (!d_is_all_reg_free(context)) {
        anySaved = true;
        for (reg_t i = 0; i < VM_NUM_REG; i++) {
            if (IS_REG_USED(context->usedReg, i) && !IS_REG_SAFE(i)) {
                reg_t valReg = i;

                // Again, if the register holds a float, we need to move it to
                // an integer register.
                if (VM_IS_FLOAT_REG(i)) {
                    valReg = d_next_general_reg(context, false);
                    d_free_reg(context, i);

                    // We can't use d_convert_between_number_types here, since
                    // we don't know anything about the sockets used.
                    action = d_malloc_bytecode(d_vm_ins_size(OP_MVTI));
                    d_bytecode_set_byte(action, 0, OP_MVTI);
                    d_bytecode_set_byte(action, 1, (char)i);
                    d_bytecode_set_byte(action, 2, (char)valReg);

                    d_concat_bytecode(&out, &action);
                    d_free_bytecode(&action);
                }

                // Add the register onto the stack.
                action = d_malloc_bytecode(d_vm_ins_size(OP_PUSH));
                d_bytecode_set_byte(action, 0, OP_PUSH);
                d_bytecode_set_byte(action, 1, (char)valReg);

                d_concat_bytecode(&out, &action);
                d_free_bytecode(&action);

                d_free_reg(context, valReg);
            }
        }
    }

    // NOW add the bytecode for the arguments onto the output.
    d_concat_bytecode(&out, &argCode);
    d_free_bytecode(&argCode);

    // Now generate the call instruction, and setup the link.
    reg_t adrReg = d_next_general_reg(context, false);
    d_free_reg(context, adrReg);

    DIns callOp = (isCFunction) ? OP_CALLC : OP_CALL;

    action = d_malloc_bytecode((size_t)d_vm_ins_size(OP_LOADUI) +
                               d_vm_ins_size(OP_ORI) + d_vm_ins_size(callOp));
    d_bytecode_set_byte(action, 0, OP_LOADUI);
    d_bytecode_set_byte(action, 1, (char)adrReg);

    d_bytecode_set_byte(action, d_vm_ins_size(OP_LOADUI), OP_ORI);
    d_bytecode_set_byte(action, (size_t)d_vm_ins_size(OP_LOADUI) + 1,
                        (char)adrReg);

    d_bytecode_set_byte(
        action, (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI),
        callOp);
    d_bytecode_set_byte(
        action, (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI) + 1,
        (char)adrReg);

    // We don't care if there's a duplicate entry of the metadata. It won't
    // affect how we allocate data in the data section or anything like that.
    size_t _dummyMeta;
    bool _dummyBool;

    LinkType linkType = (isCFunction) ? LINK_CFUNCTION : LINK_FUNCTION;
    void *metaData    = (isCFunction) ? (void *)node->definition.cFunction
                                   : (void *)node->definition.function;

    LinkMeta meta = d_link_new_meta(linkType, node->name, metaData);
    d_add_link_to_ins(context, &action, 0, meta, &_dummyMeta, &_dummyBool);

    d_concat_bytecode(&out, &action);
    d_free_bytecode(&action);

    // Now that the function's been called, we need to get the return values
    // out from the stack.
    // We don't want to put them in registers that will hold saved values,
    // however! So we'll temporarily copy back the freeReg array and set the
    // 'next' values so this doesn't happen.
    if (anySaved) {
        memcpy(context->usedReg, usedRegCopy, VM_NUM_REG / 8);

        context->nextReg = NUM_SAFE_GENERAL_REGISTERS;
        while (IS_REG_USED(context->usedReg, context->nextReg) &&
               context->nextReg < VM_REG_FLOAT_START - 1) {
            context->nextReg++;
        }

        context->nextFloatReg = VM_REG_FLOAT_START + NUM_SAFE_FLOAT_REGISTERS;
        while (IS_REG_USED(context->usedReg, context->nextFloatReg) &&
               context->nextFloatReg < VM_NUM_REG - 1) {
            context->nextFloatReg++;
        }

        context->nextSafeReg = 0;
        while (IS_REG_USED(context->usedReg, context->nextSafeReg) &&
               context->nextSafeReg < VM_REG_FLOAT_START - 1) {
            context->nextSafeReg++;
        }

        context->nextSafeFloatReg = VM_REG_FLOAT_START;
        while (IS_REG_USED(context->usedReg, context->nextSafeFloatReg) &&
               context->nextSafeFloatReg < VM_NUM_REG - 1) {
            context->nextSafeFloatReg++;
        }
    }

    for (size_t i = isSubroutine; i < node->numSockets; i++) {
        SheetSocket *socket = node->sockets[i];

        if (!socket->isInput && (socket->type & TYPE_VAR_ANY) != 0) {
            // We've got a a return value.
            reg_t retReg = d_next_general_reg(context, false);
            socket->_reg = retReg;

            // Now get the return value from the stack.
            action = d_malloc_bytecode(d_vm_ins_size(OP_POP));
            d_bytecode_set_byte(action, 0, OP_POP);
            d_bytecode_set_byte(action, 1, (char)retReg);

            d_concat_bytecode(&out, &action);
            d_free_bytecode(&action);

            // Because the stacks is for integers only, if the socket is a
            // float socket, we need to move the integer value into a float
            // register.
            if (socket->type == TYPE_FLOAT) {
                BCode mv = d_convert_between_number_types(socket, context,
                                                          OP_MVTF, false);
                d_concat_bytecode(&out, &mv);
                d_free_bytecode(&mv);
            }
        }
    }

    // And now we've gotten the return values, any values we saved need to be
    // restored to their correct registers.
    if (anySaved) {
        for (int i = VM_NUM_REG - 1; i >= 0; i--) {
            if (IS_REG_USED(usedRegCopy, i) && !IS_REG_SAFE(i)) {
                // Pop the value out of the stack.
                action = d_malloc_bytecode(d_vm_ins_size(OP_POP));
                d_bytecode_set_byte(action, 0, OP_POP);
                d_bytecode_set_byte(action, 1, (char)i);

                d_concat_bytecode(&out, &action);
                d_free_bytecode(&action);

                SET_REG_USED(context->usedReg, i);

                // If the stack value was originally a float, we need to first
                // pop it into a integer register, then move it to the correct
                // float register.
                if (VM_IS_FLOAT_REG(i)) {
                    reg_t valReg = d_next_float_reg(context, false);
                    d_free_reg(context, (reg_t)i);

                    // We can't use d_convert_between_number_types here, since
                    // we don't know anything about the sockets used.
                    action = d_malloc_bytecode(d_vm_ins_size(OP_MVTF));
                    d_bytecode_set_byte(action, 0, OP_MVTI);
                    d_bytecode_set_byte(action, 1, (char)valReg);
                    d_bytecode_set_byte(action, 2, (char)i);

                    d_concat_bytecode(&out, &action);
                    d_free_bytecode(&action);
                }
            }
        }
    }

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_operator(
 * SheetNode *node, BuildContext *context, DIns opcode, DIns fopcode,
 * DIns iopcode, bool oneInput, bool infiniteInputs, bool forceFloat)
 * \brief Given an operator node, generate the bytecode for it.
 *
 * \return The malloc'd bytecode generated to get the result.
 *
 * \param node The node to get the result from.
 * \param context The context needed to generate the bytecode.
 * \param opcode The operator instruction.
 * \param fopcode The float variant of the instruction.
 * \param iopcode The immediate variant of the instruction.
 * \param oneInput Is there only one input?
 * \param infiniteInputs Does this node take an infinite amount of inputs?
 * \param forceFloat Should the output always be a float?
 */
/*
BCode d_generate_bytecode_for_operator(SheetNode *node, BuildContext *context,
                                       DIns opcode, DIns fopcode, DIns iopcode,
                                       bool oneInput, bool infiniteInputs,
                                       bool forceFloat) {
    VERBOSE(5, "Generating operator bytecode for node %s...\n", node->name);
    BCode out       = (BCode){NULL, 0};
    BCode subaction = (BCode){NULL, 0};
    reg_t reg1, reg2 = 0;
    size_t i = 0;

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

    DIns nonImmediateOpcode = (convertFloat) ? fopcode : opcode;

    // We're going to do stuff onto the first value's register.
    SheetSocket *socket = node->sockets[i];
    d_setup_input(socket, context, convertFloat, false, &out);
    reg1 = socket->_reg;

    if (oneInput) {
        subaction = d_malloc_bytecode(d_vm_ins_size(opcode));
        d_bytecode_set_byte(subaction, 0, opcode);
        d_bytecode_set_byte(subaction, 1, (char)reg1);

        d_concat_bytecode(&out, &subaction);
        d_free_bytecode(&subaction);

        socket = node->sockets[++i];
    } else {
        socket = node->sockets[++i];

        while (socket->isInput) {
            bool do2 = false;

            // Is the socket just an immediate value?
            if (socket->numConnections == 0) {
                if (socket->type == TYPE_BOOL) {
                    bool literalBool = socket->defaultValue.booleanValue;

                    // TODO: LOADBI. Add it.
                    subaction = d_malloc_bytecode(d_vm_ins_size(iopcode));
                    d_bytecode_set_byte(subaction, 0, iopcode);
                    d_bytecode_set_byte(subaction, 1, (char)reg1);
                    d_bytecode_set_immediate(subaction, 2, literalBool);

                    d_concat_bytecode(&out, &subaction);
                    d_free_bytecode(&subaction);
                } else if (socket->type == TYPE_INT && !convertFloat) {
                    dint literalInt = socket->defaultValue.integerValue;

                    // Do we need to load the integer in 2 parts?
                    // If so, we can't use the immediate verison of the
                    // instruction.
                    // Even if the upper half is all 0xff, then we can still
                    // use it with an immediate instruction, since it (should)
                    // get casted to an integer of the immediate's size.
                    if ((literalInt & IMMEDIATE_UPPER_MASK) != 0 &&
                        (literalInt & IMMEDIATE_UPPER_MASK) !=
                            IMMEDIATE_UPPER_MASK) {
                        // Generate the bytecode for loading the full integer,
                        subaction = d_generate_bytecode_for_literal(
                            socket, context, convertFloat, false);
                        d_concat_bytecode(&out, &subaction);
                        d_free_bytecode(&subaction);

                        // then do the normal operation on it.
                        do2 = true;
                    } else {
                        subaction = d_malloc_bytecode(d_vm_ins_size(iopcode));
                        d_bytecode_set_byte(subaction, 0, iopcode);
                        d_bytecode_set_byte(subaction, 1, (char)reg1);
                        d_bytecode_set_immediate(subaction, 2,
                                                 (literalInt & IMMEDIATE_MASK));

                        d_concat_bytecode(&out, &subaction);
                        d_free_bytecode(&subaction);
                    }
                } else {
                    // Generate the bytecode for loading the full float,
                    subaction = d_generate_bytecode_for_literal(
                        socket, context, convertFloat, false);
                    d_concat_bytecode(&out, &subaction);
                    d_free_bytecode(&subaction);

                    // then do the normal operation on it.
                    do2 = true;
                }
            } else {
                do2 = true;
            }

            if (do2) {
                reg2 = socket->_reg;

                // Do we need to convert this register into a float?
                if (!VM_IS_FLOAT_REG(reg2) && convertFloat) {
                    BCode cvt = d_convert_between_number_types(socket, context,
                                                               OP_CVTF, false);
                    d_concat_bytecode(&out, &cvt);
                    d_free_bytecode(&cvt);

                    reg2 = socket->_reg;
                }

                subaction =
                    d_malloc_bytecode(d_vm_ins_size(nonImmediateOpcode));
                d_bytecode_set_byte(subaction, 0, nonImmediateOpcode);
                d_bytecode_set_byte(subaction, 1, (char)reg1);
                d_bytecode_set_byte(subaction, 2, (char)reg2);

                d_concat_bytecode(&out, &subaction);
                d_free_bytecode(&subaction);

                d_free_reg(context, reg2);
            }

            socket = node->sockets[++i];

            // If the operator only has 2 inputs, break out now.
            if (!infiniteInputs)
                break;
        }
    }

    // "socket" should now point to the output socket.
    // We need to set the output socket's register for the next
    // node to use.
    socket->_reg = reg1;

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_comparator(SheetNode *node,
 *                                              BuildContext *context,
 *                                              DIns opcode, DIns fopcode,
 *                                              DIns sopcode, bool notAfter)
 * \brief Given an comparator node, generate the bytecode for it.
 *
 * \return The malloc'd bytecode generated to get the result.
 *
 * \param node The node to get the result from.
 * \param context The context needed to generate the bytecode.
 * \param opcode The operator instruction.
 * \param fopcode The float variant of the instruction.
 * \param sopcode The string variant of the instruction.
 * \param notAfter After the comparison is done, do we invert the answer?
 */
/*
BCode d_generate_bytecode_for_comparator(SheetNode *node, BuildContext *context,
                                         DIns opcode, DIns fopcode,
                                         DIns sopcode, bool notAfter) {
    VERBOSE(5, "Generating comparator bytecode for node %s...\n", node->name);
    BCode out       = (BCode){NULL, 0};
    BCode subaction = (BCode){NULL, 0};
    reg_t reg1, reg2, reg3 = 0;

    // Do we have string inputs?
    bool stringInput = (node->sockets[0]->type == TYPE_STRING &&
                        node->sockets[1]->type == TYPE_STRING);

    // Do we need to convert integer inputs to floats?
    // Check what our inputs are. If one is a float, we need to.
    bool convertFloat = false;

    if (!stringInput)
        for (size_t i = 0; i < 2; i++) {
            SheetSocket *inputSocket = node->sockets[i];

            if (inputSocket->type == TYPE_FLOAT) {
                convertFloat = true;
                break;
            }
        }

    DIns opcodeToUse = (convertFloat) ? fopcode : opcode;
    opcodeToUse      = (stringInput) ? sopcode : opcodeToUse;

    // We're going to do stuff onto the first value's register.
    SheetSocket *socket = node->sockets[0];
    d_setup_input(socket, context, convertFloat, false, &out);
    reg1 = socket->_reg;

    socket = node->sockets[1];
    d_setup_input(socket, context, convertFloat, false, &out);
    reg2 = socket->_reg;

    // If we're dealing with floats, we need a GENERAL register to store the
    // result.
    if (convertFloat) {
        reg3 = d_next_general_reg(context, false);
    } else {
        reg3 = reg1;
    }

    subaction = d_malloc_bytecode(d_vm_ins_size(opcodeToUse));
    d_bytecode_set_byte(subaction, 0, opcodeToUse);
    d_bytecode_set_byte(subaction, 1, (char)reg3);
    d_bytecode_set_byte(subaction, 2, (char)reg1);
    d_bytecode_set_byte(subaction, 3, (char)reg2);

    if (notAfter) {
        BCode not = d_malloc_bytecode(d_vm_ins_size(OP_NOT));
        d_bytecode_set_byte(not, 0, OP_NOT);
        d_bytecode_set_byte(not, 1, (char)reg3);

        d_concat_bytecode(&subaction, &not);
        d_free_bytecode(&not);
    }

    d_concat_bytecode(&out, &subaction);

    d_free_reg(context, reg2);
    if (convertFloat)
        d_free_reg(context, reg1);

    // Setup the outptut socket so the next node knows which register to use.
    socket       = node->sockets[2];
    socket->_reg = reg3;

    d_free_bytecode(&subaction);

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_nonexecution_node(
 * SheetNode *node, BuildContext *context, bool inLoop)
 * \brief Given a node, generate the bytecode to get the correct output.
 *
 * \return The malloc'd bytecode it generated.
 *
 * \param node The non-execution node to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 * \param inLoop Is this node being run from inside a loop?
 */
/*
BCode d_generate_bytecode_for_nonexecution_node(SheetNode *node,
                                                BuildContext *context,
                                                bool inLoop) {
    VERBOSE(5, "- Generating bytecode for non-execution node %s...\n",
            node->name);

    BCode out = (BCode){NULL, 0};

    // Firstly, we need to check if the node is a particular function -
    // spoiler alert, one of them is not like the others...
    const CoreFunction coreFunc = d_core_find_name(node->name);

    if (coreFunc == CORE_TERNARY) {
        // Hi. This is the story of why this if statement exists.
        // I was a dumbass. I just thought, "oh, I should probably add a
        // Ternary node, and it doesn't seem too bad, it's just one JRCON,
        // right?". WRONG. SO WRONG. Turns out for LITERALLY ANY OTHER
        // non-execution node you can just call d_generate_bytecode_for_inputs,
        // but NOOO here you need to get seperate bytecode for each of the
        // inputs since you don't want to run the bytecode for ALL inputs only
        // to just use one, do you??? Nooo, no one wants that...
        // P.S. This is why d_generate_bytecode_for_input exists.
        //     - drwhut

        // Firstly, generate the bytecode for the boolean input.
        SheetSocket *boolSocket = node->sockets[0];

        // If the boolean input is a literal, we only need to generate bytecode
        // for the input that is active, since that is all that is ever gonna
        // be picked!
        bool boolIsLiteral    = (boolSocket->numConnections == 0);
        bool boolLiteralValue = boolSocket->defaultValue.booleanValue;

        BCode boolCode =
            d_generate_bytecode_for_input(boolSocket, context, inLoop, false);

        d_concat_bytecode(&out, &boolCode);
        d_free_bytecode(&boolCode);

        reg_t regBool = boolSocket->_reg;

        // Once we have used the bool value to determine which code we run,
        // we don't need it anymore.
        if (!boolIsLiteral) {
            d_free_reg(context, regBool);
        }

        // Next, get the bytecode for the true input.
        SheetSocket *trueSocket = node->sockets[1];
        BCode trueCode          = (BCode){NULL, 0};

        if (!boolIsLiteral || boolLiteralValue) {
            trueCode = d_generate_bytecode_for_input(trueSocket, context,
                                                     inLoop, true);

            d_free_reg(context, trueSocket->_reg);
        }

        // Finally, get the bytecode for the false input.
        SheetSocket *falseSocket = node->sockets[2];
        BCode falseCode          = (BCode){NULL, 0};

        if (!boolIsLiteral || !boolLiteralValue) {
            falseCode = d_generate_bytecode_for_input(falseSocket, context,
                                                      inLoop, true);

            d_free_reg(context, falseSocket->_reg);
        }

        SheetSocket *outputSocket = node->sockets[3];

        // If the boolean input is a literal, then it's easy! We just output
        // the bytecode that gives us the output corresponding to if the
        // literal is true or false.
        if (boolIsLiteral) {
            BCode *append = (boolLiteralValue) ? &trueCode : &falseCode;
            d_concat_bytecode(&out, append);
            d_free_bytecode(append);

            // Setting the output socket register.
            SheetSocket *inputSocket =
                (boolLiteralValue) ? trueSocket : falseSocket;
            outputSocket->_reg = inputSocket->_reg;

        } else {

            // Wise guy, eh? So we need to include the bytecode for both the
            // true AND the false inputs, and only run the correct one
            // depending on the value of the boolean input.

            // Have the output register be the true input register. If the
            // false input is the used input, then we need to make sure the
            // false code outputs to the same register.
            outputSocket->_reg = trueSocket->_reg;

            if (falseSocket->_reg != outputSocket->_reg) {
                DIns moveOp =
                    (outputSocket->type == TYPE_FLOAT) ? OP_LOADF : OP_LOAD;
                BCode moveReg = d_malloc_bytecode(d_vm_ins_size(moveOp));

                d_bytecode_set_byte(moveReg, 0, moveOp);
                d_bytecode_set_byte(moveReg, 1, (char)outputSocket->_reg);
                d_bytecode_set_byte(moveReg, 2, (char)falseSocket->_reg);

                d_concat_bytecode(&falseCode, &moveReg);
                d_free_bytecode(&moveReg);
            }

            // At the end of the false code, we need to add a JR to jump over
            // the true code.
            BCode jumpOverTrue = d_malloc_bytecode(d_vm_ins_size(OP_JR));
            d_bytecode_set_byte(jumpOverTrue, 0, OP_JR);

            dint jmpAmt = d_vm_ins_size(OP_JR) + trueCode.size;
            d_bytecode_set_immediate(jumpOverTrue, 1,
                                     (jmpAmt & IMMEDIATE_MASK));

            d_concat_bytecode(&falseCode, &jumpOverTrue);
            d_free_bytecode(&jumpOverTrue);

            // After we have evaluated the boolean input, we need to add a
            // JRCON instruction to jump to the true code if the boolean is
            // true, and run the false code otherwise.
            BCode jumpToCode = d_malloc_bytecode(d_vm_ins_size(OP_JRCON));
            d_bytecode_set_byte(jumpToCode, 0, OP_JRCON);
            d_bytecode_set_byte(jumpToCode, 1, (char)regBool);

            jmpAmt = d_vm_ins_size(OP_JRCON) + falseCode.size;
            d_bytecode_set_immediate(jumpToCode, 2, (jmpAmt & IMMEDIATE_MASK));

            d_concat_bytecode(&out, &jumpToCode);
            d_free_bytecode(&jumpToCode);

            // Add the false code, and the true code afterwards as well.
            d_concat_bytecode(&out, &falseCode);
            d_free_bytecode(&falseCode);

            d_concat_bytecode(&out, &trueCode);
            d_free_bytecode(&trueCode);
        }

        // Since the output register makes use of one of the input registers,
        // both of which were free'd (since the bytecodes were independent),
        // we should set the register to be used. We don't need to worry about
        // the max used array, since the register WAS used.
        SET_REG_USED(context->usedReg, outputSocket->_reg);

    } else {
        // Firstly, we need to generate the bytecode for our inputs.
        out = d_generate_bytecode_for_inputs(node, context, inLoop, false);

        // Secondly, we do stuff with those inputs.
        BCode action = (BCode){NULL, 0};

        if ((int)coreFunc >= 0) {
            // Generate bytecode depending on the core function.
            // Remember that it's only non-execution functions we care about.
            switch (coreFunc) {
                case CORE_ADD:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_ADD, OP_ADDF, OP_ADDI, false, true,
                        false);
                    break;
                case CORE_AND:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_AND, 0, OP_ANDI, false, false, false);
                    break;
                case CORE_DIV:
                case CORE_DIVIDE:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_DIV, OP_DIVF, OP_DIVI, false, false,
                        coreFunc == CORE_DIVIDE);

                    SheetSocket *divOutput = node->sockets[2];
                    if (coreFunc == CORE_DIV &&
                        VM_IS_FLOAT_REG(divOutput->_reg)) {
                        // If the answer is a float, we need to turn it back
                        // into an integer.
                        // TODO: Does this need to go into a safe register?
                        BCode cvti = d_convert_between_number_types(
                            divOutput, context, OP_CVTI, false);

                        d_concat_bytecode(&action, &cvti);
                        d_free_bytecode(&cvti);
                    }
                    break;
                case CORE_EQUAL:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CEQ, OP_CEQF, OP_CEQS, false);
                    break;
                case CORE_MULTIPLY:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_MUL, OP_MULF, OP_MULI, false, true,
                        false);
                    break;
                case CORE_LENGTH:;
                    // For the length of a string, we're essentially going to
                    // do in Decision bytecode what strlen() does, which is
                    // loop through the string until we find the terminating
                    // NULL, and return how many characters there were.

                    // Firstly, copy the string pointer. This is the pointer
                    // we'll be using to check the characters.
                    reg_t ptrReg = d_next_general_reg(context, false);

                    SheetSocket *inputSock = node->sockets[0];

                    d_setup_input(inputSock, context, false, false, &action);

                    BCode loadPtr = d_malloc_bytecode(d_vm_ins_size(OP_LOAD));
                    d_bytecode_set_byte(loadPtr, 0, OP_LOAD);
                    d_bytecode_set_byte(loadPtr, 1, (char)ptrReg);
                    d_bytecode_set_byte(loadPtr, 2, (char)inputSock->_reg);

                    d_concat_bytecode(&action, &loadPtr);
                    d_free_bytecode(&loadPtr);

                    // Secondly, we make the loop bytecode. Here we get the
                    // character the pointer points to, check if it is 0, if
                    // it is, we exit the loop.
                    reg_t charReg = d_next_general_reg(context, false);

                    BCode loop = d_malloc_bytecode(d_vm_ins_size(OP_LOADADRB));
                    d_bytecode_set_byte(loop, 0, OP_LOADADRB);
                    d_bytecode_set_byte(loop, 1, (char)charReg);
                    d_bytecode_set_byte(loop, 2, (char)ptrReg);

                    reg_t cmpReg = d_next_general_reg(context, false);
                    d_free_reg(context, cmpReg);
                    d_free_reg(context, charReg);

                    BCode loopAdd = d_malloc_bytecode(d_vm_ins_size(OP_LOADI));
                    d_bytecode_set_byte(loopAdd, 0, OP_LOADI);
                    d_bytecode_set_byte(loopAdd, 1, (char)cmpReg);
                    d_bytecode_set_immediate(loopAdd, 2, 0);

                    d_concat_bytecode(&loop, &loopAdd);
                    d_free_bytecode(&loopAdd);

                    loopAdd = d_malloc_bytecode(d_vm_ins_size(OP_CEQ));
                    d_bytecode_set_byte(loopAdd, 0, OP_CEQ);
                    d_bytecode_set_byte(loopAdd, 1, (char)cmpReg);
                    d_bytecode_set_byte(loopAdd, 2, (char)charReg);
                    d_bytecode_set_byte(loopAdd, 3, (char)cmpReg);

                    d_concat_bytecode(&loop, &loopAdd);
                    d_free_bytecode(&loopAdd);

                    immediate_t jmpToAfterLoop = d_vm_ins_size(OP_JRCON) +
                                                 d_vm_ins_size(OP_ADDI) +
                                                 d_vm_ins_size(OP_JR);

                    loopAdd = d_malloc_bytecode(d_vm_ins_size(OP_JRCON));
                    d_bytecode_set_byte(loopAdd, 0, OP_JRCON);
                    d_bytecode_set_byte(loopAdd, 1, (char)cmpReg);
                    d_bytecode_set_immediate(loopAdd, 2, jmpToAfterLoop);

                    d_concat_bytecode(&loop, &loopAdd);
                    d_free_bytecode(&loopAdd);

                    loopAdd = d_malloc_bytecode(d_vm_ins_size(OP_ADDI));
                    d_bytecode_set_byte(loopAdd, 0, OP_ADDI);
                    d_bytecode_set_byte(loopAdd, 1, (char)ptrReg);
                    d_bytecode_set_immediate(loopAdd, 2, 1);

                    d_concat_bytecode(&loop, &loopAdd);
                    d_free_bytecode(&loopAdd);

                    immediate_t jmpToStart = -((immediate_t)loop.size);

                    loopAdd = d_malloc_bytecode(d_vm_ins_size(OP_JR));
                    d_bytecode_set_byte(loopAdd, 0, OP_JR);
                    d_bytecode_set_immediate(loopAdd, 1, jmpToStart);

                    d_concat_bytecode(&loop, &loopAdd);
                    d_free_bytecode(&loopAdd);

                    d_concat_bytecode(&action, &loop);
                    d_free_bytecode(&loop);

                    // Now the pointer is pointing to the terminating NULL
                    // at the end of the string. So if we want the length,
                    // we can just subtract the pointer from the string
                    // pointer!
                    BCode subPtr = d_malloc_bytecode(d_vm_ins_size(OP_SUB));
                    d_bytecode_set_byte(subPtr, 0, OP_SUB);
                    d_bytecode_set_byte(subPtr, 1, (char)ptrReg);
                    d_bytecode_set_byte(subPtr, 2, (char)inputSock->_reg);

                    d_concat_bytecode(&action, &subPtr);
                    d_free_bytecode(&subPtr);

                    SheetSocket *outputSock = node->sockets[1];
                    outputSock->_reg        = ptrReg;

                    d_free_reg(context, inputSock->_reg);

                    break;
                case CORE_LESS_THAN:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CLT, OP_CLTF, OP_CLTS, false);
                    break;
                case CORE_LESS_THAN_OR_EQUAL:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CLEQ, OP_CLEQF, OP_CLEQS, false);
                    break;
                case CORE_MOD:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_MOD, 0, OP_MODI, false, false, false);
                    break;
                case CORE_MORE_THAN:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CMT, OP_CMTF, OP_CMTS, false);
                    break;
                case CORE_MORE_THAN_OR_EQUAL:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CMEQ, OP_CMEQF, OP_CMEQS, false);
                    break;
                case CORE_NOT:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_NOT, 0, 0, true, false, false);
                    break;
                case CORE_NOT_EQUAL:;
                    action = d_generate_bytecode_for_comparator(
                        node, context, OP_CEQ, OP_CEQF, OP_CEQS, true);
                    break;
                case CORE_OR:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_OR, 0, OP_ORI, false, false, false);
                    break;
                case CORE_SUBTRACT:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_SUB, OP_SUBF, OP_SUBI, false, false,
                        false);
                    break;
                case CORE_TERNARY:;
                    // Ternary is a special snowflake when it comes to
                    // non-execution nodes...
                    // See above.
                    break;
                case CORE_XOR:;
                    action = d_generate_bytecode_for_operator(
                        node, context, OP_XOR, 0, OP_XORI, false, false, false);
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
                action = d_generate_bytecode_for_variable(
                    node, context, node->definition.variable);
            } else if (node->definition.type == NAME_FUNCTION) {
                action =
                    d_generate_bytecode_for_call(node, context, false, false);
            } else if (node->definition.type == NAME_CFUNCTION) {
                action =
                    d_generate_bytecode_for_call(node, context, false, true);
            }
        }

        d_concat_bytecode(&out, &action);
        d_free_bytecode(&action);
    }

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_execution_node(
 * SheetNode *node, BuildContext *context, bool retAtEnd, bool inLoop)
 * \brief Given a node, generate the bytecode to carry out it's instruction.
 *
 * \return The malloc'd bytecode it generated.
 *
 * \param node The execution node to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 * \param retAtEnd If true, and if the node is at the end of the chain of
 * execution, put a OP_RET instruction at the end of the bytecode for this node.
 * \param inLoop Should be false if this is a top-level node, true if it is
 * being run inside a loop. This prevents a RET being put at the end of the
 * sequence of instructions.
 */
/*
BCode d_generate_bytecode_for_execution_node(SheetNode *node,
                                             BuildContext *context,
                                             bool retAtEnd, bool inLoop) {
    VERBOSE(5, "- Generating bytecode for execution node %s...\n", node->name);

    // Firstly, we need to get the bytecode generated to get the inputs.
    BCode out = d_generate_bytecode_for_inputs(node, context, inLoop, false);

    // Secondly, we use the inputs we just got to perform an action.
    BCode action                = (BCode){NULL, 0};
    const CoreFunction coreFunc = d_core_find_name(node->name);

    if ((int)coreFunc >= 0) {
        SheetSocket *socket;
        SheetNode *firstNode;
        BCode ready = (BCode){NULL, 0};

        // Generate bytecode depending on the core function.
        // Remember that it's only execution functions we care about.
        switch (coreFunc) {
            case CORE_FOR:;
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

                break;

            case CORE_IF_THEN:
            case CORE_IF_THEN_ELSE:;
                // Get the socket with the condition input.
                socket = node->sockets[1];
                d_setup_input(socket, context, false, false, &action);

                // Get it's register.
                reg_t condReg = socket->_reg;
                d_free_reg(context, condReg);

                BCode thenBranch = (BCode){NULL, 0};
                BCode elseBranch = (BCode){NULL, 0};

                // Get the individual branches compiled.
                socket = node->sockets[2];
                if (socket->numConnections == 1) {
                    firstNode  = socket->connections[0]->node;
                    thenBranch = d_generate_bytecode_for_execution_node(
                        firstNode, context, false, inLoop);
                }

                if (coreFunc == CORE_IF_THEN_ELSE) {
                    socket = node->sockets[3];
                    if (socket->numConnections == 1) {
                        firstNode  = socket->connections[0]->node;
                        elseBranch = d_generate_bytecode_for_execution_node(
                            firstNode, context, false, inLoop);
                    }
                }

                // We put a JRCON at the start to jump to the THEN branch if
                // the condition is true, which comes after the ELSE branch.
                // If the JRCON fails, the next bit is the ELSE section, which
                // is followed by a JR to go past the THEN branch.
                // [JRCON][ELSE][JR][THEN][...]
                //    \-----------\-/     /
                //                 \-----/

                immediate_t jmpToThen =
                    d_vm_ins_size(OP_JRCON) + (immediate_t)elseBranch.size;
                immediate_t jmpToEnd =
                    d_vm_ins_size(OP_JR) + (immediate_t)thenBranch.size;

                // There's a possibility that the then branch doesn't have any
                // code, which means the jump instruction that goes over the
                // then branch doesn't need to exist.
                bool optimizeJmpToEnd = thenBranch.size == 0;

                if (!optimizeJmpToEnd) {
                    jmpToThen += d_vm_ins_size(OP_JR);
                }

                BCode conAtStart = d_malloc_bytecode(d_vm_ins_size(OP_JRCON));
                d_bytecode_set_byte(conAtStart, 0, OP_JRCON);
                d_bytecode_set_byte(conAtStart, 1, (char)condReg);
                d_bytecode_set_immediate(conAtStart, 2, jmpToThen);

                BCode conAtEndElse = (BCode){NULL, 0};

                if (!optimizeJmpToEnd) {
                    conAtEndElse = d_malloc_bytecode(d_vm_ins_size(OP_JR));
                    d_bytecode_set_byte(conAtEndElse, 0, OP_JR);
                    d_bytecode_set_immediate(conAtEndElse, 1, jmpToEnd);
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
                // Get the socket with the variable input.
                socket = node->sockets[1];
                d_setup_input(socket, context, false, false, &action);
                reg_t regToPrint = socket->_reg;

                // Set the arguments for the syscall.
                immediate_t typeArg = 0;

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

                // TODO: Load all arguments in one instruction.
                ready =
                    d_malloc_bytecode(3 * (size_t)d_vm_ins_size(OP_LOADARGI) +
                                      d_vm_ins_size(OP_SYSCALL));

                // arg0
                d_bytecode_set_byte(ready, 0, OP_LOADARGI);
                d_bytecode_set_byte(ready, 1, 0);
                d_bytecode_set_immediate(ready, 2, typeArg);

                // arg1
                d_bytecode_set_byte(ready, d_vm_ins_size(OP_LOADARGI),
                                    OP_LOADARGI);
                d_bytecode_set_byte(ready,
                                    (size_t)d_vm_ins_size(OP_LOADARGI) + 1, 1);
                d_bytecode_set_immediate(
                    ready, (size_t)d_vm_ins_size(OP_LOADARGI) + 2, regToPrint);

                // arg2
                d_bytecode_set_byte(
                    ready, (size_t)d_vm_ins_size(OP_LOADARGI) * 2, OP_LOADARGI);
                d_bytecode_set_byte(
                    ready, (size_t)d_vm_ins_size(OP_LOADARGI) * 2 + 1, 2);
                d_bytecode_set_immediate(
                    ready, (size_t)d_vm_ins_size(OP_LOADARGI) * 2 + 2, 1);

                d_bytecode_set_byte(
                    ready, (size_t)d_vm_ins_size(OP_LOADARGI) * 3, OP_SYSCALL);
                d_bytecode_set_byte(ready,
                                    (size_t)d_vm_ins_size(OP_LOADARGI) * 3 + 1,
                                    SYS_PRINT);

                d_concat_bytecode(&action, &ready);
                d_free_bytecode(&ready);

                d_free_reg(context, regToPrint);
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
                DIns storeAdr     = (varSize == 1) ? OP_STOADRB : OP_STOADR;
                LinkType linkType = (var->dataType == TYPE_STRING)
                                        ? LINK_VARIABLE_POINTER
                                        : LINK_VARIABLE;

                // We also need the socket containing the value we want to set.
                socket = node->sockets[2];
                d_setup_input(socket, context, false, false, &out);

                // We'll need a register to store the address of the variable
                // in.
                reg_t adrReg = d_next_general_reg(context, false);

                // And get the value's register.
                reg_t valReg = socket->_reg;

                // If we need to set a malloc'd bit of memory, use this to
                // store the address of the bit of memory, i.e. the start of
                // a variable string.
                reg_t dataAdrReg = valReg;

                // Firstly, generate the starting bytecode to store the value
                // at the sonn-to-be linked address.
                // TODO: Can this be optimized for strings?
                BCode subaction = d_malloc_bytecode(
                    (size_t)d_vm_ins_size(OP_LOADUI) + d_vm_ins_size(OP_ORI));
                d_bytecode_set_byte(subaction, 0, OP_LOADUI);
                d_bytecode_set_byte(subaction, 1, (char)adrReg);

                d_bytecode_set_byte(subaction, d_vm_ins_size(OP_LOADUI),
                                    OP_ORI);
                d_bytecode_set_byte(subaction,
                                    (size_t)d_vm_ins_size(OP_LOADUI) + 1,
                                    (char)adrReg);

                d_concat_bytecode(&action, &subaction);
                d_free_bytecode(&subaction);

                // If we're setting a string, we need to use a special opcode
                // to reallocate the string variable and copy over the content
                // of the string.
                if (var->dataType == TYPE_STRING) {
                    // We need to save the original address to the variable to
                    // write back to it afterwards, in case the memory changes
                    // location.
                    dataAdrReg = d_next_general_reg(context, false);
                    d_free_reg(context, dataAdrReg);

                    subaction = d_malloc_bytecode(
                        (size_t)d_vm_ins_size(OP_LOAD) +
                        d_vm_ins_size(OP_LOADADR) +
                        2 * (size_t)d_vm_ins_size(OP_LOADARGI) +
                        d_vm_ins_size(OP_SYSCALL));

                    d_bytecode_set_byte(subaction, 0, OP_LOAD);
                    d_bytecode_set_byte(subaction, 1, (char)dataAdrReg);
                    d_bytecode_set_byte(subaction, 2, (char)adrReg);

                    d_bytecode_set_byte(subaction, d_vm_ins_size(OP_LOAD),
                                        OP_LOADADR);
                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) + 1,
                                        (char)dataAdrReg);
                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) + 2,
                                        (char)dataAdrReg);

                    // Make the LOADSTR system call.
                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) +
                                            d_vm_ins_size(OP_LOADADR),
                                        OP_LOADARGI);
                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) +
                                            d_vm_ins_size(OP_LOADADR) + 1,
                                        0);
                    d_bytecode_set_immediate(subaction,
                                             (size_t)d_vm_ins_size(OP_LOAD) +
                                                 d_vm_ins_size(OP_LOADADR) + 2,
                                             (char)dataAdrReg);

                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) +
                                            d_vm_ins_size(OP_LOADADR) +
                                            d_vm_ins_size(OP_LOADARGI),
                                        OP_LOADARGI);
                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) +
                                            d_vm_ins_size(OP_LOADADR) +
                                            d_vm_ins_size(OP_LOADARGI) + 1,
                                        1);
                    d_bytecode_set_immediate(subaction,
                                             (size_t)d_vm_ins_size(OP_LOAD) +
                                                 d_vm_ins_size(OP_LOADADR) +
                                                 d_vm_ins_size(OP_LOADARGI) + 2,
                                             (char)valReg);

                    d_bytecode_set_byte(subaction,
                                        (size_t)d_vm_ins_size(OP_LOAD) +
                                            d_vm_ins_size(OP_LOADADR) +
                                            (size_t)d_vm_ins_size(OP_LOADARGI) *
                                                2,
                                        OP_SYSCALL);
                    d_bytecode_set_byte(
                        subaction,
                        (size_t)d_vm_ins_size(OP_LOAD) +
                            d_vm_ins_size(OP_LOADADR) +
                            (size_t)d_vm_ins_size(OP_LOADARGI) * 2 + 1,
                        SYS_LOADSTR);

                    d_concat_bytecode(&action, &subaction);
                    d_free_bytecode(&subaction);
                }

                subaction = d_malloc_bytecode(d_vm_ins_size(storeAdr));
                d_bytecode_set_byte(subaction, 0, (char)storeAdr);
                d_bytecode_set_byte(subaction, 1, (char)dataAdrReg);
                d_bytecode_set_byte(subaction, 2, (char)adrReg);

                d_concat_bytecode(&action, &subaction);
                d_free_bytecode(&subaction);

                // Now we've generated the bytecode, we don't need the value
                // register or address anymore.
                d_free_reg(context, adrReg);
                d_free_reg(context, valReg);

                // Secondly, setup the link to the variable.
                LinkMeta varMeta = d_link_new_meta(linkType, var->name, var);

                size_t index;
                bool wasDuplicate;
                d_add_link_to_ins(context, &action, 0, varMeta, &index,
                                  &wasDuplicate);

                break;

            case CORE_WHILE:;
                // We need the register for the boolean condition.
                socket = node->sockets[1];
                d_setup_input(socket, context, false, false, &out);
                reg_t condWhileReg = socket->_reg;

                // Now we'll generate the bytecode for the code that executes
                // if the condition is true.
                socket         = node->sockets[2];
                BCode trueCode = (BCode){NULL, 0};

                if (socket->numConnections == 1) {
                    firstNode = socket->connections[0]->node;
                    trueCode  = d_generate_bytecode_for_execution_node(
                        firstNode, context, false, true);
                }

                // After the loop code has executed, we want to re-calculate
                // our boolean input, i.e. jump back as far as we need to.
                dint loopBackAmt =
                    -((dint)out.size + (dint)trueCode.size +
                      d_vm_ins_size(OP_NOT) + d_vm_ins_size(OP_JRCON));

                action = d_malloc_bytecode(d_vm_ins_size(OP_JR));
                d_bytecode_set_byte(action, 0, OP_JR);
                d_bytecode_set_immediate(action, 1,
                                         (loopBackAmt & IMMEDIATE_MASK));

                d_concat_bytecode(&trueCode, &action);
                d_free_bytecode(&action);

                // When we get our input, we want to skip the loop code if it
                // is false.
                action = d_malloc_bytecode((size_t)d_vm_ins_size(OP_NOT) +
                                           d_vm_ins_size(OP_JRCON));

                d_bytecode_set_byte(action, 0, OP_NOT);
                d_bytecode_set_byte(action, 1, (char)condWhileReg);

                d_bytecode_set_byte(action, d_vm_ins_size(OP_NOT), OP_JRCON);
                d_bytecode_set_byte(action, (size_t)d_vm_ins_size(OP_NOT) + 1,
                                    (char)condWhileReg);
                d_bytecode_set_immediate(
                    action, (size_t)d_vm_ins_size(OP_NOT) + 2,
                    (immediate_t)trueCode.size + d_vm_ins_size(OP_JRCON));

                d_concat_bytecode(&action, &trueCode);
                d_free_bytecode(&trueCode);
                break;

            default:
                break;
        }
    }
    // TODO: Add define here for safety.
    else if (strcmp(node->name, "Return") == 0) {
        // Problem: We're in a subroutine, and we need to load back in safe
        // registers now... but we cannot be certain at this point that we've
        // used all of the safe registers we're gonna use. For now, don't put
        // anything, but once we're done with the function, we'll add in the
        // instructions to load back safe registers.

        d_setup_returns(node, context, &action, true, true);
    } else {
        // Put arguments into the stack and call the subroutine.
        action = d_generate_bytecode_for_call(
            node, context, true, node->definition.type == NAME_CFUNCTION);
    }

    d_concat_bytecode(&out, &action);
    d_free_bytecode(&action);

    // Thirdly, we generate the bytecode for the next execution node.
    BCode nextCode = (BCode){NULL, 0};

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

            // Recursively call d_generate_bytecode_for_execution_node
            // to generate the bytecode after this node's bytecode.
            nextCode = d_generate_bytecode_for_execution_node(
                nodeConnectedTo, context, true, inLoop);
        }
    }

    if (!isNextNode && retAtEnd && !inLoop) {
        // Add a RET instruction here, so the VM returns.
        nextCode = d_malloc_bytecode(1);
        d_bytecode_set_byte(nextCode, 0, OP_RET);
    }

    d_concat_bytecode(&out, &nextCode);
    d_free_bytecode(&nextCode);

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_start(SheetNode *startNode,
 *                                         BuildContext *context)
 * \brief Given a Start node, generate the bytecode for the sequence starting
 * from this node.
 *
 * \return The bytecode generated for the Start function.
 *
 * \param startNode A pointer to the Start node.
 * \param context The context needed to generate the bytecode.
 */
/*
BCode d_generate_bytecode_for_start(SheetNode *startNode,
                                    BuildContext *context) {
    // New function means all registers are free before we generate bytecode.
    d_reg_new_function(context);

    // As the first instruction, place a RET, which acts as a safety barrier
    // for any function that is defined before this one (this ensures the
    // previous function doesn't "leak" into this one).
    BCode out = d_malloc_bytecode(1);
    d_bytecode_set_byte(out, 0, OP_RET);

    // In terms of generating the bytecode from the Start node, this is easy.
    // We just need to call the bytecode generation functions starting from the
    // first node after Start.

    if (startNode->numSockets == 1) {
        SheetSocket *socket = startNode->sockets[0];

        if (socket->numConnections == 1) {
            VERBOSE(5, "-- Generating bytecode for Start function...\n");

            socket = socket->connections[0];

            BCode exe = d_generate_bytecode_for_execution_node(
                socket->node, context, true, false);

            d_concat_bytecode(&out, &exe);
            d_free_bytecode(&exe);
        }
    }

    return out;
}
*/

/**
 * \fn BCode d_generate_bytecode_for_function(SheetFunction *func,
 *                                            BuildContext *context)
 * \brief Given a function, generate the bytecode for it.
 *
 * \return The bytecode generated for the function.
 *
 * \param func The function to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 */
/*
BCode d_generate_bytecode_for_function(SheetFunction *func,
                                       BuildContext *context) {
    // New function means all registers are free before we generate bytecode.
    d_reg_new_function(context);

    // As the first instruction, place a RET, which allows linkers to link to
    // this instruction, since the program counter of the virtual machine will
    // ALWAYS increment, it will increment to the actual first instruction of
    // the function.
    BCode out = d_malloc_bytecode(1);
    d_bytecode_set_byte(out, 0, OP_RET);

    if (func->isSubroutine) {
        if (func->defineNode != NULL) {
            VERBOSE(5, "-- Generating bytecode for subroutine %s...\n",
                    func->name);

            // If it's a subroutine, we can just recursively go through the
            // sequence to the Return.
            // NOTE: The first argument should be the name of the function.
            SheetSocket *socket = func->defineNode->sockets[1];

            // Pop arguments from the stack.
            d_setup_arguments(func->defineNode, context, &out, true);

            if (socket->numConnections == 1 && socket->type == TYPE_EXECUTION) {
                socket = socket->connections[0];

                BCode exe = d_generate_bytecode_for_execution_node(
                    socket->node, context, true, false);

                // Now we've compiled the function, we know what safe
                // registers it uses. Save these before the function
                // executes.
                d_save_safe_reg(context, &out);

                // Let's keep bytecode to load them back in handy as well
                // for later...
                BCode loadSafeReg = (BCode){NULL, 0};
                d_load_safe_reg(context, &loadSafeReg);

                // Before we continue, we need to address something.
                // If there are any Returns in exe, then they will have no
                // pops for restoring safe registers... NONE OF THEM. Let's
                // go ahead and add the instructions that pop safe registers
                // this function uses.

                // We will add the pop instructions at the positions of return
                // markers, then remove the return markers to show that we've
                // dealt with them.

                for (size_t i = 0; i < exe.numStartOfReturnMarkers; i++) {
                    size_t returnMarker = exe.startOfReturnMarkers[i];

                    d_insert_bytecode(&exe, &loadSafeReg, returnMarker);
                }

                if (exe.startOfReturnMarkers != NULL) {
                    free(exe.startOfReturnMarkers);
                    exe.startOfReturnMarkers = NULL;
                }

                exe.numStartOfReturnMarkers = 0;

                d_free_bytecode(&loadSafeReg);

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
            // Now to generate the bytecode.
            // Firstly, we need to pop the arguments of the function from
            // the stack. But if it's a function, it may not have a Define
            // node!
            if (func->defineNode != NULL) {
                d_setup_arguments(func->defineNode, context, &out, false);
            }

            // Now we recursively generate the bytecode for the inputs
            // of the Return node, so the final Return values are
            // calculated.
            BCode funcCode = d_generate_bytecode_for_inputs(returnNode, context,
                                                            false, true);

            // Now we've compiled the function, we know what safe
            // registers it uses. Save these before the function
            // executes.
            d_save_safe_reg(context, &out);

            d_concat_bytecode(&out, &funcCode);
            d_free_bytecode(&funcCode);

            // While we're here, we'll load back the saved register
            // values.
            d_load_safe_reg(context, &out);

            // Now we need to push the return values onto the stack,
            // return, then we're done!
            d_setup_returns(returnNode, context, &out, false, true);
        }
    }

    // We've generated the bytecode for the function.
    // Since we copy the arguments of the function into different
    // registers to preserve the original data, the registers
    // containing the original data are never freed. BE FREE, MY
    // MINIONS!
    if (func->defineNode != NULL) {
        for (size_t j = 1 + (size_t)func->isSubroutine;
             j < func->defineNode->numSockets; j++) {
            SheetSocket *outputSocket = func->defineNode->sockets[j];

            if (!outputSocket->isInput &&
                (outputSocket->type & TYPE_VAR_ANY) != 0) {
                d_free_reg(context, outputSocket->_reg);
            }
        }
    }

    return out;
}
*/

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
    context.nextReg      = NUM_SAFE_GENERAL_REGISTERS;
    context.nextFloatReg = VM_REG_FLOAT_START + NUM_SAFE_FLOAT_REGISTERS;

    context.nextSafeReg      = 0;
    context.nextSafeFloatReg = VM_REG_FLOAT_START;

    // Set all of the bits in the register arrays to 0, so they're all "free".
    d_reg_new_function(&context);

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

        BCode code = d_generate_bytecode_for_function(func, &context);

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
        BCode start = d_generate_bytecode_for_start(sheet->startNode, &context);

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
