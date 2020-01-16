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

#include <stdbool.h>
#include "dcfg.h"
#include "dlink.h"
#include "dvm.h"

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

    size_t *startOfReturnMarkers; ///< A list of indexes which correspond to
                                  ///< the start of where functions prepare
                                  ///< their return values in the bytecode.

    size_t numStartOfReturnMarkers; ///< The number of start of return markers.

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
    reg_t nextReg;      ///< The next available general register.
    reg_t nextFloatReg; ///< The next available float register.

    reg_t nextSafeReg;      ///< The next available safe general register.
    reg_t nextSafeFloatReg; ///< The next available safe float register.

    char usedReg[VM_NUM_REG / 8]; ///< Each bit represents a register, the bit
                                  ///< being set to 1 means it is in use.
                                  ///< For a particular function, which
                                  ///< registers are in use at a particular
                                  ///< moment in time?

    char maxUsedReg[VM_NUM_REG / 8]; ///< Each bit represents a register, the
                                     ///< bit being set to 1 means it is in use.
                                     ///< For a particular function, what
                                     ///< registers does it use throughout
                                     ///< its execution?

    LinkMetaList linkMetaList; ///< A list of link metadata.

    char *dataSection;      ///< The data section being built up.
    size_t dataSectionSize; ///< The size of the data section.
} BuildContext;

/**
 * \def SHIFT_BYTE(byte, n)
 * \brief Shifts a byte `byte` `n` bytes left in a `dint`.
 */
#define SHIFT_BYTE(byte, n) (((dint)(byte)) << ((n)*8))

/**
 * \def IS_REG_USED(usedArray, reg)
 * \brief Return if a register is being used.
 */
#define IS_REG_USED(usedArray, reg) \
    (((usedArray)[(reg) / 8] & (1 << ((reg) % 8))))

/**
 * \def IS_REG_FREE(usedArray, reg)
 * \brief Return if a register is not being used.
 *
 * This is equivalent to `!IS_REG_USED(usedArray, reg)`
 */
#define IS_REG_FREE(usedArray, reg) !IS_REG_USED(usedArray, reg)

/**
 * \def SET_REG_FREE(usedArray, reg)
 * \brief Set a register to be free.
 */
#define SET_REG_FREE(usedArray, reg) \
    ((usedArray)[(reg) / 8] ^= (1 << ((reg) % 8))) // XOR

/**
 * \def SET_REG_USED(usedArray, reg)
 * \brief Set a register to be used.
 */
#define SET_REG_USED(usedArray, reg) \
    ((usedArray)[(reg) / 8] |= (1 << ((reg) % 8))) // OR

/*
    Macros to define the calling convention.

    Safe registers: Registers that are guaranteed to retain their value after
    a call has been made.
    Temporary registers: Registers that ARE NOT guaranteed to retain their value
    after a call has been made.

    Safe registers come before temporary ones.
*/
/**
 * \def NUM_SAFE_GENERAL_REGISTERS
 * \brief The number of safe general registers in the calling convention.
 */
#define NUM_SAFE_GENERAL_REGISTERS 32

/**
 * \def NUM_SAFE_FLOAT_REGISTERS
 * \brief The number of safe float registers in the calling convention.
 */
#define NUM_SAFE_FLOAT_REGISTERS 32

/**
 * \def IS_REG_SAFE_GENERAL(reg)
 * \brief Return if a register is a safe general register.
 */
#define IS_REG_SAFE_GENERAL(reg) \
    ((reg) >= 0 && (reg) < NUM_SAFE_GENERAL_REGISTERS)

/**
 * \def IS_REG_SAFE_FLOAT(reg)
 * \brief Return if a register is a safe float register.
 */
#define IS_REG_SAFE_FLOAT(reg)      \
    ((reg) >= VM_REG_FLOAT_START && \
     (reg) < VM_REG_FLOAT_START + NUM_SAFE_FLOAT_REGISTERS)

/**
 * \def IS_REG_SAFE(reg)
 * \brief Return if a register is safe, be it a general or a float register.
 */
#define IS_REG_SAFE(reg) (IS_REG_SAFE_GENERAL(reg) || IS_REG_SAFE_FLOAT(reg))

/*
=== STRUCTURE FUNCTIONS ===================================
*/

/**
 * \fn reg_t d_next_reg(BuildContext *context, reg_t *nextRegInContext,
 *                      reg_t end)
 * \brief Return the next available register index, and increment it afterwards.
 *
 * \return The next available register.
 *
 * \param context The build context.
 * \param nextRegInContext A pointer to the next register variable in the
 * context.
 * \param end The ending register index of the section we want to get.
 */
DECISION_API reg_t d_next_reg(BuildContext *context, reg_t *nextRegInContext,
                              reg_t end);

/**
 * \fn reg_t d_next_general_reg(BuildContext* context, bool safe)
 * \brief Return the next available general register index, and increment it
 * afterwards.
 *
 * \return The next available general register.
 *
 * \param context The build context.
 * \param safe Should the register be a safe register?
 */
DECISION_API reg_t d_next_general_reg(BuildContext *context, bool safe);

/**
 * \fn reg_t d_next_float_reg(BuildContext *context, bool safe)
 * \brief Return the next available float register index, and increment it
 * afterwards.
 *
 * \return The next available float register.
 *
 * \param context The build context.
 * \param safe Should the register be a safe register?
 */
DECISION_API reg_t d_next_float_reg(BuildContext *context, bool safe);

/**
 * \fn void d_free_general_reg(BuildContext *context, reg_t reg)
 * \brief Free a general register, and potentially change the next register to
 * use.
 *
 * \param context The build context.
 * \param reg The general register to free in the build context.
 */
DECISION_API void d_free_general_reg(BuildContext *context, reg_t reg);

/**
 * \fn void d_free_float_reg(BuildContext *context, reg_t reg)
 * \brief Free a float register, and potentially change the next register to
 * use.
 *
 * \param context The build context.
 * \param reg The float register to free in the build context.
 */
DECISION_API void d_free_float_reg(BuildContext *context, reg_t reg);

/**
 * \fn void d_free_reg(BuildContext *context, reg_t reg)
 * \brief Free a register.
 *
 * It will automatically call either `d_free_general_reg` or `d_free_float_reg`.
 *
 * \param context The build context.
 * \param reg The register to free in the build context.
 */
DECISION_API void d_free_reg(BuildContext *context, reg_t reg);

/**
 * \fn void d_reg_new_function(BuildContext *context)
 * \brief Set the contents of the register arrays in the context ready for
 * generating the bytecode for a new function.
 *
 * \param context The context to set.
 */
DECISION_API void d_reg_new_function(BuildContext *context);

/**
 * \fn bool d_is_all_reg_free(BuildContext *context)
 * \brief Are all of the registers in the context free?
 *
 * \return `true` if yes, `false` if no.
 *
 * \param context The build context whose registers we want to check.
 */
DECISION_API bool d_is_all_reg_free(BuildContext *context);

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
 * \fn void d_bytecode_set_byte(BCode bcode, size_t index, char byte)
 * \brief Given some bytecode, set a byte in the bytecode to a given value.
 *
 * \param bcode The bytecode to edit.
 * \param index The index of the byte in the bytecode to set.
 * \param byte The value to set.
 */
DECISION_API void d_bytecode_set_byte(BCode bcode, size_t index, char byte);

/**
 * \fn void d_bytecode_set_immediate(BCode bcode, size_t index,
 *                                   immediate_t immediate)
 * \brief Given some bytecode, set an immediate-sized value into the bytecode.
 *
 * \param bcode The bytecode to edit.
 * \param index The starting index of the section of the bytecode to edit.
 * \param immediate The immediate value to set.
 */
DECISION_API void d_bytecode_set_immediate(BCode bcode, size_t index,
                                           immediate_t immediate);

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

/**
 * \fn void d_insert_return_marker(BCode *bcode, size_t returnMarker)
 * \brief Insert a return marker into some bytecode.
 *
 * The marker represents the index of the first byte of the first instruction
 * of setting up return values for a function.
 *
 * \param bcode The bytecode to add the marker to.
 * \param returnMarker The marker to add.
 */
DECISION_API void d_insert_return_marker(BCode *bcode, size_t returnMarker);

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
 * \param insIndex The index of the LOADUI instruction to replace when
 * linking is taking place.
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
 * \fn BCode d_convert_between_number_types(SheetSocket *socket,
 *                                          BuildContext *context, DIns opcode,
 *                                          bool useSafeReg)
 * \brief Generate bytecode to convert from one number type to the other, i.e.
 * Integer -> Float or Float -> Integer.
 *
 * The new register is placed in the socket.
 *
 * \return The bytecode generated to convert.
 *
 * \param socket The input socket to convert.
 * \param context The context needed to generate the bytecode.
 * \param opcode The opcode to use to convert.
 * \param useSafeReg Specifies if the output should be in a safe register.
 */
DECISION_API BCode d_convert_between_number_types(struct _sheetSocket *socket,
                                                  BuildContext *context,
                                                  DIns opcode, bool useSafeReg);

/**
 * \fn void d_setup_input(SheetSocket *socket, BuildContext *context,
 *                        bool forceFloat, bool useSafeReg, BCode *addTo)
 * \brief Given an input socket, do what is nessesary to set it up for use in a
 * node.
 *
 * \param socket The socket corresponding to the input.
 * \param context The context needed to build bytecode.
 * \param forceFloat Always convert the input to a float if it isn't already.
 * \param useSafeReg Specifies if the input should be in a safe register.
 * \param addTo If any extra bytecode is needed to setup the input, add it onto
 * this bytecode.
 */
DECISION_API void d_setup_input(struct _sheetSocket *socket,
                                BuildContext *context, bool forceFloat,
                                bool useSafeReg, BCode *addTo);

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
DECISION_API void d_setup_arguments(struct _sheetNode *defineNode,
                                    BuildContext *context, BCode *addTo,
                                    bool isSubroutine);

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
DECISION_API void d_setup_returns(struct _sheetNode *returnNode,
                                  BuildContext *context, BCode *addTo,
                                  bool isSubroutine, bool retAtEnd);

/**
 * \fn void d_save_safe_reg(BuildContext *context, BCode *addTo)
 * \brief Given a function has just been compiled (and maxUsedReg has been
 * filled), generate the bytecode needed to save the safe registers onto the
 * stack before they are manipulated.
 *
 * \param context The context needed to generate the bytecode.
 * \param addTo Where to add the extra bytecode onto. Should be just after the
 * arguments of the function have been poped off the stack.
 */
DECISION_API void d_save_safe_reg(BuildContext *context, BCode *addTo);

/**
 * \fn void d_load_safe_reg(BuildContext *context, BCode *addTo)
 * \brief Given a return node has been reached, generate the bytecode needed to
 * load safe register values from the stack.
 *
 * Note that for subroutines, there can be multiple return nodes, each of which
 * could miss some safe registers to load. On those instances, we don't insert
 * anything until after we've compiled the entire subroutine, then we add on
 * the bytecode before every instance of a return.
 *
 * \param context The context needed to generate the bytecode.
 * \param addTo Where to add the extra bytecode onto. Should be just before
 * the return values of the function are pushed onto the stack.
 */
DECISION_API void d_load_safe_reg(BuildContext *context, BCode *addTo);

/**
 * \fn BCode d_generate_bytecode_for_literal(SheetSocket *socket,
 *                                           BuildContext *context,
 *                                           bool cvtFloat,
 *                                           bool useSafeReg)
 * \brief Given a socket, generate the bytecode to load the literal value.
 *
 * \return The malloc'd bytecode generated to get the literal value.
 *
 * \param socket The socket to get the literal value from.
 * \param context The context needed to generate the bytecode.
 * \param cvtFloat If true, and if the literal is an integer, convert it into a
 * float.
 * \param useSafeReg If true, the output will be placed in a safe register.
 */
DECISION_API BCode d_generate_bytecode_for_literal(struct _sheetSocket *socket,
                                                   BuildContext *context,
                                                   bool cvtFloat,
                                                   bool useSafeReg);

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
DECISION_API BCode d_generate_bytecode_for_input(struct _sheetSocket *socket,
                                                 BuildContext *context,
                                                 bool inLoop,
                                                 bool forceLiteral);

/**
 * \fn BCode d_generate_bytecode_for_inputs(SheetNode *node,
 *                                          BuildContext *context, bool inLoop)
 * \brief Given a node, generate the bytecode to get the values of the inputs.
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
DECISION_API BCode d_generate_bytecode_for_inputs(struct _sheetNode *node,
                                                  BuildContext *context,
                                                  bool inLoop,
                                                  bool forceLiterals);

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
DECISION_API BCode
d_generate_bytecode_for_variable(struct _sheetNode *node, BuildContext *context,
                                 struct _sheetVariable *variable);

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
DECISION_API BCode d_generate_bytecode_for_call(struct _sheetNode *node,
                                                BuildContext *context,
                                                bool isSubroutine,
                                                bool isCFunction);

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
DECISION_API BCode d_generate_bytecode_for_operator(
    struct _sheetNode *node, BuildContext *context, DIns opcode, DIns fopcode,
    DIns iopcode, bool oneInput, bool infiniteInputs, bool forceFloat);

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
DECISION_API BCode d_generate_bytecode_for_comparator(struct _sheetNode *node,
                                                      BuildContext *context,
                                                      DIns opcode, DIns fopcode,
                                                      DIns sopcode,
                                                      bool notAfter);

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
DECISION_API BCode d_generate_bytecode_for_nonexecution_node(
    struct _sheetNode *node, BuildContext *context, bool inLoop);

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
DECISION_API BCode d_generate_bytecode_for_execution_node(
    struct _sheetNode *node, BuildContext *context, bool retAtEnd, bool inLoop);

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
DECISION_API BCode d_generate_bytecode_for_start(struct _sheetNode *startNode,
                                                 BuildContext *context);

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
DECISION_API BCode d_generate_bytecode_for_function(struct _sheetFunction *func,
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
