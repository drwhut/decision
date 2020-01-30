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

#include "dvm.h"

#include "dcfunc.h"
#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A constant array of the size of each opcode's instruction in bytes. */
static const unsigned char VM_INS_SIZE[NUM_OPCODES] = {
    1,                                     // OP_RET
    1 + BIMMEDIATE_SIZE,                   // OP_RETN
    1,                                     // OP_ADD
    1,                                     // OP_ADDF
    1 + BIMMEDIATE_SIZE,                   // OP_ADDBI
    1 + HIMMEDIATE_SIZE,                   // OP_ADDHI
    1 + FIMMEDIATE_SIZE,                   // OP_ADDFI
    1,                                     // OP_AND
    1 + BIMMEDIATE_SIZE,                   // OP_ANDBI
    1 + HIMMEDIATE_SIZE,                   // OP_ANDHI
    1 + FIMMEDIATE_SIZE,                   // OP_ANDFI
    1 + BIMMEDIATE_SIZE,                   // OP_CALL
    1,                                     // OP_CALLC
    1 + FIMMEDIATE_SIZE,                   // OP_CALLCI
    1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLI
    1 + BIMMEDIATE_SIZE,                   // OP_CALLR
    1 + BIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRB
    1 + HIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRH
    1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRF
    1,                                     // OP_CEQ
    1,                                     // OP_CEQF
    1,                                     // OP_CLEQ
    1,                                     // OP_CLEQF
    1,                                     // OP_CLT
    1,                                     // OP_CLTF
    1,                                     // OP_CMEQ
    1,                                     // OP_MEQF
    1,                                     // OP_CMT
    1,                                     // OP_CMTF
    1,                                     // OP_CVTF
    1,                                     // OP_CVTI
    1,                                     // OP_DEREF
    1 + FIMMEDIATE_SIZE,                   // OP_DEREFI
    1,                                     // OP_DEREFB
    1 + FIMMEDIATE_SIZE,                   // OP_DEREFBI
    1,                                     // OP_DIV
    1,                                     // OP_DIVF
    1 + BIMMEDIATE_SIZE,                   // OP_DIVBI
    1 + HIMMEDIATE_SIZE,                   // OP_DIVHI
    1 + FIMMEDIATE_SIZE,                   // OP_DIVFI
    1,                                     // OP_GET
    1 + BIMMEDIATE_SIZE,                   // OP_GETBI
    1 + HIMMEDIATE_SIZE,                   // OP_GETHI
    1 + FIMMEDIATE_SIZE,                   // OP_GETFI
    1,                                     // OP_J
    1,                                     // OP_JCON
    1 + FIMMEDIATE_SIZE,                   // OP_JCONI
    1 + FIMMEDIATE_SIZE,                   // OP_JI
    1,                                     // OP_JR
    1 + BIMMEDIATE_SIZE,                   // OP_JRBI
    1 + HIMMEDIATE_SIZE,                   // OP_JRHI
    1 + FIMMEDIATE_SIZE,                   // OP_JRFI
    1,                                     // OP_JRCON
    1 + BIMMEDIATE_SIZE,                   // OP_JRCONBI
    1 + HIMMEDIATE_SIZE,                   // OP_JRCONHI
    1 + FIMMEDIATE_SIZE,                   // OP_JRCONFI
    1,                                     // OP_MOD
    1 + BIMMEDIATE_SIZE,                   // OP_MODBI
    1 + HIMMEDIATE_SIZE,                   // OP_MODHI
    1 + FIMMEDIATE_SIZE,                   // OP_MODFI
    1,                                     // OP_MUL
    1,                                     // OP_MULF
    1 + BIMMEDIATE_SIZE,                   // OP_MULBI
    1 + HIMMEDIATE_SIZE,                   // OP_MULHI
    1 + FIMMEDIATE_SIZE,                   // OP_MULFI
    1,                                     // OP_NOT
    1,                                     // OP_OR
    1 + BIMMEDIATE_SIZE,                   // OP_ORBI
    1 + HIMMEDIATE_SIZE,                   // OP_ORHI
    1 + FIMMEDIATE_SIZE,                   // OP_ORFI
    1,                                     // OP_POP
    1 + BIMMEDIATE_SIZE,                   // OP_POPB
    1 + HIMMEDIATE_SIZE,                   // OP_POPH
    1 + FIMMEDIATE_SIZE,                   // OP_POPF
    1 + BIMMEDIATE_SIZE,                   // OP_PUSHB
    1 + HIMMEDIATE_SIZE,                   // OP_PUSHH
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHF
    1 + BIMMEDIATE_SIZE,                   // OP_PUSHNB
    1 + HIMMEDIATE_SIZE,                   // OP_PUSHNH
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHNF
    1,                                     // OP_SETADR
    1,                                     // OP_SETADRB
    1,                                     // OP_SUB
    1,                                     // OP_SUBF
    1 + BIMMEDIATE_SIZE,                   // OP_SUBBI
    1 + HIMMEDIATE_SIZE,                   // OP_SUBHI
    1 + FIMMEDIATE_SIZE,                   // OP_SUBFI
    1 + BIMMEDIATE_SIZE,                   // OP_SYSCALL
    1,                                     // OP_XOR
    1 + BIMMEDIATE_SIZE,                   // OP_XORBI
    1 + HIMMEDIATE_SIZE,                   // OP_XORHI
    1 + FIMMEDIATE_SIZE,                   // OP_XORFI
};

/*
=== STACK FUNCTIONS =======================================
*/

/**
 * \fn static void vm_set_stack_size_to(DVM *vm, duint size)
 * \brief Set the VM's stack size.
 *
 * \param vm The VM whose stack to set the size of.
 * \param size The size the stack should be set to, i.e. how many elements can
 * it contain?
 */
static void vm_set_stack_size_to(DVM *vm, duint size) {
    vm->stackSize = size;

    const size_t newAlloc = size * sizeof(dint);

    if (vm->basePtr == NULL) {
        vm->basePtr = d_malloc(newAlloc);
    } else {
        vm->basePtr = d_realloc(vm->basePtr, newAlloc);
    }
}

/**
 * \fn static void vm_increase_stack_size(DVM *vm)
 * \brief Increase the stack size of the VM by `VM_STACK_SIZE_SCALE_INC`.
 *
 * \param vm The VM of the stack to increase the size of.
 */
static void vm_increase_stack_size(DVM *vm) {
    duint newSize = (duint)(vm->stackSize * VM_STACK_SIZE_SCALE_INC);

    if (newSize > vm->stackSize) {
        vm_set_stack_size_to(vm, newSize);
    }
}

/**
 * \fn static void vm_decrease_stack_size(DVM *vm)
 * \brief Decrease the size of the stack by `VM_STACK_SIZE_SCALE_DEC`.
 * Note that the size cannot go lower than `VM_STACK_SIZE_MIN`.
 *
 * \param vm The VM of the stack to decrease the size of.
 */
static void vm_decrease_stack_size(DVM *vm) {
    duint newSize = (duint)(vm->stackSize * VM_STACK_SIZE_SCALE_DEC);

    if (newSize < VM_STACK_SIZE_MIN) {
        newSize = VM_STACK_SIZE_MIN;
    }

    if (newSize < vm->stackSize) {
        vm_set_stack_size_to(vm, newSize);
    }
}

/**
 * \def VM_GET_FRAME_PTR(vm, index)
 * \brief Get a pointer relative to the VM's frame pointer.
 */
#define VM_GET_FRAME_PTR(vm, index) ((vm)->framePtr + (index))

/**
 * \def VM_GET_STACK_PTR(vm, index)
 * \brief Get a pointer relative to the VM's stack pointer.
 */
#define VM_GET_STACK_PTR(vm, index) ((vm)->stackPtr + (index))

/**
 * \def VM_GET_FRAME(vm, index)
 * \brief Get the value of the stack at the index, relative to the frame
 * pointer.
 */
#define VM_GET_FRAME(vm, index) (*VM_GET_FRAME_PTR(vm, index))

/**
 * \def VM_GET_STACK(vm, index)
 * \brief Get the value of the stack at the index, relative to the stack
 * pointer.
 */
#define VM_GET_STACK(vm, index) (*VM_GET_STACK_PTR(vm, index))

/**
 * \def VM_GET_FRAME_FLOAT_PTR(vm, index)
 * \brief Get a float pointer relative to the VM's frame pointer.
 */
#define VM_GET_FRAME_FLOAT_PTR(vm, index) \
    ((dfloat *)VM_GET_FRAME_PTR(vm, index))

/**
 * \def VM_GET_STACK_FLOAT_PTR(vm, index)
 * \brief Get a float pointer relative to the VM's stack pointer.
 */
#define VM_GET_STACK_FLOAT_PTR(vm, index) \
    ((dfloat *)VM_GET_STACK_PTR(vm, index))

/**
 * \def VM_GET_FRAME_FLOAT(vm, index)
 * \brief Get the float value of the stack at the index, relative to the frame
 * pointer.
 */
#define VM_GET_FRAME_FLOAT(vm, index) (*VM_GET_STACK_FLOAT_PTR(vm, index))

/**
 * \def VM_GET_STACK_FLOAT(vm, index)
 * \brief Get the float value of the stack at the index, relative to the stack
 * pointer.
 */
#define VM_GET_STACK_FLOAT(vm, index) (*VM_GET_STACK_FLOAT_PTR(vm, index))

/**
 * \fn size_t d_vm_frame(DVM *vm)
 * \brief Get the number of elements in the current stack frame.
 *
 * \return The number of elements in the stack frame.
 *
 * \param vm The VM whose stack to query.
 */
size_t d_vm_frame(DVM *vm) {
    if (vm->basePtr != NULL) {
        return 0;
    }

    return (size_t)(vm->stackPtr - vm->framePtr);
}

/**
 * \fn dint d_vm_get(DVM *vm, dint index)
 * \brief Get an integer from a value in the stack at a particular index.
 *
 * * If `index` is non-negative, it will index relative to the start of the
 * stack frame.
 * * If `index` is negative, it will index relative to the top of the stack.
 *
 * \return The integer value of the stack at the given index.
 *
 * \param vm The VM whose stack to retrieve from.
 * \param index The index of the stack.
 */
dint d_vm_get(DVM *vm, dint index) {
    if (index > 0) {
        // Get the value relative to the frame pointer.
        return VM_GET_FRAME(vm, index);
    } else {
        // Get the value relative to the stack pointer.
        return VM_GET_STACK(vm, index);
    }
}

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
dfloat d_vm_get_float(DVM *vm, dint index) {
    dint value = d_vm_get(vm, index);

    return *((dfloat *)(&value));
}

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
void *d_vm_get_ptr(DVM *vm, dint index) {
    dint value = d_vm_get(vm, index);

    return (void *)value;
}

/**
 * \def VM_INSERT_LEN(vm, ptr, len, numMove)
 * \brief A helper macro for for inserting a length of items onto the stack,
 * without the checks.
 */
#define VM_INSERT_LEN(vm, ptr, len, numMove)                     \
    {                                                            \
        d_vm_pushn(vm, len);                                     \
        memmove((ptr) + (len), (ptr), (numMove) * sizeof(dint)); \
    }

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
void d_vm_insert(DVM *vm, dint index, dint value) {
    if (index == 0) {
        // If the index is 0, then we want to insert the value to the top of
        // the stack, and move whatever is already there up one place.
        d_vm_push(vm, 0);
        *VM_GET_STACK_PTR(vm, 0)  = VM_GET_STACK(vm, -1);
        *VM_GET_STACK_PTR(vm, -1) = value;
    } else {
        dint *insertPtr;

        if (index > 0) {
            // Get the position relative to the frame pointer.
            insertPtr = VM_GET_FRAME_PTR(vm, index);
        } else {
            // Get the position relative to the stack pointer.
            insertPtr = VM_GET_STACK_PTR(vm, index);
        }

        if (insertPtr >= vm->basePtr && insertPtr <= vm->stackPtr) {
            const size_t numElemsToMove = ((vm->stackPtr - insertPtr) + 1);
            VM_INSERT_LEN(vm, insertPtr, 1, numElemsToMove)
            *insertPtr = value;
        }
    }
}

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
void d_vm_insert_float(DVM *vm, dint index, dfloat value) {
    dint intValue = *((dint *)(&value));

    d_vm_insert(vm, index, intValue);
}

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
 * \param value The pointer to insert into the stack.
 */
void d_vm_insert_ptr(DVM *vm, dint index, void *ptr) {
    dint value = (dint)ptr;

    d_vm_insert(vm, index, value);
}

/**
 * \fn dint d_vm_pop(DVM *vm)
 * \brief Pop an integer from the top of the stack.
 *
 * \return The integer at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
dint d_vm_pop(DVM *vm) {
    dint value = d_vm_get(vm, 0);
    d_vm_popn(vm, 1);
    return value;
}

/**
 * \fn void d_vm_popn(DVM *vm, size_t n)
 * \brief Pop `n` elements from the stack.
 *
 * \param vm The VM whose stack to pop from.
 * \param n The number of elements to pop.
 */
void d_vm_popn(DVM *vm, size_t n) {
    if (n > 0) {
        const size_t maxN = d_vm_top(vm);

        if (n > maxN) {
            n = maxN;
        }

        vm->stackPtr = vm->stackPtr - n;

        if (maxN - n < (size_t)(vm->stackSize * VM_STACK_SIZE_SCALE_DEC)) {
            vm_decrease_stack_size(vm);
        }
    }
}

/**
 * \fn dfloat d_vm_pop_float(DVM *vm)
 * \brief Pop a float from the top of the stack.
 *
 * \return The float at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
dfloat d_vm_pop_float(DVM *vm) {
    dfloat value = d_vm_get_float(vm, 0);
    d_vm_popn(vm, 1);
    return value;
}

/**
 * \fn void *d_vm_pop_ptr(DVM *vm)
 * \brief Pop a pointer from the top of the stack.
 *
 * \return The pointer at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
void *d_vm_pop_ptr(DVM *vm) {
    void *ptr = d_vm_get_ptr(vm, 0);
    d_vm_popn(vm, 1);
    return ptr;
}

/**
 * \fn void d_vm_push(DVM *vm, dint value)
 * \brief Push an integer value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
void d_vm_push(DVM *vm, dint value) {
    if (d_vm_top(vm) >= vm->stackSize) {
        vm_increase_stack_size(vm);
    }

    *(++vm->stackPtr) = value;
}

/**
 * \fn void d_vm_pushn(DVM *vm, size_t n)
 * \brief Push `0` onto the stack `n` times.
 *
 * \param vm The VM whose stack to push onto.
 * \param n The number of items to push onto the stack.
 */
void d_vm_pushn(DVM *vm, size_t n) {
    if (n > 0) {
        dint *newStackPtr    = vm->stackPtr + n;
        const size_t newSize = (newStackPtr - vm->basePtr) + 1;

        if (newSize >= vm->stackSize) {
            // Make space for the new elements if needed, plus a bit extra.
            vm_set_stack_size_to(vm,
                                 (duint)(newSize * VM_STACK_SIZE_SCALE_INC));
        }

        memset(vm->stackPtr + 1, 0, n * sizeof(dint));

        vm->stackPtr = newStackPtr;
    }
}

/**
 * \fn void d_vm_push_float(DVM *vm, dfloat value)
 * \brief Push a float value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
void d_vm_push_float(DVM *vm, dfloat value) {
    dint intValue = *((dint *)(&value));

    d_vm_push(vm, intValue);
}

/**
 * \fn void d_vm_push_ptr(DVM *vm, void *ptr)
 * \brief Push a pointer onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param ptr The pointer to push onto the stack.
 */
void d_vm_push_ptr(DVM *vm, void *ptr) {
    dint value = (dint)ptr;

    d_vm_push(vm, value);
}

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
void d_vm_remove(DVM *vm, dint index) {
    d_vm_remove_len(vm, index, 1);
}

/**
 * \def VM_REMOVE_LEN(vm, ptr, len)
 * \brief A helper macro for removing a length of items in the stack, without
 * any of the checks.
 */
#define VM_REMOVE_LEN(vm, ptr, len, numMove)                     \
    {                                                            \
        memmove((ptr), (ptr) + (len), (numMove) * sizeof(dint)); \
        d_vm_popn(vm, len);                                      \
    }

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
void d_vm_remove_len(DVM *vm, dint index, size_t len) {
    if (len > 0) {
        if (index == 0) {
            d_vm_popn(vm, 1);
        } else {
            dint *removePtr;

            if (index > 0) {
                // Get the position relative to the frame pointer.
                removePtr = VM_GET_FRAME_PTR(vm, index);
            } else {
                // Get the position relative to the stack pointer.
                removePtr = VM_GET_STACK_PTR(vm, index);
            }

            if (removePtr >= vm->basePtr && removePtr <= vm->stackPtr) {
                const size_t max = (vm->stackPtr - removePtr) + 1;
                if (len >= max) {
                    len = max;
                }

                const size_t numElemsToMove = max - len;
                VM_REMOVE_LEN(vm, removePtr, len, numElemsToMove)
            }
        }
    }
}

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
void d_vm_set(DVM *vm, dint index, dint value) {
    if (index > 0) {
        // Set the value relative to the frame pointer.
        *VM_GET_FRAME_PTR(vm, index) = value;
    } else {
        // Set the value relative to the stack pointer.
        *VM_GET_STACK_PTR(vm, index) = value;
    }
}

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
void d_vm_set_float(DVM *vm, dint index, dfloat value) {
    dint intValue = *((dint *)(&value));

    d_vm_set(vm, index, intValue);
}

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
void d_vm_set_ptr(DVM *vm, dint index, void *ptr) {
    dint value = (dint)ptr;

    d_vm_set(vm, index, value);
}

/**
 * \fn size_t d_vm_top(DVM *vm)
 * \brief Get the number of elements in the stack.
 *
 * \return The number of elements in the stack.
 *
 * \param vm The VM whose stack to query.
 */
size_t d_vm_top(DVM *vm) {
    if (vm->basePtr == NULL) {
        return 0;
    }

    return (size_t)((vm->stackPtr - vm->basePtr) + 1);
}

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
const unsigned char d_vm_ins_size(DIns opcode) {
    if (opcode >= NUM_OPCODES)
        return 0;

    return VM_INS_SIZE[opcode];
}

/**
 * \fn DVM d_vm_create()
 * \brief Create a Decision VM in its starting state, with malloc'd elements.
 *
 * \return A Decision VM in its starting state.
 */
DVM d_vm_create() {
    DVM vm;

    // In order to set the VM to its starting state, we just need to set the
    // base stack pointer to NULL, and d_vm_reset will do the rest for us.
    // Setting the pointer to NULL will force d_vm_reset to malloc a new stack.
    vm.basePtr = NULL;

    d_vm_reset(&vm);

    return vm;
}

/**
 * \fn void d_vm_reset(DVM *vm)
 * \brief Reset a Decision VM to its starting state.
 *
 * \param vm A Decision VM to set to its starting state.
 */
void d_vm_reset(DVM *vm) {
    vm->pc      = 0;
    vm->_inc_pc = 0;

    vm_set_stack_size_to(vm, VM_STACK_SIZE_MIN);

    dint *ptr    = vm->basePtr - 1;
    vm->stackPtr = ptr;
    vm->framePtr = ptr;

    vm->halted       = true;
    vm->runtimeError = false;
}

/**
 * \fn void d_vm_free(DVM *vm)
 * \brief Free the malloc'd elements of a Decision VM. Note that this makes the
 * VM unusable unless you call `d_vm_reset` on it.
 *
 * \param vm The Decision VM to free.
 */
void d_vm_free(DVM *vm) {
    if (vm->basePtr != NULL) {
        free(vm->basePtr);
        vm->basePtr = NULL;
    }
}

/**
 * \fn void d_vm_runtime_error(DVM *vm, const char *error)
 * \brief Print a runtime error to `stdout`, and halt the VM.
 *
 * Unlike compiler errors, these errors aren't stored anywhere.
 *
 * \param vm The VM that came across the error.
 * \param error The error message to display.
 */
void d_vm_runtime_error(DVM *vm, const char *error) {
    printf("Fatal: (%p) %s\n", vm->pc, error);
    vm->halted       = true;
    vm->runtimeError = true;
}

/*
    NOTE: The following helper macros are used extensively in
    d_vm_parse_ins_at_pc. Since this function is going to be called a LOT,
    these helper macros need to be as efficient as possible. This means things
    like using as few functions as possible, making as less calculations as
    possible, etc.
*/

/**
 * \def GET_IMMEDIATE(tm, offset)
 * \brief A helper macro for getting a generic immediate value.
 */
#define GET_IMMEDIATE(t, offset) (*((t *)(vm->pc + (offset))))

/**
 * \def GET_BIMMEDIATE(offset)
 * \brief A helper macro for getting a byte immediate.
 */
#define GET_BIMMEDIATE(offset) GET_IMMEDIATE(bimmediate_t, offset)

/**
 * \def GET_HIMMEDIATE(offset)
 * \brief A helper macro for getting a half immediate.
 */
#define GET_HIMMEDIATE(offset) GET_IMMEDIATE(himmediate_t, offset)

/**
 * \def GET_FIMMEDIATE(offset)
 * \brief A helper macro for getting a full immediate.
 */
#define GET_FIMMEDIATE(offset) GET_IMMEDIATE(fimmediate_t, offset)

/**
 * \def OP_1_1_I(sym, fun)
 * \brief A helper macro for opcodes with 1 input and 1 output involving
 * integers, and an immediate.
 */
#define OP_1_1_I(sym, fun)                   \
    {                                        \
        dint *top = VM_GET_STACK_PTR(vm, 0); \
        *top      = *top sym fun(1);         \
    }

/**
 * \def OP_2_1(sym)
 * \brief A helper macro for opcodes with 2 inputs and 1 output involving
 * integers.
 */
#define OP_2_1(sym)                                                  \
    {                                                                \
        dint value = (VM_GET_STACK(vm, 0) sym VM_GET_STACK(vm, -1)); \
        *VM_GET_STACK_PTR(vm, -1) = value;                           \
        d_vm_popn(vm, 1);                                            \
    }

/**
 * \def OP_2_1_F(sym)
 * \brief A helper macro for opcodes with 2 inputs and 1 output involving
 * floats.
 */
#define OP_2_1_F(sym)                                                   \
    {                                                                   \
        dfloat value =                                                  \
            (VM_GET_STACK_FLOAT(vm, 0) sym VM_GET_STACK_FLOAT(vm, -1)); \
        *VM_GET_STACK_FLOAT_PTR(vm, -1) = value;                        \
        d_vm_popn(vm, 1);                                               \
    }

/**
 * \def OP_2_1_C(sym)
 * \brief A helper macro for opcodes with 2 inputs and 1 output involving
 * comparisons with floats.
 */
#define OP_2_1_C(sym)                                                   \
    {                                                                   \
        dint value =                                                    \
            (VM_GET_STACK_FLOAT(vm, 0) sym VM_GET_STACK_FLOAT(vm, -1)); \
        *VM_GET_STACK_PTR(vm, -1) = value;                              \
        d_vm_popn(vm, 1);                                               \
    }

/**
 * \def CALL_GENERIC(sym, newPC, offset)
 * \brief A generic helper macro for call opcodes.
 */
#define CALL_GENERIC(sym, newPC, offset)                              \
    {                                                                 \
        vm->pc sym newPC;                                             \
        const uint8_t numArguments = (uint8_t)GET_BIMMEDIATE(offset); \
        dint *insertPtr            = vm->stackPtr - numArguments - 1; \
        VM_INSERT_LEN(vm, insertPtr, 2, numArguments)                 \
        *insertPtr = (dint)vm->framePtr;                              \
        insertPtr++;                                                  \
        *insertPtr   = (dint)vm->pc;                                  \
        vm->framePtr = insertPtr;                                     \
        vm->_inc_pc  = 0;                                             \
    }

/**
 * \def CALL_1_0(sym)
 * \brief A helper macro for call opcodes with 1 input and 0 outputs.
 *
 * **NOTE:** Here we just decrement the stack pointer instead of using
 * `d_vm_popn`, since `VM_INSERT_LEN` pushed 2 times, it makes an overall
 * difference of 1 element being pushed on, so the stack should not have to be
 * decreased in size. This is simply an optimisation.
 */
#define CALL_1_0(sym)                              \
    {                                              \
        CALL_GENERIC(sym, VM_GET_STACK(vm, 0), 1); \
        vm->stackPtr--;                            \
    }

/**
 * \def CALL_0_0_BI(sym)
 * \brief A helper macro for call opcodes with 0 inputs and 0 outputs,
 * involving a byte immediate.
 */
#define CALL_0_0_BI(sym) \
    CALL_GENERIC(sym, GET_BIMMEDIATE(1), 1 + BIMMEDIATE_SIZE)

/**
 * \def CALL_0_0_HI(sym)
 * \brief A helper macro for call opcodes with 0 inputs and 0 outputs,
 * involving a half immediate.
 */
#define CALL_0_0_HI(sym) \
    CALL_GENERIC(sym, GET_HIMMEDIATE(1), 1 + HIMMEDIATE_SIZE)

/**
 * \def CALL_0_0_FI(sym)
 * \brief A helper macro for call opcodes with 0 inputs and 0 outputs,
 * involving a full immediate.
 */
#define CALL_0_0_FI(sym) \
    CALL_GENERIC(sym, GET_FIMMEDIATE(1), 1 + FIMMEDIATE_SIZE)

/**
 * \def J_0_0_I(sym)
 * \brief A helper macro for jump opcodes with 0 inputs and 0 outputs, and an
 * immediate.
 */
#define J_0_0_I(sym, fun)  \
    {                      \
        vm->pc sym fun(1); \
        vm->_inc_pc = 0;   \
    }

/**
 * \def J_1_0(sym)
 * \brief A helper macro for jump opcodes with 1 input and 0 outputs.
 */
#define J_1_0(sym)                      \
    {                                   \
        vm->pc sym VM_GET_STACK(vm, 0); \
        d_vm_popn(vm, 1);               \
        vm->_inc_pc = 0;                \
    }

/**
 * \def JCON_1_0_I(sym, fun)
 * \brief A helper macro for jump condition opcodes with 1 input and 0 outputs,
 * and an immediate.
 */
#define JCON_1_0_I(sym, fun)       \
    {                              \
        if (VM_GET_STACK(vm, 0)) { \
            vm->pc sym fun(1);     \
            vm->_inc_pc = 0;       \
        }                          \
        d_vm_popn(vm, 1);          \
    }

/**
 * \def JCON_2_0(sym)
 * \brief A helper macro for jump condition opcodes with 2 inputs and 0 outputs.
 */
#define JCON_2_0(sym)                        \
    {                                        \
        if (VM_GET_STACK(vm, 0)) {           \
            vm->pc sym VM_GET_STACK(vm, -1); \
            vm->_inc_pc = 0;                 \
        }                                    \
        d_vm_popn(vm, 2);                    \
    }

/**
 * \union _ptrToC
 * \brief A union of a pointer to a C function, and the pointer value.
 *
 * \typedef union _ptrToC PtrToC
 */
typedef union _ptrToC {
    DecisionCFunction func;
    intptr_t ptr;
} PtrToC;

/**
 * \fn void d_vm_parse_ins_at_pc(DVM *vm)
 * \brief Given a Decision VM, at it's current position in the program, parse
 * the instruction at that position.
 *
 * **NOTE:** This function will be run a lot during the course of execution.
 * If you are looking to optimise Decision, this is a good place to start.
 *
 * \param vm The VM to use to parse the instruction.
 */
void d_vm_parse_ins_at_pc(DVM *vm) {
    DIns opcode = *(vm->pc);

    // Set how many bytes we need to increment.
    // However, if we're about to perform a jump, we need to set this value to
    // 0.
    vm->_inc_pc = VM_INS_SIZE[opcode];

    // Do stuff depending on what the opcode is.
    switch (opcode) {
        case OP_RET:
        case OP_RETN:;
            // If the frame pointer is pointing to a location before the start
            // of the stack, then halt, as this is the last frame.
            if (vm->framePtr < vm->basePtr) {
                vm->halted = true;
            } else {
                uint8_t numReturnValues = 0;

                if (opcode == OP_RETN) {
                    numReturnValues = (uint8_t)GET_BIMMEDIATE(1);
                }

                // The frame pointer is now pointing at the saved program
                // counter, i.e. the return address.
                dint *ptr = vm->framePtr;
                vm->pc    = (char *)(*ptr);

                // The element before that is the saved frame pointer of the
                // previous stack frame.
                ptr--;
                vm->framePtr = (dint *)(*ptr);

                // Now this position is where the return values should go.
                const size_t lenRemove =
                    (vm->stackPtr - ptr) + 1 - numReturnValues;

                VM_REMOVE_LEN(vm, ptr, lenRemove, numReturnValues);
            }
            break;

        case OP_ADD:;
            OP_2_1(+)
            break;

        case OP_ADDF:;
            OP_2_1_F(+)
            break;

        case OP_ADDBI:;
            OP_1_1_I(+, GET_BIMMEDIATE)
            break;

        case OP_ADDHI:;
            OP_1_1_I(+, GET_HIMMEDIATE)
            break;

        case OP_ADDFI:;
            OP_1_1_I(+, GET_FIMMEDIATE)
            break;

        case OP_AND:;
            OP_2_1(&)
            break;

        case OP_ANDBI:;
            OP_1_1_I(&, GET_BIMMEDIATE)
            break;

        case OP_ANDHI:;
            OP_1_1_I(&, GET_HIMMEDIATE)
            break;

        case OP_ANDFI:;
            OP_1_1_I(&, GET_FIMMEDIATE)
            break;

        case OP_CALL:;
            CALL_1_0(= (char *))
            break;

        case OP_CALLC:;
            PtrToC funcPtr;

            funcPtr.ptr = VM_GET_STACK(vm, 0);
            d_vm_popn(vm, 1);

            // Call the C function.
            funcPtr.func(vm);
            break;

        case OP_CALLCI:;
            PtrToC funcPtrImmediate;

            funcPtrImmediate.ptr = GET_FIMMEDIATE(1);

            // Call the C function.
            funcPtrImmediate.func(vm);
            break;

        case OP_CALLI:;
            CALL_0_0_FI(= (char *))
            break;

        case OP_CALLR:;
            CALL_1_0(+=)
            break;

        case OP_CALLRB:;
            CALL_0_0_BI(+=)
            break;

        case OP_CALLRH:;
            CALL_0_0_HI(+=)
            break;

        case OP_CALLRF:;
            CALL_0_0_FI(+=)
            break;

        case OP_CEQ:;
            OP_2_1(==)
            break;

        case OP_CEQF:;
            OP_2_1_C(==)
            break;

        case OP_CLEQ:;
            OP_2_1(<=)
            break;

        case OP_CLEQF:;
            OP_2_1_C(<=)
            break;

        case OP_CLT:;
            OP_2_1(<)
            break;

        case OP_CLTF:;
            OP_2_1_C(<)
            break;

        case OP_CMEQ:;
            OP_2_1(>=)
            break;

        case OP_CMEQF:;
            OP_2_1_C(>=)
            break;

        case OP_CMT:;
            OP_2_1(>)
            break;

        case OP_CMTF:;
            OP_2_1_C(>)
            break;

        case OP_CVTF:;
            *VM_GET_STACK_FLOAT_PTR(vm, 0) = (dfloat)VM_GET_STACK(vm, 0);
            break;

        case OP_CVTI:;
            *VM_GET_STACK_PTR(vm, 0) = (dint)VM_GET_STACK_FLOAT(vm, 0);
            break;

        case OP_DEREF:;
            *VM_GET_STACK_PTR(vm, 0) = *((dint *)VM_GET_STACK(vm, 0));
            break;

        case OP_DEREFI:;
            *VM_GET_STACK_PTR(vm, 0) = *((dint *)GET_FIMMEDIATE(1));
            break;

        case OP_DEREFB:;
            *VM_GET_STACK_PTR(vm, 0) = *((uint8_t *)VM_GET_STACK(vm, 0));
            break;

        case OP_DEREFBI:;
            *VM_GET_STACK_PTR(vm, 0) = *((uint8_t *)GET_FIMMEDIATE(1));
            break;

        case OP_DIV:;
            OP_2_1(/)
            break;

        case OP_DIVF:;
            OP_2_1_F(/)
            break;

        case OP_DIVBI:;
            OP_1_1_I(/, GET_BIMMEDIATE)
            break;

        case OP_DIVHI:;
            OP_1_1_I(/, GET_HIMMEDIATE)
            break;

        case OP_DIVFI:;
            OP_1_1_I(/, GET_FIMMEDIATE)
            break;

        case OP_GET:;
            *VM_GET_STACK_PTR(vm, 0) = d_vm_get(vm, VM_GET_STACK(vm, 0));
            break;

        case OP_GETBI:;
            d_vm_push(vm, d_vm_get(vm, GET_BIMMEDIATE(1)));
            break;

        case OP_GETHI:;
            d_vm_push(vm, d_vm_get(vm, GET_HIMMEDIATE(1)));
            break;

        case OP_GETFI:;
            d_vm_push(vm, d_vm_get(vm, GET_FIMMEDIATE(1)));
            break;

        case OP_J:;
            J_1_0(= (char *))
            break;

        case OP_JCON:;
            JCON_2_0(= (char *))
            break;

        case OP_JCONI:;
            JCON_1_0_I(= (char *), GET_FIMMEDIATE)
            break;

        case OP_JI:;
            J_0_0_I(= (char *), GET_FIMMEDIATE)
            break;

        case OP_JR:;
            J_1_0(+=)
            break;

        case OP_JRBI:;
            J_0_0_I(+=, GET_BIMMEDIATE)
            break;

        case OP_JRHI:;
            J_0_0_I(+=, GET_HIMMEDIATE)
            break;

        case OP_JRFI:;
            J_0_0_I(+=, GET_FIMMEDIATE)
            break;

        case OP_JRCON:;
            JCON_2_0(+=)
            break;

        case OP_JRCONBI:;
            JCON_1_0_I(+=, GET_BIMMEDIATE)
            break;

        case OP_JRCONHI:;
            JCON_1_0_I(+=, GET_HIMMEDIATE)
            break;

        case OP_JRCONFI:;
            JCON_1_0_I(+=, GET_FIMMEDIATE)
            break;

        case OP_MOD:;
            OP_2_1(%)
            break;

        case OP_MODBI:;
            OP_1_1_I(%, GET_BIMMEDIATE)
            break;

        case OP_MODHI:;
            OP_1_1_I(%, GET_HIMMEDIATE)
            break;

        case OP_MODFI:;
            OP_1_1_I(%, GET_FIMMEDIATE)
            break;

        case OP_MUL:;
            OP_2_1(*)
            break;

        case OP_MULF:;
            OP_2_1_F(*)
            break;

        case OP_MULBI:;
            OP_1_1_I(*, GET_BIMMEDIATE)
            break;

        case OP_MULHI:;
            OP_1_1_I(*, GET_HIMMEDIATE)
            break;

        case OP_MULFI:;
            OP_1_1_I(*, GET_FIMMEDIATE)
            break;

        case OP_NOT:;
            *VM_GET_STACK_PTR(vm, 0) = !VM_GET_STACK(vm, 0);
            break;

        case OP_OR:;
            OP_2_1(|)
            break;

        case OP_ORBI:;
            OP_1_1_I(|, GET_BIMMEDIATE)
            break;

        case OP_ORHI:;
            OP_1_1_I(|, GET_HIMMEDIATE)
            break;

        case OP_ORFI:;
            OP_1_1_I(|, GET_FIMMEDIATE)
            break;

        case OP_POP:;
            d_vm_popn(vm, 1);
            break;

        case OP_POPB:;
            d_vm_popn(vm, GET_BIMMEDIATE(1));
            break;

        case OP_POPH:;
            d_vm_popn(vm, GET_HIMMEDIATE(1));
            break;

        case OP_POPF:;
            d_vm_popn(vm, GET_FIMMEDIATE(1));
            break;

        case OP_PUSHB:;
            d_vm_push(vm, GET_BIMMEDIATE(1));
            break;

        case OP_PUSHH:;
            d_vm_push(vm, GET_HIMMEDIATE(1));
            break;

        case OP_PUSHF:;
            d_vm_push(vm, GET_FIMMEDIATE(1));
            break;

        case OP_PUSHNB:;
            d_vm_pushn(vm, GET_BIMMEDIATE(1));
            break;

        case OP_PUSHNH:;
            d_vm_pushn(vm, GET_HIMMEDIATE(1));
            break;

        case OP_PUSHNF:;
            d_vm_pushn(vm, GET_FIMMEDIATE(1));
            break;

        case OP_SETADR:;
            *((dint *)VM_GET_STACK(vm, 0)) = VM_GET_STACK(vm, -1);
            d_vm_popn(vm, 2);
            break;

        case OP_SETADRB:;
            *((uint8_t *)VM_GET_STACK(vm, 0)) = (uint8_t)VM_GET_STACK(vm, -1);
            d_vm_popn(vm, 2);
            break;

        case OP_SUB:;
            OP_2_1(-)
            break;

        case OP_SUBF:;
            OP_2_1_F(-)
            break;

        case OP_SUBBI:;
            OP_1_1_I(-, GET_BIMMEDIATE)
            break;

        case OP_SUBHI:;
            OP_1_1_I(-, GET_HIMMEDIATE)
            break;

        case OP_SUBFI:;
            OP_1_1_I(-, GET_FIMMEDIATE)
            break;

        case OP_SYSCALL:;
            switch (GET_BIMMEDIATE(1)) {
                case SYS_PRINT:;
                    switch (VM_GET_STACK(vm, 0)) {
                        case 0: // Integer
                            printf("%" DINT_PRINTF_d, VM_GET_STACK(vm, -1));
                            break;
                        case 1: // Float
                            printf("%g", VM_GET_STACK_FLOAT(vm, -1));
                            break;
                        case 2: // String
                            printf("%s", (char *)VM_GET_STACK(vm, -1));
                            break;
                        case 3: // Boolean
                            printf("%s",
                                   VM_GET_STACK(vm, -1) ? "true" : "false");
                            break;
                    }

                    if (VM_GET_STACK(vm, -2)) {
                        printf("\n");
                    }

                    *VM_GET_STACK_PTR(vm, -2) = 0;
                    break;
            }
            d_vm_popn(vm, 2);
            break;

        case OP_XOR:;
            OP_2_1(^)
            break;

        case OP_XORBI:;
            OP_1_1_I(^, GET_BIMMEDIATE)
            break;

        case OP_XORHI:;
            OP_1_1_I(^, GET_HIMMEDIATE)
            break;

        case OP_XORFI:;
            OP_1_1_I(^, GET_FIMMEDIATE)
            break;

        default:
            ERROR_RUNTIME(vm, "unknown opcode %d", opcode);
            break;
    }
}

/**
 * \fn void d_vm_add_pc(DVM *vm, dint rel)
 * \brief Add to the program counter to go +rel bytes.
 *
 * **NOTE:** The VM will ALWAYS increment the PC after every instruction.
 *
 * \param vm The VM whose PC to add to.
 * \param rel How many bytes to go forward. *Can* be negative.
 */
void d_vm_add_pc(DVM *vm, dint rel) {
    vm->pc += rel;
}

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
void d_vm_inc_pc(DVM *vm) {
    vm->pc += vm->_inc_pc;
}

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
bool d_vm_run(DVM *vm, void *start) {
    vm->pc     = start;
    vm->halted = false;

    while (!vm->halted) {
        d_vm_parse_ins_at_pc(vm);
        d_vm_inc_pc(vm);
    }

    return !vm->runtimeError;
}

/**
 * \fn void d_vm_dump(DVM *vm)
 * \brief Dump the contents of a Decision VM to stdout for debugging.
 *
 * \param vm The VM to dump the contents of.
 */
void d_vm_dump(DVM *vm) {
    // Print off basic information.
    printf("pc     = %p (%d)\n", vm->pc, *(vm->pc));
    printf("halted = %d\n", vm->halted);
    printf("error  = %d\n", vm->runtimeError);

    // Print off the stack.
    printf("\nstack (size = %" DINT_PRINTF_u "):\n", vm->stackSize);

    dint *ptr = vm->stackPtr;

    while (ptr >= vm->basePtr) {
        dint offset       = ptr - vm->stackPtr;
        dint intValue     = *ptr;
        dfloat floatValue = *((dfloat *)ptr);

        printf("%d\t= %d\t|\t0x%x\t|\t%f\n", offset, intValue, intValue,
               floatValue);

        ptr--;
    }
}
