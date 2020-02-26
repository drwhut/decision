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
 * \file dcodegen.h
 * \brief This header generates the bytecode from the user's source code,
 * so that it can be run on a Decision VM.
 */

#ifndef DCODEGEN_H
#define DCODEGEN_H

#include "dasm.h"
#include "dcfg.h"
#include "ddebug.h"
#include "dlink.h"
#include "dsheet.h"
#include "dvm.h"
#include <stdbool.h>

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/**
 * \struct _buildContext
 * \brief A struct to keep track of information as we're building the
 * bytecode.
 *
 * \typedef struct _buildContext BuildContext
 */
typedef struct _buildContext {
    Graph graph; ///< The graph we're building for.

    int stackTop; ///< Where the stack pointer is relative to the base pointer.

    LinkMetaList linkMetaList; ///< A list of link metadata.

    char *dataSection;      ///< The data section being built up.
    size_t dataSectionSize; ///< The size of the data section.

    bool debug;          ///< Do we want to build up debugging information?
} BuildContext;

/*
=== STRUCTURE FUNCTIONS ===================================
*/

/**
 * \def STACK_INDEX_TOP(context, index)
 * \brief Given the index from the base of the stack, get the index from the
 * top of the stack with the current context.
 */
#define STACK_INDEX_TOP(context, index) ((index) - (context)->stackTop)

/**
 * \def IS_INDEX_TOP(context, index)
 * \brief Given the index from the base of the stack, is the index the top of
 * the stack?
 */
#define IS_INDEX_TOP(context, index) ((index) == (context)->stackTop)

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
DECISION_API void d_add_link_to_ins(BuildContext *context, BCode *bcode,
                                    size_t insIndex, LinkMeta linkMeta,
                                    size_t *indexInList, bool *wasDuplicate);

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
DECISION_API char *d_allocate_from_data_section(BuildContext *context,
                                                size_t size, size_t *index);

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
DECISION_API size_t d_allocate_string_literal_in_data(BuildContext *context,
                                                      BCode *linkCode,
                                                      size_t insIndex,
                                                      char *stringLiteral);

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
DECISION_API void d_allocate_variable(BuildContext *context,
                                      SheetVariable *variable, size_t size,
                                      size_t indexInLinkMeta);

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
DECISION_API BCode d_push_literal(BuildContext *context, NodeSocket socket,
                                  bool cvtFloat);

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
DECISION_API BCode d_push_variable(BuildContext *context, size_t nodeIndex);

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
DECISION_API BCode d_push_input(BuildContext *context, NodeSocket socket,
                                bool forceFloat);

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
DECISION_API BCode d_push_node_inputs(BuildContext *context, size_t nodeIndex,
                                      bool order, bool ignoreLiterals,
                                      bool forceFloat);

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
DECISION_API BCode d_generate_operator(BuildContext *context, size_t nodeIndex,
                                       DIns opcode, DIns fopcode, DIns fiopcode,
                                       bool forceFloat);

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
DECISION_API BCode d_generate_comparator(BuildContext *context,
                                         size_t nodeIndex, DIns opcode,
                                         DIns fopcode, fimmediate_t strCmpArg,
                                         bool notAfter);

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
DECISION_API BCode d_generate_call(BuildContext *context, size_t nodeIndex);

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
DECISION_API BCode d_push_argument(BuildContext *context, NodeSocket socket);

/**
 * \fn BCode d_generate_return(BuildContext *context, SheetNode *returnNode)
 * \brief Given a Return node, generate the bytecode to return from the
 * function/subroutine with the return values.
 *
 * \return Bytecode to return from the function/subroutine.
 *
 * \param context The context needed to generate the bytecode.
 * \param returnNode The Return node to return with.
 */
DECISION_API BCode d_generate_return(BuildContext *context,
                                     size_t returnNodeIndex);

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
DECISION_API BCode d_generate_nonexecution_node(BuildContext *context,
                                                size_t nodeIndex);

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
DECISION_API BCode d_generate_execution_node(BuildContext *context,
                                             size_t nodeIndex, bool retAtEnd);

/**
 * \fn BCode d_generate_start(BuildContext *context, SheetNode *startNode)
 * \brief Given a Start node, generate the bytecode for the sequence starting
 * from this node.
 *
 * \return The bytecode generated for the Start function.
 *
 * \param context The context needed to generate the bytecode.
 * \param startNode A pointer to the Start node.
 */
DECISION_API BCode d_generate_start(BuildContext *context,
                                    size_t startNodeIndex);

/**
 * \fn BCode d_generate_function(BuildContext *context, SheetFunction func)
 * \brief Given a function, generate the bytecode for it.
 *
 * \return The bytecode generated for the function.
 *
 * \param context The context needed to generate the bytecode.
 * \param func The function to generate the bytecode for.
 */
DECISION_API BCode d_generate_function(BuildContext *context,
                                       SheetFunction func);

/**
 * \fn void d_codegen_compile(Sheet *sheet, bool debug)
 * \brief Given that Semantic Analysis has taken place, generate the bytecode
 * for a given sheet.
 *
 * \param sheet The sheet to generate the bytecode for.
 * \param debug If true, we include debug information as well.
 */
DECISION_API void d_codegen_compile(Sheet *sheet, bool debug);

#endif // DCODEGEN_H
