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
 * \file dcodegen.h
 * \brief This header generates the bytecode from the user's source code,
 * so that it can be run on a Decision VM.
 */

#ifndef DCODEGEN_H
#define DCODEGEN_H

#include "dcfg.h"
#include "dlink.h"
#include "dvm.h"
#include <stdbool.h>

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

/* Forward declaration of the InstructionToLink struct from dsheet.h */
struct _insToLink;

/**
 * \struct _bcode
 * \brief A struct for nodes to return what instructions they created.
 *
 * \typedef struct _bcode BCode
 */
typedef struct _bcode {
    char *code;  ///< The bytecode as an array of bytes.
    size_t size; ///< The size of the bytecode in bytes.

    struct _insToLink *linkList; ///< An array of instructions that will need
                                 ///< to be linked.
    size_t linkListSize;         ///< The size of the `linkList` array.
} BCode;

/**
 * \struct _buildContext
 * \brief A struct to keep track of information as we're building the
 * bytecode.
 *
 * \typedef struct _buildContext BuildContext
 */
typedef struct _buildContext {
    int stackTop; ///< Where the stack pointer is relative to the base pointer.

    LinkMetaList linkMetaList; ///< A list of link metadata.

    char *dataSection;      ///< The data section being built up.
    size_t dataSectionSize; ///< The size of the data section.
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

/**
 * \fn BCode d_malloc_bytecode(size_t size)
 * \brief Create a malloc'd BCode object, with a set number of bytes.
 *
 * \return The BCode object with malloc'd elements.
 *
 * \param size The number of bytes.
 */
DECISION_API BCode d_malloc_bytecode(size_t size);

/**
 * \fn BCode d_bytecode_ins(DIns opcode)
 * \brief Quickly create bytecode that is the size of an opcode, which also has
 * its first byte set as the opcode itself.
 *
 * \return The opcode-initialised bytecode.
 *
 * \param opcode The opcode to initialise with.
 */
DECISION_API BCode d_bytecode_ins(DIns opcode);

/**
 * \fn void d_bytecode_set_byte(BCode bcode, size_t index, char byte)
 * \brief Given some bytecode, set a byte in the bytecode to a given value.
 *
 * \param bcode The bytecode to edit.
 * \param index The index of the byte in the bytecode to set.
 * \param byte The value to set.
 */
DECISION_API void d_bytecode_set_byte(BCode bcode, size_t index, char byte);

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
DECISION_API void d_bytecode_set_fimmediate(BCode bcode, size_t index,
                                            fimmediate_t fimmediate);

/**
 * \fn void d_free_bytecode(BCode *bcode)
 * \brief Free malloc'd elements of bytecode.
 *
 * \param bcode The bytecode to free.
 */
DECISION_API void d_free_bytecode(BCode *bcode);

/**
 * \fn void d_concat_bytecode(BCode *base, BCode *after)
 * \brief Append bytecode to the end of another set of bytecode.
 *
 * \param base The bytecode to be added to.
 * \param after The bytecode to append. Not changed.
 */
DECISION_API void d_concat_bytecode(BCode *base, BCode *after);

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
DECISION_API void d_insert_bytecode(BCode *base, BCode *insertCode,
                                    size_t insertIndex);

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
                                      struct _sheetVariable *variable,
                                      size_t size, size_t indexInLinkMeta);

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
DECISION_API bool d_is_node_call(struct _sheetNode *node);

/**
 * \fn bool d_does_input_involve_call(SheetNode *node)
 * \brief Does getting the input of a node require calling another function?
 *
 * \return The answer to the above question.
 *
 * \param node The node whose inputs to query.
 */
DECISION_API bool d_does_input_involve_call(struct _sheetNode *node);

/**
 * \fn bool d_does_output_involve_call(SheetNode *node)
 * \brief Does the execution sequence starting from node require a call?
 *
 * \return The answer to the above question.
 *
 * \param node The node to start querying from.
 */
DECISION_API bool d_does_output_involve_call(struct _sheetNode *node);

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
DECISION_API BCode d_push_literal(struct _sheetSocket *socket,
                                  BuildContext *context, bool cvtFloat);

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
DECISION_API BCode d_push_variable(struct _sheetNode *node,
                                   BuildContext *context);

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
DECISION_API BCode d_push_input(struct _sheetSocket *socket,
                                BuildContext *context, bool forceFloat);

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
DECISION_API BCode d_push_node_inputs(struct _sheetNode *node,
                                      BuildContext *context, bool order,
                                      bool ignoreLiterals, bool forceFloat);

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
DECISION_API BCode d_generate_operator(struct _sheetNode *node,
                                       BuildContext *context, DIns opcode,
                                       DIns fopcode, DIns fiopcode,
                                       bool forceFloat);

/**
 * \fn BCode d_generate_comparator(SheetNode *node, BuildContext *context,
 *                                 DIns opcode, DIns fopcode, bool notAfter)
 * \brief Given a comparator node, generate the bytecode for it.
 *
 * \return Bytecode to get the output of a comparator.
 *
 * \param node The comparator node to get the result for.
 * \param context The context needed to generate the bytecode.
 * \param opcode The comparator instruction.
 * \param fopcode The float variant of the instruction.
 * \param notAfter Do we invert the answer at the end?
 */
DECISION_API BCode d_generate_comparator(struct _sheetNode *node,
                                         BuildContext *context, DIns opcode,
                                         DIns fopcode, bool notAfter);

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
DECISION_API BCode d_generate_call(struct _sheetNode *node,
                                   BuildContext *context);

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
DECISION_API BCode d_push_argument(struct _sheetSocket *socket,
                                   BuildContext *context);

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
DECISION_API BCode d_generate_return(struct _sheetNode *returnNode,
                                     BuildContext *context);

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
DECISION_API BCode d_generate_nonexecution_node(struct _sheetNode *node,
                                                BuildContext *context);

/**
 * \fn BCode d_generate_execution_node(SheetNode *node, BuildContext* context)
 * \brief Given an execution node, generate the bytecode to get the output.
 *
 * \return Bytecode to run the execution node's subroutine.
 *
 * \param node The execution node.
 * \param context The context needed to generate the bytecode.
 */
DECISION_API BCode d_generate_execution_node(struct _sheetNode *node,
                                             BuildContext *context);

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
DECISION_API BCode d_generate_start(struct _sheetNode *startNode,
                                    BuildContext *context);

/**
 * \fn BCode d_generate_function(SheetFunction *func, BuildContext *context)
 * \brief Given a function, generate the bytecode for it.
 *
 * \return The bytecode generated for the function.
 *
 * \param func The function to generate the bytecode for.
 * \param context The context needed to generate the bytecode.
 */
DECISION_API BCode d_generate_function(struct _sheetFunction *func,
                                       BuildContext *context);

/**
 * \fn void d_codegen_compile(Sheet *sheet)
 * \brief Given that Semantic Analysis has taken place, generate the bytecode
 * for a given sheet.
 *
 * \param sheet The sheet to generate the bytecode for.
 */
DECISION_API void d_codegen_compile(struct _sheet *sheet);

#endif // DCODEGEN_H
