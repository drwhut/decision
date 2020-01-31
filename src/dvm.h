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
 * \file dvm.h
 * \brief This header contains functionality for the Decision VM - the object
 * that will run the generated bytecode.
 */

#ifndef DVM_H
#define DVM_H

#include "dcfg.h"
#include "derror.h"
#include <stdbool.h>

#include <stdint.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/**
 * \enum _dIns
 * \brief The Decision VM Instruction Set.
 *
 * \typedef enum _dIns DIns
 */
typedef enum _dIns {
    OP_RET     = 0,  ///< Equivalent to RETN 0.
    OP_RETN    = 1,  ///< pop(stackFrame w/ I(1) return values)
    OP_ADD     = 2,  ///< push(pop() + pop())
    OP_ADDF    = 3,  ///< pushFloat(popFloat() + popFloat())
    OP_ADDBI   = 4,  ///< push(pop() + I(1))
    OP_ADDHI   = 5,  ///< push(pop() + I(|M|/2))
    OP_ADDFI   = 6,  ///< push(pop() + I(|M|))
    OP_AND     = 7,  ///< push(pop() & pop())
    OP_ANDBI   = 8,  ///< push(pop() & I(1))
    OP_ANDHI   = 9,  ///< push(pop() & I(|M|/2))
    OP_ANDFI   = 10, ///< push(pop() & I(|M|))
    OP_CALL    = 11, ///< pc = pop(); push(stackFrame w/ I(1) arguments)
    OP_CALLC   = 12, ///< (*pop())(this) w/ I(1) arguments
    OP_CALLCI  = 13, ///< (*I(|M|))(this) w/ I(1) arguments
    OP_CALLI   = 14, ///< pc = I(|M|); push(stackFrame w/ I(1) arguments)
    OP_CALLR   = 15, ///< pc += pop(); push(stackFrame w/ I(1) arguments)
    OP_CALLRB  = 16, ///< pc += I(1); push(stackFrame w/ I(1) arguments)
    OP_CALLRH  = 17, ///< pc += I(|M|/2); push(stackFrame w/ I(1) arguments)
    OP_CALLRF  = 18, ///< pc += I(|M|); push(stackFrame w/ I(1) arguments)
    OP_CEQ     = 19, ///< push(pop() == pop())
    OP_CEQF    = 20, ///< push(popFloat() == popFloat())
    OP_CLEQ    = 21, ///< push(pop() <= pop())
    OP_CLEQF   = 22, ///< push(popFloat() <= popFloat())
    OP_CLT     = 23, ///< push(pop() < pop())
    OP_CLTF    = 24, ///< push(popFloat() < popFloat())
    OP_CMEQ    = 25, ///< push(pop() >= pop())
    OP_CMEQF   = 26, ///< push(popFloat() >= popFloat())
    OP_CMT     = 27, ///< push(pop() > pop())
    OP_CMTF    = 28, ///< push(popFloat() > popFloat())
    OP_CVTF    = 29, ///< push((dfloat)pop())
    OP_CVTI    = 30, ///< push((dint)pop())
    OP_DEREF   = 31, ///< push(*pop())
    OP_DEREFI  = 32, ///< push(*I(|M|))
    OP_DEREFB  = 33, ///< push(*((uint8_t *)pop()))
    OP_DEREFBI = 34, ///< push(*((uint8_t *)I(|M|)))
    OP_DIV     = 35, ///< push(pop() / pop())
    OP_DIVF    = 36, ///< pushFloat(popFloat() / popFloat())
    OP_DIVBI   = 37, ///< push(pop() / I(1))
    OP_DIVHI   = 38, ///< push(pop() / I(|M|/2))
    OP_DIVFI   = 39, ///< push(pop() / I(|M|))
    OP_GET     = 40, ///< push(get(pop()))
    OP_GETBI   = 41, ///< push(get(I(1)))
    OP_GETHI   = 42, ///< push(get(I(|M|/2)))
    OP_GETFI   = 43, ///< push(get(I(|M|)))
    OP_J       = 44, ///< pc = pop()
    OP_JCON    = 45, ///< IF pop() THEN pc = pop() ELSE pop()
    OP_JCONI   = 46, ///< IF pop() THEN pc = I(|M|)
    OP_JI      = 47, ///< pc = I(|M|)
    OP_JR      = 48, ///< pc += pop()
    OP_JRBI    = 49, ///< pc += I(1)
    OP_JRHI    = 50, ///< pc += I(|M|/2)
    OP_JRFI    = 51, ///< pc += I(|M|)
    OP_JRCON   = 52, ///< IF pop() THEN pc += pop() ELSE pop()
    OP_JRCONBI = 53, ///< IF pop() THEN pc += I(1)
    OP_JRCONHI = 54, ///< IF pop() THEN pc += I(|M|/2)
    OP_JRCONFI = 55, ///< IF pop() THEN pc += I(|M|)
    OP_MOD     = 56, ///< push(pop() % pop())
    OP_MODBI   = 57, ///< push(pop() % I(1))
    OP_MODHI   = 58, ///< push(pop() % I(|M|/2))
    OP_MODFI   = 59, ///< push(pop() % I(|M|))
    OP_MUL     = 60, ///< push(pop() * pop())
    OP_MULF    = 61, ///< pushFloat(popFloat() * popFloat())
    OP_MULBI   = 62, ///< push(pop() * I(1))
    OP_MULHI   = 63, ///< push(pop() * I(|M|/2))
    OP_MULFI   = 64, ///< push(pop() * I(|M|))
    OP_NOT     = 65, ///< push(!pop())
    OP_OR      = 66, ///< push(pop() | pop())
    OP_ORBI    = 67, ///< push(pop() | I(1))
    OP_ORHI    = 68, ///< push(pop() | I(|M|/2))
    OP_ORFI    = 69, ///< push(pop() | I(|M|))
    OP_POP     = 70, ///< pop() once
    OP_POPB    = 71, ///< pop() I(1) times
    OP_POPH    = 72, ///< pop() I(|M|/2) times
    OP_POPF    = 73, ///< pop() I(|M|) times
    OP_PUSHB   = 74, ///< push(I(1))
    OP_PUSHH   = 75, ///< push(I(|M|/2))
    OP_PUSHF   = 76, ///< push(I(|M|))
    OP_PUSHNB  = 77, ///< push(0) I(1) times
    OP_PUSHNH  = 78, ///< push(0) I(|M|/2) times
    OP_PUSHNF  = 79, ///< push(0) I(|M|) times
    OP_SETADR  = 80, ///< *((dint *)pop()) = pop()
    OP_SETADRB = 81, ///< *((uint8_t *)pop()) = pop()
    OP_SUB     = 82, ///< push(pop() - pop())
    OP_SUBF    = 83, ///< pushFloat(popFloat() - popFloat())
    OP_SUBBI   = 84, ///< push(pop() - I(1))
    OP_SUBHI   = 85, ///< push(pop() - I(|M|/2))
    OP_SUBFI   = 86, ///< push(pop() - I(|M|))
    OP_SYSCALL = 87, ///< push(syscall(I(1), pop(), pop(), pop()))
    OP_XOR     = 88, ///< push(pop() ^ pop())
    OP_XORBI   = 89, ///< push(pop() ^ I(1))
    OP_XORHI   = 90, ///< push(pop() ^ I(|M|/2))
    OP_XORFI   = 91, ///< push(pop() ^ I(|M|))
} DIns;

/**
 * \def NUM_OPCODES
 * \brief Macro constant representing the number of opcodes.
 */
#define NUM_OPCODES (OP_XORFI + 1)

/**
 * \enum _dSyscall
 * \brief The Decision VM Syscall specification.
 *
 * \typedef enum _dSyscall DSyscall
 */
typedef enum _dSyscall {
    SYS_PRINT = 0, ///< Print a value to `stdout`.
                   ///< * `arg0`: `0`: `Integer`, `1`: `Float`, `2`: `String`,
                   ///< `3`: `Boolean`.
                   ///< * `arg1`: If set to `1`, it will print a newline at the
                   ///< end, otherwise it will not.
                   ///< * `arg2`: The value to print.
                   ///< * Returns: The value 0.

    SYS_STRCMP = 1, ///< Compare two strings.
                    ///< * `arg0`: `0`: Equal, `1`: Less Than or Equal,
                    ///< `2`: Less Than, `3`: More Than or Equal,
                    ///< `4`: More Than.
                    ///< * `arg1`: The first string pointer.
                    ///< * `arg2`: The second string pointer.
                    ///< * Returns: 1 if true, 0 if false.

    SYS_STRLEN = 2, ///< Get the length of a string.
                    ///< * `arg0`: Unused.
                    ///< * `arg1`: Unused.
                    ///< * `arg2`: The string to get the length of.
                    ///< * Returns: The length of the string.
} DSyscall;

/**
 * \def VM_STACK_SIZE_MIN
 * \brief The minimum, and starting, size of the VM's stack.
 */
#define VM_STACK_SIZE_MIN 16

/**
 * \def VM_STACK_SIZE_SCALE_INC
 * \brief How much should the stack size increase once it reaches capacity?
 */
#define VM_STACK_SIZE_SCALE_INC 1.5

/**
 * \def VM_STACK_SIZE_SCALE_DEC
 * \brief How much should the stack size decrease to save memory?
 */
#define VM_STACK_SIZE_SCALE_DEC 0.5

/**
 * \enum _DVM
 * \brief The Decision VM structure.
 *
 * \typedef struct _DVM DVM
 */
typedef struct _DVM {
    char *pc;              ///< The program counter.
    unsigned char _inc_pc; ///< How many bytes to increment the program counter.
                           ///< This is determined automatically.

    dint *basePtr;  ///< A pointer to the base of the stack.
    dint *stackPtr; ///< A pointer to the top of the stack.
    dint *framePtr; ///< A pointer to the start of the stack frame.

    duint stackSize; ///< The current size of the stack.

    bool halted;       ///< The halted flag.
    bool runtimeError; ///< The runtime error flag.
} DVM;

#define BIMMEDIATE_SIZE   1
#define BIMMEDIATE_PRINTF "hh"
#define bimmediate_t      int8_t

#ifdef DECISION_32
#define HIMMEDIATE_SIZE   2
#define HIMMEDIATE_PRINTF "h"
#define himmediate_t      int16_t
#define FIMMEDIATE_SIZE   4
#define FIMMEDIATE_PRINTF "l"
#define fimmediate_t      int32_t
#else
#define HIMMEDIATE_SIZE   4
#define HIMMEDIATE_PRINTF "l"
#define himmediate_t      int32_t
#define FIMMEDIATE_SIZE   8
#define FIMMEDIATE_PRINTF "ll"
#define fimmediate_t      int64_t
#endif // DECISION_32

/*
=== STACK FUNCTIONS =======================================
*/

/**
 * \fn size_t d_vm_frame(DVM *vm)
 * \brief Get the number of elements in the current stack frame.
 *
 * \return The number of elements in the stack frame.
 *
 * \param vm The VM whose stack to query.
 */
DECISION_API size_t d_vm_frame(DVM *vm);

/**
 * \fn dint d_vm_get(DVM *vm, dint index)
 * \brief Get an integer from a value in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \return The integer value of the stack at the given index.
 *
 * \param vm The VM whose stack to retrieve from.
 * \param index The index of the stack.
 */
DECISION_API dint d_vm_get(DVM *vm, dint index);

/**
 * \fn dfloat d_vm_get_float(DVM *vm, dint index)
 * \brief Get a float from a value in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \return The float value of the stack at the given index.
 *
 * \param vm The VM whose stack to retrieve from.
 * \param index The index of the stack.
 */
DECISION_API dfloat d_vm_get_float(DVM *vm, dint index);

/**
 * \fn void *d_vm_get_ptr(DVM *vm, dint index)
 * \brief Get a pointer from a value in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \return The pointer value of the stack at the given index.
 *
 * \param vm The VM whose stack to retrieve from.
 * \param index The index of the stack.
 */
DECISION_API void *d_vm_get_ptr(DVM *vm, dint index);

/**
 * \fn void d_vm_insert(DVM *vm, dint index, dint value)
 * \brief Insert an integer into the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to insert to.
 * \param index The index of the stack to insert to, i.e. `value` will be at
 * this location when the function returns.
 * \param value The value to insert into the stack.
 */
DECISION_API void d_vm_insert(DVM *vm, dint index, dint value);

/**
 * \fn void d_vm_insert_float(DVM *vm, dint index, dfloat value)
 * \brief Insert a float into the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to insert to.
 * \param index The index of the stack to insert to, i.e. `value` will be at
 * this location when the function returns.
 * \param value The value to insert into the stack.
 */
DECISION_API void d_vm_insert_float(DVM *vm, dint index, dfloat value);

/**
 * \fn void d_vm_insert_ptr(DVM *vm, dint index, void *ptr)
 * \brief Insert a pointer into the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to insert to.
 * \param index The index of the stack to insert to, i.e. `ptr` will be at this
 * location when the function returns.
 * \param ptr The pointer to insert into the stack.
 */
DECISION_API void d_vm_insert_ptr(DVM *vm, dint index, void *ptr);

/**
 * \fn dint d_vm_pop(DVM *vm)
 * \brief Pop an integer from the top of the stack.
 *
 * \return The integer at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
DECISION_API dint d_vm_pop(DVM *vm);

/**
 * \fn void d_vm_popn(DVM *vm, size_t n)
 * \brief Pop `n` elements from the stack.
 *
 * \param vm The VM whose stack to pop from.
 * \param n The number of elements to pop.
 */
DECISION_API void d_vm_popn(DVM *vm, size_t n);

/**
 * \fn dfloat d_vm_pop_float(DVM *vm)
 * \brief Pop a float from the top of the stack.
 *
 * \return The float at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
DECISION_API dfloat d_vm_pop_float(DVM *vm);

/**
 * \fn void *d_vm_pop_ptr(DVM *vm)
 * \brief Pop a pointer from the top of the stack.
 *
 * \return The pointer at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
DECISION_API void *d_vm_pop_ptr(DVM *vm);

/**
 * \fn void d_vm_push(DVM *vm, dint value)
 * \brief Push an integer value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
DECISION_API void d_vm_push(DVM *vm, dint value);

/**
 * \fn void d_vm_pushn(DVM *vm, size_t n)
 * \brief Push `0` onto the stack `n` times.
 *
 * \param vm The VM whose stack to push onto.
 * \param n The number of items to push onto the stack.
 */
DECISION_API void d_vm_pushn(DVM *vm, size_t n);

/**
 * \fn void d_vm_push_float(DVM *vm, dfloat value)
 * \brief Push a float value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
DECISION_API void d_vm_push_float(DVM *vm, dfloat value);

/**
 * \fn void d_vm_push_ptr(DVM *vm, void *ptr)
 * \brief Push a pointer onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param ptr The pointer to push onto the stack.
 */
DECISION_API void d_vm_push_ptr(DVM *vm, void *ptr);

/**
 * \fn void d_vm_remove(DVM *vm, dint index)
 * \brief Remove the value from the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to remove from.
 * \param index The index to remove from the stack.
 */
DECISION_API void d_vm_remove(DVM *vm, dint index);

/**
 * \fn void d_vm_remove_len(DVM *vm, dint index, size_t len)
 * \brief Remove a number of values from the stack, starting at a particular
 * index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to remove from.
 * \param index The index to start removing from the stack.
 * \param len The number of items to remove from the stack.
 */
DECISION_API void d_vm_remove_len(DVM *vm, dint index, size_t len);

/**
 * \fn void d_vm_set(DVM *vm, dint index, dint value)
 * \brief Set the value of an element in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param value The value to set.
 */
DECISION_API void d_vm_set(DVM *vm, dint index, dint value);

/**
 * \fn void d_vm_set_float(DVM *vm, dint index, dfloat value)
 * \brief Set the value of an element in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param value The value to set.
 */
DECISION_API void d_vm_set_float(DVM *vm, dint index, dfloat value);

/**
 * \fn void d_vm_set_ptr(DVM *vm, dint index, void *ptr)
 * \brief Set the value of an element in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param ptr The value to set.
 */
DECISION_API void d_vm_set_ptr(DVM *vm, dint index, void *ptr);

/**
 * \fn size_t d_vm_top(DVM *vm)
 * \brief Get the number of elements in the stack.
 *
 * \return The number of elements in the stack.
 *
 * \param vm The VM whose stack to query.
 */
DECISION_API size_t d_vm_top(DVM *vm);

/*
=== VM FUNCTIONS ==========================================
*/

/**
 * \fn const unsigned char d_vm_ins_size(DIns opcode)
 * \brief Given an opcode, get the total size of the instruction involving that
 * opcode in bytes.
 *
 * \return The size of the instruction in bytes. 0 if the opcode doesn't exist.
 *
 * \param opcode The opcode to query.
 */
DECISION_API const unsigned char d_vm_ins_size(DIns opcode);

/**
 * \fn DVM d_vm_create()
 * \brief Create a Decision VM in its starting state, with malloc'd elements.
 *
 * \return A Decision VM in its starting state.
 */
DECISION_API DVM d_vm_create();

/**
 * \fn void d_vm_reset(DVM *vm)
 * \brief Reset a Decision VM to its starting state.
 *
 * \param vm A Decision VM to set to its starting state.
 */
DECISION_API void d_vm_reset(DVM *vm);

/**
 * \fn void d_vm_free(DVM *vm)
 * \brief Free the malloc'd elements of a Decision VM. Note that this makes the
 * VM unusable unless you call `d_vm_reset` on it.
 *
 * \param vm The Decision VM to free.
 */
DECISION_API void d_vm_free(DVM *vm);

/**
 * \fn void d_vm_runtime_error(DVM *vm, const char *error)
 * \brief Print a runtime error to `stdout`, and halt the VM.
 *
 * Unlike compiler errors, these errors aren't stored anywhere.
 *
 * \param vm The VM that came across the error.
 * \param error The error message to display.
 */
DECISION_API void d_vm_runtime_error(DVM *vm, const char *error);

/**
 * \def ERROR_RUNTIME(vm, ...)
 * \brief A macro function to be able to print formatted error messages.
 *
 * This is to `d_vm_runtime_error` what `ERROR_COMPILER` is to
 * `d_error_compiler_push`.
 */
#ifdef DECISION_SAFE_FUNCTIONS
#define ERROR_RUNTIME(vm, ...)                          \
    {                                                   \
        char errMsg[MAX_ERROR_SIZE];                    \
        sprintf_s(errMsg, MAX_ERROR_SIZE, __VA_ARGS__); \
        d_vm_runtime_error((vm), errMsg);               \
    }
#else
#define ERROR_RUNTIME(vm, ...)            \
    {                                     \
        char errMsg[MAX_ERROR_SIZE];      \
        sprintf(errMsg, __VA_ARGS__);     \
        d_vm_runtime_error((vm), errMsg); \
    }
#endif // DECISION_SAFE_FUNCTIONS

/**
 * \fn void d_vm_parse_ins_at_pc(DVM *vm)
 * \brief Given a Decision VM, at it's current position in the program, parse
 * the instruction at that position.
 *
 * \param vm The VM to use to parse the instruction.
 */
DECISION_API void d_vm_parse_ins_at_pc(DVM *vm);

/**
 * \fn void d_vm_add_pc(DVM *vm, dint rel)
 * \brief Add to the program counter to go +rel bytes.
 *
 * **NOTE:** The VM will ALWAYS increment the PC after any instruction that
 * isn't a jump, call or return.
 *
 * \param vm The VM whose PC to add to.
 * \param rel How many bytes to go forward. *Can* be negative.
 */
DECISION_API void d_vm_add_pc(DVM *vm, dint rel);

/**
 * \fn void d_vm_inc_pc(DVM *vm)
 * \brief Increment the program counter in a Decision VM, to go to the next
 * instruction.
 *
 * **NOTE:** The VM will ALWAYS increment the PC after any instruction that
 * isn't a jump, call or return.
 *
 * \param vm The VM whose PC to add to.
 */
DECISION_API void d_vm_inc_pc(DVM *vm);

/**
 * \fn bool d_vm_run(DVM *vm, void *start)
 * \brief Get a virtual machine to start running instructions in a loop, until
 * it is halted.
 *
 * \return If it ran without any runtime errors.
 *
 * \param vm The VM to run the bytecode in.
 * \param start A pointer to the start of the bytecode to execute.
 */
DECISION_API bool d_vm_run(DVM *vm, void *start);

/**
 * \fn void d_vm_dump(DVM *vm)
 * \brief Dump the contents of a Decision VM to stdout for debugging.
 *
 * \param vm The VM to dump the contents of.
 */
DECISION_API void d_vm_dump(DVM *vm);

#endif // DVM_H
