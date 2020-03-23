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
    1 + BIMMEDIATE_SIZE,                   // OP_ADDBI
    1 + HIMMEDIATE_SIZE,                   // OP_ADDHI
    1 + FIMMEDIATE_SIZE,                   // OP_ADDFI
    1,                                     // OP_AND
    1 + BIMMEDIATE_SIZE,                   // OP_ANDBI
    1 + HIMMEDIATE_SIZE,                   // OP_ANDHI
    1 + FIMMEDIATE_SIZE,                   // OP_ANDFI
    1 + BIMMEDIATE_SIZE,                   // OP_CALL
    1 + BIMMEDIATE_SIZE,                   // OP_CALLC
    1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLCI
    1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLI
    1 + BIMMEDIATE_SIZE,                   // OP_CALLR
    1 + BIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRB
    1 + HIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRH
    1 + FIMMEDIATE_SIZE + BIMMEDIATE_SIZE, // OP_CALLRF
    1,                                     // OP_CEQ
    1,                                     // OP_CLEQ
    1,                                     // OP_CLT
    1,                                     // OP_CMEQ
    1,                                     // OP_CMT
    1,                                     // OP_CVTF
    1,                                     // OP_CVTI
    1,                                     // OP_DEREF
    1 + FIMMEDIATE_SIZE,                   // OP_DEREFI
    1,                                     // OP_DIV
    1 + BIMMEDIATE_SIZE,                   // OP_DIVBI
    1 + HIMMEDIATE_SIZE,                   // OP_DIVHI
    1 + FIMMEDIATE_SIZE,                   // OP_DIVFI
    1,                                     // OP_GET
    1 + BIMMEDIATE_SIZE,                   // OP_GETBI
    1 + HIMMEDIATE_SIZE,                   // OP_GETHI
    1 + FIMMEDIATE_SIZE,                   // OP_GETFI
    1,                                     // OP_INV
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
    1,                                     // OP_MARK
    1,                                     // OP_MOD
    1 + BIMMEDIATE_SIZE,                   // OP_MODBI
    1 + HIMMEDIATE_SIZE,                   // OP_MODHI
    1 + FIMMEDIATE_SIZE,                   // OP_MODFI
    1,                                     // OP_MUL
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
    1 + BIMMEDIATE_SIZE,                   // OP_PUSHIB
    1 + HIMMEDIATE_SIZE,                   // OP_PUSHIH
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHIF
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHF
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHS
    1 + BIMMEDIATE_SIZE,                   // OP_PUSHB
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHP
    1 + BIMMEDIATE_SIZE,                   // OP_PUSHNB
    1 + HIMMEDIATE_SIZE,                   // OP_PUSHNH
    1 + FIMMEDIATE_SIZE,                   // OP_PUSHNF
    1,                                     // OP_SETADR
    1 + FIMMEDIATE_SIZE,                   // OP_SETADR
    1,                                     // OP_SUB
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
 * If the size of the stack will decrease due to this operation, then it will
 * free marked stack entries that are about to be poped.
 *
 * \param vm The VM whose stack to set the size of.
 * \param size The size the stack should be set to, i.e. how many elements can
 * it contain?
 */
static void vm_set_stack_size_to(DVM *vm, duint size) {
    // Before we set the stack size, we need to check if the size is about to
    // decrease, and if it is, we need to free marked entries so we don't lose
    // them forever!
    // NOTE: This assumes that entries that never got reached by the stack
    // pointer have been zero'd.
    if (size < vm->stackSize) {
        StackEntry *entry  = vm->basePtr + vm->stackSize;
        StackEntry *newTop = vm->basePtr + size;

        while (entry > newTop) {
            if (entry->free) {
                if (entry->data.p != NULL) {
                    free(entry->data.p);
                }
            }

            entry--;
        }
    }

    if (size == 0) {
        if (vm->basePtr != NULL) {
            free(vm->basePtr);
            vm->basePtr = NULL;
        }
    } else if (vm->basePtr == NULL) {
        vm->basePtr = d_calloc(size, sizeof(StackEntry));
    } else {
        // The problem with reallocing the base pointer is that it can move,
        // which would invalidate both the stack pointer and the frame
        // pointer!
        ptrdiff_t stackDiff = vm->stackPtr - vm->basePtr;
        ptrdiff_t frameDiff = vm->framePtr - vm->basePtr;

        vm->basePtr = d_realloc(vm->basePtr, size * sizeof(StackEntry));

        vm->stackPtr = vm->basePtr + stackDiff;
        vm->framePtr = vm->basePtr + frameDiff;

        // It's good practice to zero-out the new memory.
        const int numNewEntries = size - vm->stackSize;
        if (numNewEntries > 0) {
            memset(vm->basePtr + vm->stackSize, 0,
                   numNewEntries * sizeof(StackEntry));
        }
    }

    vm->stackSize = size;
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
 * \brief Get the entry of the stack at the index, relative to the frame
 * pointer.
 */
#define VM_GET_FRAME(vm, index) (*VM_GET_FRAME_PTR(vm, index))

/**
 * \def VM_GET_STACK(vm, index)
 * \brief Get the entry of the stack at the index, relative to the stack
 * pointer.
 */
#define VM_GET_STACK(vm, index) (*VM_GET_STACK_PTR(vm, index))

/**
 * \def VM_GET_PTR(vm, index)
 * \brief Get a pointer relative to the VM's frame or stack pointer, depending
 * on the index.
 */
#define VM_GET_PTR(vm, index) \
    (((index) > 0) ? VM_GET_FRAME_PTR(vm, index) : VM_GET_STACK_PTR(vm, index))

/**
 * \def VM_GET(vm, index)
 * \brief Get the entry of the stack at the index.
 */
#define VM_GET(vm, index) \
    (((index) > 0) ? VM_GET_FRAME(vm, index) : VM_GET_STACK(vm, index))

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
 * \fn dint d_vm_get_int(DVM *vm, dint index)
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
dint d_vm_get_int(DVM *vm, dint index) {
    return VM_GET(vm, index).data.i;
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
    return VM_GET(vm, index).data.f;
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
    return VM_GET(vm, index).data.p;
}

/**
 * \fn bool d_vm_get_bool(DVM *vm, dint index)
 * \brief Get a boolean from a value in the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \return The boolean value of the stack at the given index.
 *
 * \param vm The VM whose stack to retrieve from.
 * \param index The index of the stack.
 */
bool d_vm_get_bool(DVM *vm, dint index) {
    return VM_GET(vm, index).data.b;
}

/**
 * \def VM_INSERT_LEN(vm, baseIndex, len, numMove)
 * \brief A helper macro for for inserting a length of items onto the stack,
 * without the checks.
 */
#define VM_INSERT_LEN(vm, baseIndex, len, numMove)      \
    {                                                   \
        d_vm_pushn(vm, len);                            \
        dint *_insertPtr = (vm)->basePtr + (baseIndex); \
        memmove(_insertPtr + (len), _insertPtr,         \
                (numMove) * sizeof(StackEntry));        \
    }

/**
 * \fn static void insert_generic(DVM *vm, dint index, StackEntry entry)
 * \brief Insert a stack entry into the stack at a particular index.
 *
 * \param vm The VM whose stack to insert to.
 * \param index The index of the stack to insert to, i.e. `entry` will be at
 * this location when the function returns.
 * \param value The entry to insert into the stack.
 */
static void insert_generic(DVM *vm, dint index, StackEntry entry) {
    if (index == 0) {
        // If the index is 0, then we want to insert the entry to the top of
        // the stack, and move whatever is already there up one place.
        d_vm_push(vm, 0);
        *VM_GET_STACK_PTR(vm, 0)  = VM_GET_STACK(vm, -1);
        *VM_GET_STACK_PTR(vm, -1) = entry;
    } else {
        dint *insertPtr = VM_GET_PTR(vm, index);

        if (insertPtr >= vm->basePtr && insertPtr <= vm->stackPtr) {
            const size_t numElemsToMove = ((vm->stackPtr - insertPtr) + 1);
            ptrdiff_t baseIndex         = insertPtr - vm->basePtr;

            VM_INSERT_LEN(vm, baseIndex, 1, numElemsToMove)

            // Need to use this rather than reuse insertPtr, since inserting
            // may have increased the capacity of the stack, and thus could
            // have changed the base pointer.
            *(vm->basePtr + baseIndex) = entry;
        }
    }
}

/**
 * \fn void d_vm_insert_int(DVM *vm, dint index, dint value)
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
void d_vm_insert_int(DVM *vm, dint index, dint value) {
    StackEntry entry = {{0}, TYPE_INT, false};
    entry.data.i     = value;

    insert_generic(vm, index, entry);
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
    StackEntry entry = {{0}, TYPE_FLOAT, false};
    entry.data.f     = value;

    insert_generic(vm, index, entry);
}

/**
 * \fn void d_vm_insert_str(DVM *vm, dint index, char *str, bool mark)
 * \brief Insert a string into the stack at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to insert to.
 * \param index The index of the stack to insert to, i.e. `str` will be at this
 * location when the function returns.
 * \param str The string to insert into the stack.
 * \param mark Mark this string to be freed when it is poped from the stack by
 * the VM.
 */
void d_vm_insert_str(DVM *vm, dint index, char *str, bool mark) {
    StackEntry entry = {{0}, TYPE_STRING, false};
    entry.data.p     = str;
    entry.free       = mark;

    insert_generic(vm, index, entry);
}

/**
 * \fn void d_vm_insert_bool(DVM *vm, dint index, bool value)
 * \brief Insert a boolean into the stack at a particular index.
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
void d_vm_insert_bool(DVM *vm, dint index, bool value) {
    StackEntry entry = {{0}, TYPE_BOOL, false};
    entry.data.b     = value;

    insert_generic(vm, index, entry);
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
 * \fn dint d_vm_pop_int(DVM *vm)
 * \brief Pop an integer from the top of the stack.
 *
 * \return The integer at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
dint d_vm_pop_int(DVM *vm) {
    dint value = d_vm_get_int(vm, 0);
    d_vm_popn(vm, 1);
    return value;
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
 * \fn bool d_vm_pop_bool(DVM *vm)
 * \brief Pop a boolean from the top of the stack.
 *
 * \return The boolean at the top of the stack.
 *
 * \param vm The VM whose stack to pop from.
 */
bool d_vm_pop_bool(DVM *vm) {
    bool value = d_vm_get_bool(vm, 0);
    d_vm_popn(vm, 1);
    return value;
}

/**
 * \fn void d_vm_pushn(DVM *vm, size_t n)
 * \brief Push `0` onto the stack `n` times. The new entries will not have any
 * type associated with them.
 *
 * \param vm The VM whose stack to push onto.
 * \param n The number of items to push onto the stack.
 */
void d_vm_pushn(DVM *vm, size_t n) {
    if (n > 0) {
        const size_t newSize = (vm->stackPtr + n - vm->basePtr) + 1;

        if (newSize >= vm->stackSize) {
            // Make space for the new elements if needed, plus a bit extra.
            vm_set_stack_size_to(vm,
                                 (duint)(newSize * VM_STACK_SIZE_SCALE_INC));
        }

        memset(vm->stackPtr + 1, 0, n * sizeof(StackEntry));

        vm->stackPtr = vm->stackPtr + n;
    }
}

/**
 * \fn static void push_generic(DVM *vm, StackEntry entry)
 * \brief Push an entry onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
static void push_generic(DVM *vm, StackEntry entry) {
    if (d_vm_top(vm) >= vm->stackSize) {
        vm_increase_stack_size(vm);
    }

    *(++vm->stackPtr) = entry;
}

/**
 * \fn void d_vm_push_int(DVM *vm, dint value)
 * \brief Push an integer value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
void d_vm_push_int(DVM *vm, dint value) {
    StackEntry entry = {{0}, TYPE_INT, false};
    entry.data.i     = value;

    push_generic(vm, entry);
}

/**
 * \fn void d_vm_push_float(DVM *vm, dfloat value)
 * \brief Push a float value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
void d_vm_push_float(DVM *vm, dfloat value) {
    StackEntry entry = {{0}, TYPE_FLOAT, false};
    entry.data.f     = value;

    push_generic(vm, entry);
}

/**
 * \fn void d_vm_push_str(DVM *vm, char *str, bool mark)
 * \brief Push a string onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param ptr The string to push onto the stack.
 * \param mark Mark this string to be freed when it is poped from the stack by
 * the VM.
 */
void d_vm_push_str(DVM *vm, char *str, bool mark) {
    StackEntry entry = {{0}, TYPE_STRING, false};
    entry.data.p     = str;
    entry.free       = mark;

    push_generic(vm, entry);
}

/**
 * \fn void d_vm_push_bool(DVM *vm, bool value)
 * \brief Push a boolean value onto the stack.
 *
 * \param vm The VM whose stack to push onto.
 * \param value The value to push onto the stack.
 */
void d_vm_push_bool(DVM *vm, bool value) {
    StackEntry entry = {{0}, TYPE_BOOL, false};
    entry.data.b     = value;

    push_generic(vm, entry);
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
#define VM_REMOVE_LEN(vm, ptr, len, numMove)                           \
    {                                                                  \
        memmove((ptr), (ptr) + (len), (numMove) * sizeof(StackEntry)); \
        d_vm_popn(vm, len);                                            \
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
            dint *removePtr = VM_GET_PTR(vm, index);

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
 * \fn void d_vm_set_int(DVM *vm, dint index, dint value)
 * \brief Set the value of an element in the stack at a particular index to be
 * a given integer.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param value The integer value to set.
 */
void d_vm_set(DVM *vm, dint index, dint value) {
    StackEntry *entry = VM_GET_PTR(vm, index);
    entry->data.i     = value;
    entry->type       = TYPE_INT;
    entry->free       = false;
}

/**
 * \fn void d_vm_set_float(DVM *vm, dint index, dfloat value)
 * \brief Set the value of an element in the stack at a particular index to be
 * a given float.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param value The float value to set.
 */
void d_vm_set_float(DVM *vm, dint index, dfloat value) {
    StackEntry *entry = VM_GET_PTR(vm, index);
    entry->data.f     = value;
    entry->type       = TYPE_FLOAT;
    entry->free       = false;
}

/**
 * \fn void d_vm_set_str(DVM *vm, dint index, char *str, bool mark)
 * \brief Set the value of an element in the stack at a particular index to be
 * a given string.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param str The string value to set.
 * \param mark Mark this string to be freed when it is poped from the stack by
 * the VM.
 */
void d_vm_set_str(DVM *vm, dint index, char *str, bool mark) {
    StackEntry *entry = VM_GET_PTR(vm, index);
    entry->data.p     = str;
    entry->type       = TYPE_STRING;
    entry->free       = mark;
}

/**
 * \fn void d_vm_set_bool(DVM *vm, dint index, bool value)
 * \brief Set the value of an element in the stack at a particular index to be
 * a given boolean.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \param vm The VM whose stack to set the element of.
 * \param index The index of the stack.
 * \param value The boolean value to set.
 */
void d_vm_set_bool(DVM *vm, dint index, bool value) {
    StackEntry *entry = VM_GET_PTR(vm, index);
    entry->data.b     = value;
    entry->type       = TYPE_BOOL;
    entry->free       = false;
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

/**
 * \fn DType d_vm_type(DVM *vm, dint index)
 * \brief Get the type of the stack element at a particular index.
 *
 * * If `index` is positive, it will index relative to the start of the stack
 * frame.
 * * If `index` is non-positive, it will index relative to the top of the stack.
 *
 * \return The type of the stack element at the given index.
 *
 * \param vm The VM whose stack to get the element from.
 * \param index The index of the stack.
 */
DType d_vm_type(DVM *vm, dint index) {
    if (vm->basePtr == NULL) {
        return TYPE_NONE;
    }

    return VM_GET(vm, index).type;
}

/*
=== VM FUNCTIONS ==========================================
*/

/**
 * \fn unsigned char d_vm_ins_size(DIns opcode)
 * \brief Given an opcode, get the total size of the instruction involving that
 * opcode in bytes.
 *
 * \return The size of the instruction in bytes. 0 if the opcode doesn't exist.
 *
 * \param opcode The opcode to query.
 */
unsigned char d_vm_ins_size(DIns opcode) {
    if (opcode >= NUM_OPCODES)
        return 0;

    return (const unsigned char)VM_INS_SIZE[opcode];
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

    vm->stackSize = 0;
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
    // This should free the stack for us.
    vm_set_stack_size_to(vm, 0);
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

    StackEntry *entry0 = vm->stackPtr;
    StackEntry *entry1 = vm->stackPtr - 1;

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
                StackEntry *ptr = vm->framePtr;
                if (ptr->type != TYPE_PTR) {
                    d_vm_runtime_error(vm, "Return address is not a pointer");
                    break;
                }
                vm->pc = ptr->data.p;

                // The element before that is the saved frame pointer
                // difference of the last stack frame.
                ptr--;
                if (ptr->type != TYPE_INT) {
                    d_vm_runtime_error(vm,
                                       "Saved frame offset is not an integer");
                    break;
                }
                vm->framePtr = vm->basePtr + ptr->data.i;

                // Now this position is where the return values should go.
                const size_t lenRemove =
                    (vm->stackPtr - ptr) + 1 - numReturnValues;

                VM_REMOVE_LEN(vm, ptr, lenRemove, numReturnValues);

                vm->_inc_pc = 0;
            }
            break;

        case OP_ADD:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.f += entry0->data.f;
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.f = entry0->data.f + entry1->data.i;
                    entry1->type   = TYPE_FLOAT;
                } else {
                    ERROR_RUNTIME(vm, "Cannot ADD %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.f += entry0->data.i;
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.i += entry0->data.i;
                } else {
                    ERROR_RUNTIME(vm, "Cannot ADD %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot ADD %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            d_vm_popn(vm, 1);
            break;

        case OP_ADDBI:;
            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f += GET_BIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.i += GET_BIMMEDIATE(1);
            } else {
                ERROR_RUNTIME(vm, "Cannot ADDBI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_ADDHI:;
            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f += GET_HIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.i += GET_HIMMEDIATE(1);
            } else {
                ERROR_RUNTIME(vm, "Cannot ADDHI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_ADDFI:;
            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f += GET_FIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.i += GET_FIMMEDIATE(1);
            } else {
                ERROR_RUNTIME(vm, "Cannot ADDFI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_AND:;
            if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_INT) {
                    entry1->data.i &= entry0->data.i;
                } else {
                    ERROR_RUNTIME(vm, "Cannot AND %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_BOOL) {
                if (entry1->type == TYPE_BOOL) {
                    entry1->data.b &= entry0->data.b;
                } else {
                    ERROR_RUNTIME(vm, "Cannot AND %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot AND %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            d_vm_popn(vm, 1);
            break;

        case OP_ANDBI:;
            if (entry0->type == TYPE_INT) {
                entry0->data.i &= GET_BIMMEDIATE(1);
            } else if (entry0->type == TYPE_BOOL) {
                entry0->data.b &= !!(GET_BIMMEDIATE(1));
            } else {
                ERROR_RUNTIME(vm, "Cannot ANDBI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_ANDHI:;
            if (entry0->type == TYPE_INT) {
                entry0->data.i &= GET_HIMMEDIATE(1);
            } else if (entry0->type == TYPE_BOOL) {
                entry0->data.b &= !!(GET_HIMMEDIATE(1));
            } else {
                ERROR_RUNTIME(vm, "Cannot ANDHI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_ANDFI:;
            if (entry0->type == TYPE_INT) {
                entry0->data.i &= GET_FIMMEDIATE(1);
            } else if (entry0->type == TYPE_BOOL) {
                entry0->data.b &= !!(GET_FIMMEDIATE(1));
            } else {
                ERROR_RUNTIME(vm, "Cannot ANDFI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_CALL:
        case OP_CALLI:
        case OP_CALLR:
        case OP_CALLRB:
        case OP_CALLRH:
        case OP_CALLRF:;
            // Get the number of arguments.
            size_t offset = 1;

            if (opcode == OP_CALLRB) {
                offset += BIMMEDIATE_SIZE;
            } else if (opcode == OP_CALLRH) {
                offset += HIMMEDIATE_SIZE;
            } else if (opcode == OP_CALLRF) {
                offset += FIMMEDIATE_SIZE;
            }

            const uint8_t numArguments = (uint8_t)GET_BIMMEDIATE(offset);

            // Get the address that the PC should be set to when the call
            // returns.
            char *retAdr = vm->pc + VM_INS_SIZE[(unsigned char)*(vm->pc)];

            // Manipulate the PC.
            if (opcode == OP_CALL) {
                if (entry0->type == TYPE_PTR) {
                    vm->pc = entry0->data.p;

                    // The stack is about to get inserted to, so don't bother
                    // using d_vm_popn.
                    vm->stackPtr--;
                } else {
                    d_vm_runtime_error(vm, "CALL argument is not a pointer");
                    break;
                }
            } else if (opcode == OP_CALLI) {
                vm->pc = GET_FIMMEDIATE(1);
            } else if (opcode == OP_CALLR) {
                if (entry0->type == TYPE_INT) {
                    vm->pc += entry0->data.i;

                    // The stack is about to get inserted to, so don't bother
                    // using d_vm_popn.
                    vm->stackPtr--;
                } else {
                    d_vm_runtime_error(vm, "CALLR argument is not an integer");
                    break;
                }
            } else if (opcode == OP_CALLRB) {
                vm->pc += GET_BIMMEDIATE(1);
            } else if (opcode == OP_CALLRH) {
                vm->pc += GET_HIMMEDIATE(1);
            } else if (opcode == OP_CALLRF) {
                vm->pc += GET_FIMMEDIATE(1);
            }

            StackEntry *insertPtr = vm->stackPtr - numArguments + 1;
            ptrdiff_t baseIndex   = insertPtr - vm->basePtr;

            // Insert the frame pointer offset and the return address.
            VM_INSERT_LEN(vm, baseIndex, 2, numArguments);
            insertPtr = vm->basePtr + baseIndex;

            insertPtr->data.i = (dint)(vm->framePtr - vm->basePtr);
            insertPtr->type   = TYPE_INT;
            insertPtr->free   = false;

            insertPtr++;
            insertPtr->data.p = retAdr;
            insertPtr->type   = TYPE_PTR;
            insertPtr->free   = false;

            vm->framePtr = insertPtr;
            vm->_inc_pc  = 0;
            break;

        case OP_CALLC:
        case OP_CALLCI:;
            CFunction *cFunc;
            uint8_t numArgs;

            if (opcode == OP_CALLC) {
                cFunc = (CFunction *)entry0->data.p;
                d_vm_popn(vm, 1);

                numArgs = (uint8_t)GET_BIMMEDIATE(1);
            } else {
                cFunc   = (CFunction *)GET_FIMMEDIATE(1);
                numArgs = (uint8_t)GET_BIMMEDIATE(1 + FIMMEDIATE_SIZE);
            }

            // Save the current frame pointer.
            dint *savedFramePtr = vm->framePtr;

            // Set the frame pointer such that it is one below the first
            // argument.
            vm->framePtr = vm->stackPtr - numArgs;

            // Call the C function.
            cFunc->function(vm);

            // Restore the original frame pointer.
            vm->framePtr = savedFramePtr;
            break;

        case OP_CEQ:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = (entry0->data.f == entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.f == (dfloat)entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = ((dfloat)entry0->data.i == entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.i == entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
                // Booleans should only be comparable via equals!
            } else if (entry0->type == TYPE_BOOL) {
                if (entry1->type == TYPE_BOOL) {
                    entry1->data.b = (entry0->data.b == entry0->data.b);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot CEQ %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            entry1->type = TYPE_BOOL;
            d_vm_popn(vm, 1);
            break;

        case OP_CLEQ:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = (entry0->data.f <= entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.f <= (dfloat)entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CLEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = ((dfloat)entry0->data.i <= entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.i <= entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CLEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot CLEQ %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            entry1->type = TYPE_BOOL;
            d_vm_popn(vm, 1);
            break;

        case OP_CLT:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = (entry0->data.f < entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.f < (dfloat)entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CLT %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = ((dfloat)entry0->data.i < entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.i < entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CLT %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot CLT %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            entry1->type = TYPE_BOOL;
            d_vm_popn(vm, 1);
            break;

        case OP_CMEQ:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = (entry0->data.f >= entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.f >= (dfloat)entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CMEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = ((dfloat)entry0->data.i >= entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.i >= entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CMEQ %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot CMEQ %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            entry1->type = TYPE_BOOL;
            d_vm_popn(vm, 1);
            break;

        case OP_CMT:;
            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = (entry0->data.f > entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.f > (dfloat)entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CMT %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.b = ((dfloat)entry0->data.i > entry0->data.f);
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.b = (entry0->data.i > entry0->data.i);
                } else {
                    ERROR_RUNTIME(vm, "Cannot CMT %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot CMT %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            entry1->type = TYPE_BOOL;
            d_vm_popn(vm, 1);
            break;

        case OP_CVTF:;
            if (entry0->type == TYPE_INT) {
                entry0->data.f = (dfloat)entry0->data.i;
                entry0->type   = TYPE_FLOAT;
            } else {
                d_vm_runtime_error(vm, "CVTF argument is not an integer");
                break;
            }
            break;

        case OP_CVTI:;
            if (entry0->type == TYPE_FLOAT) {
                entry0->data.i = (dint)entry0->data.f;
                entry0->type   = TYPE_INT;
            } else {
                d_vm_runtime_error(vm, "CVTI argument is not a float");
                break;
            }
            break;

        case OP_DEREF:;
            if (entry0->type == TYPE_PTR) {
                *entry0 = *(StackEntry *)entry0->data.p;
            } else {
                d_vm_runtime_error(vm, "DEREF argument is not a pointer");
                break;
            }
            break;

        case OP_DEREFI:;
            d_vm_pushn(vm, 1);
            *(vm->stackPtr) = *(StackEntry *)GET_FIMMEDIATE(1);
            break;

        case OP_DIV:;
            if ((entry1->type == TYPE_FLOAT && entry1->data.f == 0.0) ||
                (entry1->type == TYPE_INT && entry1->data.i == 0)) {
                d_vm_runtime_error(vm, "Division by 0");
                break;
            }

            if (entry0->type == TYPE_FLOAT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.f = entry0->data.f / entry1->data.f;
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.f = entry0->data.f / entry1->data.i;
                    entry1->type   = TYPE_FLOAT;
                } else {
                    ERROR_RUNTIME(vm, "Cannot DIV %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else if (entry0->type == TYPE_INT) {
                if (entry1->type == TYPE_FLOAT) {
                    entry1->data.f = entry0->data.i / entry1->data.f;
                } else if (entry1->type == TYPE_INT) {
                    entry1->data.f = (dfloat)entry0->data.i / entry1->data.i;
                    entry1->type   = TYPE_FLOAT;
                } else {
                    ERROR_RUNTIME(vm, "Cannot DIV %s and %s",
                                  d_type_name(entry0->type),
                                  d_type_name(entry1->type));
                    break;
                }
            } else {
                ERROR_RUNTIME(vm, "Cannot DIV %s and %s",
                              d_type_name(entry0->type),
                              d_type_name(entry1->type));
                break;
            }
            d_vm_popn(vm, 1);
            break;

        case OP_DIVBI:;
            if (GET_BIMMEDIATE(1) == 0) {
                d_vm_runtime_error(vm, "Division by 0");
                break;
            }

            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f /= GET_BIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.f = (dfloat)entry0->data.i / GET_BIMMEDIATE(1);
                entry0->type   = TYPE_FLOAT;
            } else {
                ERROR_RUNTIME(vm, "Cannot DIVBI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_DIVHI:;
            if (GET_HIMMEDIATE(1) == 0) {
                d_vm_runtime_error(vm, "Division by 0");
                break;
            }

            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f /= GET_HIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.f = (dfloat)entry0->data.i / GET_HIMMEDIATE(1);
                entry0->type   = TYPE_FLOAT;
            } else {
                ERROR_RUNTIME(vm, "Cannot DIVHI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_DIVFI:;
            if (GET_FIMMEDIATE(1) == 0) {
                d_vm_runtime_error(vm, "Division by 0");
                break;
            }

            if (entry0->type == TYPE_FLOAT) {
                entry0->data.f /= GET_FIMMEDIATE(1);
            } else if (entry0->type == TYPE_INT) {
                entry0->data.f = (dfloat)entry0->data.i / GET_FIMMEDIATE(1);
                entry0->type   = TYPE_FLOAT;
            } else {
                ERROR_RUNTIME(vm, "Cannot DIVFI %s", d_type_name(entry0->type));
                break;
            }
            break;

        case OP_GET:;
            if (entry0->type == TYPE_INT) {
                *entry0 = VM_GET(vm, entry0->data.i);
            } else {
                d_vm_runtime_error(vm, "GET argument is not an integer");
                break;
            }
            break;

        case OP_GETBI:;
            push_generic(vm, VM_GET(vm, GET_BIMMEDIATE(1)));
            break;

        case OP_GETHI:;
            push_generic(vm, VM_GET(vm, GET_HIMMEDIATE(1)));
            break;

        case OP_GETFI:;
            push_generic(vm, VM_GET(vm, GET_FIMMEDIATE(1)));
            break;

        case OP_INV:;
            if (entry0->type == TYPE_INT) {
                entry0->data.i = ~entry0->data.i;
            } else {
                d_vm_runtime_error(vm, "INV argument is not an integer");
                break;
            }
            break;

        case OP_J:;
            if (entry0->type == TYPE_PTR) {
                vm->pc = entry0->data.p;
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "J argument is not a pointer");
                break;
            }
            break;

        case OP_JCON:;
            if (entry0->type == TYPE_BOOL) {
                if (entry1->type == TYPE_PTR) {
                    if (entry0->data.b) {
                        vm->pc = entry1->data.p;
                    }
                    d_vm_popn(vm, 2);
                } else {
                    d_vm_runtime_error(vm, "JCON argument 2 is not a pointer");
                    break;
                }
            } else {
                d_vm_runtime_error(vm, "JCON argument 1 is not a boolean");
                break;
            }
            break;

        case OP_JCONI:;
            if (entry0->type == TYPE_BOOL) {
                if (entry0->data.b) {
                    vm->pc = (char *)GET_FIMMEDIATE(1);
                }
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "JCONI argument is not a boolean");
                break;
            }
            break;

        case OP_JI:;
            vm->pc = (char *)GET_FIMMEDIATE(1);
            break;

        case OP_JR:;
            if (entry0->type == TYPE_INT) {
                vm->pc += entry0->data.i;
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "JR argument is not an integer");
                break;
            }
            break;

        case OP_JRBI:;
            vm->pc += GET_BIMMEDIATE(1);
            break;

        case OP_JRHI:;
            vm->pc += GET_HIMMEDIATE(1);
            break;

        case OP_JRFI:;
            vm->pc += GET_FIMMEDIATE(1);
            break;

        case OP_JRCON:;
            if (entry0->type == TYPE_BOOL) {
                if (entry1->type == TYPE_INT) {
                    if (entry0->data.b) {
                        vm->pc += entry1->data.i;
                    }
                    d_vm_popn(vm, 2);
                } else {
                    d_vm_runtime_error(vm,
                                       "JRCON argument 2 is not an integer");
                    break;
                }
            } else {
                d_vm_runtime_error(vm, "JRCON argument 1 is not a boolean");
                break;
            }
            break;

        case OP_JRCONBI:;
            if (entry0->type == TYPE_BOOL) {
                if (entry0->data.b) {
                    vm->pc += GET_BIMMEDIATE(1);
                }
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "JRCONBI argument is not a boolean");
                break;
            }
            break;

        case OP_JRCONHI:;
            if (entry0->type == TYPE_BOOL) {
                if (entry0->data.b) {
                    vm->pc += GET_HIMMEDIATE(1);
                }
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "JRCONHI argument is not a boolean");
                break;
            }
            break;

        case OP_JRCONFI:;
            if (entry0->type == TYPE_BOOL) {
                if (entry0->data.b) {
                    vm->pc += GET_FIMMEDIATE(1);
                }
                d_vm_popn(vm, 1);
            } else {
                d_vm_runtime_error(vm, "JRCONFI argument is not a boolean");
                break;
            }
            break;
        
        case OP_MARK:;
            entry0->free = true;
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
            dint result;
            switch (GET_BIMMEDIATE(1)) {
                case SYS_PRINT:;
                    switch (VM_GET_STACK(vm, 0)) {
                        case 0: // Integer
                            printf("%" DINT_PRINTF_d, VM_GET_STACK(vm, -2));
                            break;
                        case 1: // Float
                            printf("%g", VM_GET_STACK_FLOAT(vm, -2));
                            break;
                        case 2: // String
                            printf("%s", (char *)VM_GET_STACK(vm, -2));
                            break;
                        case 3: // Boolean
                            printf("%s",
                                   VM_GET_STACK(vm, -2) ? "true" : "false");
                            break;
                    }

                    if (VM_GET_STACK(vm, -1)) {
                        printf("\n");
                    }

                    *VM_GET_STACK_PTR(vm, -2) = 0;
                    break;

                case SYS_STRCMP:;
                    result = strcmp((char *)VM_GET_STACK(vm, -1),
                                    (char *)VM_GET_STACK(vm, -2));

                    switch (VM_GET_STACK(vm, 0)) {
                        case 0:
                            result = (result == 0);
                            break;
                        case 1:
                            result = (result <= 0);
                            break;
                        case 2:
                            result = (result < 0);
                            break;
                        case 3:
                            result = (result >= 0);
                            break;
                        case 4:
                            result = (result > 0);
                            break;
                        default:
                            result = 0;
                            break;
                    }

                    *VM_GET_STACK_PTR(vm, -2) = result;
                    break;

                case SYS_STRLEN:;
                    result = strlen((char *)VM_GET_STACK(vm, -2));
                    *VM_GET_STACK_PTR(vm, -2) = result;
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
            ERROR_RUNTIME(vm, "Unknown opcode %d", opcode);
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

    StackEntry *ptr = vm->stackPtr;

    while (ptr >= vm->basePtr) {
        dint offset = ptr - vm->stackPtr;
        printf("%" DINT_PRINTF_d "\t= ", offset);

        switch (ptr->type) {
            case TYPE_INT:
                printf("%" DINT_PRINTF_d " (Integer)", ptr->data.i);
                break;
            case TYPE_FLOAT:
                printf("%g (Float)", ptr->data.f);
                break;
            case TYPE_STRING:
                printf("%s (String)", (char *)ptr->data.p);
                break;
            case TYPE_BOOL:
                printf("%s (Boolean)", (ptr->data.b) ? "true" : "false");
                break;
            case TYPE_PTR:
                printf("%p (Pointer)", ptr->data.p);
                break;
            case TYPE_NONE:
                printf("(None)");
                break;
            default:
                printf("(Unknown)");
                break;
        }

        if (ptr == vm->framePtr) {
            printf("\t< frame ptr");
        }

        printf("\n");

        ptr--;
    }

    if (ptr == vm->framePtr) {
        printf("< frame ptr\n");
    }

    printf("\n");
}
