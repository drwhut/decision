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

#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A constant array of the size of each opcode's instruction in bytes. */
static const unsigned char VM_INS_SIZE[NUM_OPCODES] = {
    1,                  // OP_RET
    3,                  // OP_ADD
    3,                  // OP_ADDF
    2 + IMMEDIATE_SIZE, // OP_ADDI
    3,                  // OP_AND
    2 + IMMEDIATE_SIZE, // OP_ANDI
    2,                  // OP_CALL
    1 + IMMEDIATE_SIZE, // OP_CALLR
    4,                  // OP_CEQ
    4,                  // OP_CEQF
    4,                  // OP_CEQS
    4,                  // OP_CLEQ
    4,                  // OP_CLEQF
    4,                  // OP_CLEQS
    4,                  // OP_CLT
    4,                  // OP_CLTF
    4,                  // OP_CLTS
    4,                  // OP_CMEQ
    4,                  // OP_CMEQF
    4,                  // OP_CMEQS
    4,                  // OP_CMT
    4,                  // OP_CMTF
    4,                  // OP_CMTS
    3,                  // OP_CVTF
    3,                  // OP_CVTI
    3,                  // OP_DIV
    3,                  // OP_DIVF
    2 + IMMEDIATE_SIZE, // OP_DIVI
    2,                  // OP_J
    3,                  // OP_JR
    1 + IMMEDIATE_SIZE, // OP_JR
    2 + IMMEDIATE_SIZE, // OP_JRCON
    3,                  // OP_LOAD
    3,                  // OP_LOADADR
    3,                  // OP_LOADADRB
    3,                  // OP_LOADARG
    2 + IMMEDIATE_SIZE, // OP_LOADARGI
    3,                  // OP_LOADF
    2 + IMMEDIATE_SIZE, // OP_LOADI
    2 + IMMEDIATE_SIZE, // OP_LOADUI
    3,                  // OP_MOD
    2 + IMMEDIATE_SIZE, // OP_MODI
    3,                  // OP_MUL
    3,                  // OP_MULF
    2 + IMMEDIATE_SIZE, // OP_MULI
    3,                  // OP_MVTF
    3,                  // OP_MVTI
    2,                  // OP_NOT
    3,                  // OP_OR
    2 + IMMEDIATE_SIZE, // OP_ORI
    2,                  // OP_POP
    2,                  // OP_PUSH
    3,                  // OP_STOADR
    3,                  // OP_STOADRB
    3,                  // OP_SUB
    3,                  // OP_SUBF
    2 + IMMEDIATE_SIZE, // OP_SUBI
    2,                  // OP_SYSCALL
    3,                  // OP_XOR
    2 + IMMEDIATE_SIZE, // OP_XORI
};

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
 * \fn void d_vm_reset(DVM *vm)
 * \brief Reset a Decision VM object to it's starting state.
 *
 * \param vm A Decision VM to set to it's starting state.
 */
void d_vm_reset(DVM *vm) {
    vm->pc      = 0x0;
    vm->_inc_pc = 0;

    vm->_generalStackPtr = -1;
    vm->_callStackPtr    = -1;

    vm->halted       = true;
    vm->runtimeError = false;
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

/* Macros to help manipulate registers more generally. */
#define REG_OP_1(op) \
    vm->registers[GET_BYTEN(vm->pc, 1)] op vm->registers[GET_BYTEN(vm->pc, 1)]
#define REG_FOP_1(op)                                                   \
    vm->floatRegisters[GET_BYTEN(vm->pc, 1) - VM_REG_FLOAT_START] op vm \
        ->floatRegisters[GET_BYTEN(vm->pc, 1) - VM_REG_FLOAT_START]

#define REG_OP_2(op) \
    vm->registers[GET_BYTEN(vm->pc, 1)] op vm->registers[GET_BYTEN(vm->pc, 2)]
#define REG_FOP_2(op)                                                   \
    vm->floatRegisters[GET_BYTEN(vm->pc, 1) - VM_REG_FLOAT_START] op vm \
        ->floatRegisters[GET_BYTEN(vm->pc, 2) - VM_REG_FLOAT_START]

#define REG_OP_CON(op)                             \
    vm->registers[GET_BYTEN(vm->pc, 1)] =          \
        (vm->registers[GET_BYTEN(vm->pc, 2)] op vm \
             ->registers[GET_BYTEN(vm->pc, 3)])    \
            ? 1                                    \
            : 0
#define REG_FOP_CON(op)                                                      \
    vm->registers[GET_BYTEN(vm->pc, 1)] =                                    \
        (vm->floatRegisters[GET_BYTEN(vm->pc, 2) - VM_REG_FLOAT_START] op vm \
             ->floatRegisters[GET_BYTEN(vm->pc, 3) - VM_REG_FLOAT_START])    \
            ? 1                                                              \
            : 0
#define REG_SOP_CON(op)                                            \
    vm->registers[GET_BYTEN(vm->pc, 1)] =                          \
        (strcmp((char *)vm->registers[GET_BYTEN(vm->pc, 2)],       \
                (char *)vm->registers[GET_BYTEN(vm->pc, 3)]) op 0) \
            ? 1                                                    \
            : 0

#define REG_OP_I(op) \
    vm->registers[GET_BYTEN(vm->pc, 1)] op GET_IMMEDIATE_PTR(vm->pc + 2)
#define REG_OP_UI(op) \
    vm->registers[GET_BYTEN(vm->pc, 1)] op GET_UPPER_IMMEDIATE_PTR(vm->pc + 2)

/**
 * \fn void d_vm_parse_ins_at_pc(DVM *vm)
 * \brief Given a Decision VM, at it's current position in the program, parse
 * the instruction at that position.
 *
 * \param vm The VM to use to parse the instruction.
 */
void d_vm_parse_ins_at_pc(DVM *vm) {
    DIns opcode = *(vm->pc);

    // Set how many bytes we need to increment.
    // However, if we're about to perform a jump, we need to set this value to
    // 0.
    vm->_inc_pc = VM_INS_SIZE[opcode];

    // Immediate holder variable.
    immediate_t immediate;

    // Do stuff depending on what the opcode is.
    switch (opcode) {
        case OP_RET:;
            // If the call stack is empty, halt the VM.
            if (vm->_callStackPtr < 0) {
                vm->halted = true;
            }
            // Otherwise, pop the call stack.
            else {
                vm->pc = vm->callStack[vm->_callStackPtr--];

                // We don't want to increment now we're in the spot we want to
                // be in!
                vm->_inc_pc = 0;
            }
            break;

        case OP_ADD:;
            REG_OP_2(+=);
            break;

        case OP_ADDF:;
            REG_FOP_2(+=);
            break;

        case OP_ADDI:;
            REG_OP_I(+= (immediate_t));
            break;

        case OP_AND:;
            REG_OP_2(&=);
            break;

        case OP_ANDI:;
            REG_OP_I(&=);
            break;

        case OP_CALL:;
            if (vm->_callStackPtr >= VM_GENERAL_STACK_CAPACITY - 1) {
                d_vm_runtime_error(vm, "Call stack has reached capacity");
            } else {
                // Push the instruction after this call to the stack, so when
                // we return, we start off from there.
                vm->callStack[++vm->_callStackPtr] =
                    vm->pc + VM_INS_SIZE[OP_CALL];

                // OP_J
                vm->pc =
                    (char *)(intptr_t)(vm->registers[GET_BYTEN(vm->pc, 1)]);

                vm->_inc_pc = 0;
            }
            break;

        case OP_CALLR:;
            if (vm->_callStackPtr >= VM_CALL_STACK_CAPACITY - 1) {
                d_vm_runtime_error(vm, "Call stack has reached capacity.");
            } else {
                // Push the instruction after this call to the stack, so when
                // we return, we start off from there.
                vm->callStack[++vm->_callStackPtr] =
                    vm->pc + VM_INS_SIZE[OP_CALLR];

                // OP_JR
                immediate_t i = GET_IMMEDIATE_PTR(vm->pc + 1);
                d_vm_add_pc(vm, i);

                vm->_inc_pc = 0;
            }
            break;

        case OP_CEQ:;
            REG_OP_CON(==);
            break;

        case OP_CEQF:;
            REG_FOP_CON(==);
            break;

        case OP_CEQS:;
            REG_SOP_CON(==);
            break;

        case OP_CLEQ:;
            REG_OP_CON(<=);
            break;

        case OP_CLEQF:;
            REG_FOP_CON(<=);
            break;

        case OP_CLEQS:;
            REG_SOP_CON(<=);
            break;

        case OP_CLT:;
            REG_OP_CON(<);
            break;

        case OP_CLTF:;
            REG_FOP_CON(<);
            break;

        case OP_CLTS:;
            REG_SOP_CON(<);
            break;

        case OP_CMEQ:;
            REG_OP_CON(>=);
            break;

        case OP_CMEQF:;
            REG_FOP_CON(>=);
            break;

        case OP_CMEQS:;
            REG_SOP_CON(>=);
            break;

        case OP_CMT:;
            REG_OP_CON(>);
            break;

        case OP_CMTF:;
            REG_FOP_CON(>);
            break;

        case OP_CMTS:;
            REG_SOP_CON(>);
            break;

        case OP_CVTF:;
            vm->floatRegisters[GET_BYTEN(vm->pc, 2) - VM_REG_FLOAT_START] =
                (dfloat)vm->registers[GET_BYTEN(vm->pc, 1)];
            break;

        case OP_CVTI:;
            vm->registers[GET_BYTEN(vm->pc, 2)] =
                (dint)vm
                    ->floatRegisters[GET_BYTEN(vm->pc, 1) - VM_REG_FLOAT_START];
            break;

        case OP_DIV:;
            REG_OP_2(/=);
            break;

        case OP_DIVF:;
            REG_FOP_2(/=);
            break;

        case OP_DIVI:;
            REG_OP_I(/= (immediate_t));
            break;

        case OP_J:;
            vm->pc = (char *)(intptr_t)(vm->registers[GET_BYTEN(vm->pc, 1)]);

            vm->_inc_pc = 0;
            break;

        case OP_JCON:;
            if (vm->registers[GET_BYTEN(vm->pc, 2)]) {
                vm->pc =
                    (char *)(intptr_t)(vm->registers[GET_BYTEN(vm->pc, 1)]);

                vm->_inc_pc = 0;
            }
            break;

        case OP_JR:;
            immediate = GET_IMMEDIATE_PTR(vm->pc + 1);
            d_vm_add_pc(vm, immediate);

            vm->_inc_pc = 0;
            break;

        case OP_JRCON:;
            if (vm->registers[GET_BYTEN(vm->pc, 1)]) {
                immediate = GET_IMMEDIATE_PTR(vm->pc + 2);
                d_vm_add_pc(vm, immediate);

                vm->_inc_pc = 0;
            }
            break;

        case OP_LOAD:;
            REG_OP_2(=);
            break;

        case OP_LOADADR:;
            vm->registers[GET_BYTEN(vm->pc, 1)] =
                *((dint *)(vm->registers[GET_BYTEN(vm->pc, 2)]));
            break;

        case OP_LOADADRB:;
            vm->registers[GET_BYTEN(vm->pc, 1)] =
                *((uint8_t *)(vm->registers[GET_BYTEN(vm->pc, 2)]));
            break;

        case OP_LOADARG:;
            vm->argRegisters[GET_BYTEN(vm->pc, 1)] =
                vm->registers[GET_BYTEN(vm->pc, 2)];
            break;

        case OP_LOADARGI:;
            vm->argRegisters[GET_BYTEN(vm->pc, 1)] =
                GET_IMMEDIATE_PTR(vm->pc + 2);
            break;

        case OP_LOADF:;
            REG_FOP_2(=);
            break;

        case OP_LOADI:;
            REG_OP_I(=);
            break;

        case OP_LOADUI:;
            REG_OP_UI(=);
            break;

        case OP_MOD:;
            REG_OP_2(%=);
            break;

        case OP_MODI:;
            REG_OP_I(%= (immediate_t));
            break;

        case OP_MUL:;
            REG_OP_2(*=);
            break;

        case OP_MULF:;
            REG_FOP_2(*=);
            break;

        case OP_MULI:;
            REG_OP_I(*= (immediate_t));
            break;

        case OP_MVTF:;
            vm->floatRegisters[GET_BYTEN(vm->pc, 2) - VM_REG_FLOAT_START] =
                *((dfloat *)(vm->registers + GET_BYTEN(vm->pc, 1)));
            break;

        case OP_MVTI:;
            vm->registers[GET_BYTEN(vm->pc, 2)] =
                *((dint *)(vm->floatRegisters + GET_BYTEN(vm->pc, 1) -
                           VM_REG_FLOAT_START));
            break;

        case OP_NOT:
            REG_OP_1(= !);
            break;

        case OP_OR:;
            REG_OP_2(|=);
            break;

        case OP_ORI:;
            REG_OP_I(|=);
            break;

        case OP_POP:;
            // If the general stack is empty, produce a runtime error.
            if (vm->_generalStackPtr < 0) {
                d_vm_runtime_error(vm, "popping an empty general stack");
            }
            // Otherwise, pop the general stack.
            else {
                vm->registers[GET_BYTEN(vm->pc, 1)] =
                    vm->generalStack[vm->_generalStackPtr--];
            }
            break;

        case OP_PUSH:;
            if (vm->_generalStackPtr >= VM_GENERAL_STACK_CAPACITY - 1) {
                d_vm_runtime_error(vm, "General stack has reached capacity");
            } else {
                vm->generalStack[++vm->_generalStackPtr] =
                    vm->registers[GET_BYTEN(vm->pc, 1)];
            }
            break;

        case OP_STOADR:;
            *((dint *)(vm->registers[GET_BYTEN(vm->pc, 2)])) =
                vm->registers[GET_BYTEN(vm->pc, 1)];
            break;

        case OP_STOADRB:;
            *((uint8_t *)(vm->registers[GET_BYTEN(vm->pc, 2)])) =
                (uint8_t)vm->registers[GET_BYTEN(vm->pc, 1)];
            break;

        case OP_SUB:;
            REG_OP_2(-=);
            break;

        case OP_SUBF:;
            REG_FOP_2(-=);
            break;

        case OP_SUBI:;
            REG_OP_I(-= (immediate_t));
            break;

        case OP_SYSCALL:;
            DSyscall syscall = GET_BYTEN(vm->pc, 1);

            switch (syscall) {
                case SYS_LOADSTR:;
                    reg_t varStrReg = (reg_t)vm->argRegisters[0];
                    reg_t cpyStrReg = (reg_t)vm->argRegisters[1];
                    size_t len1 = strlen((char *)vm->registers[varStrReg]) + 1;
                    size_t len2 = strlen((char *)vm->registers[cpyStrReg]) + 1;
                    if (len1 != len2) {
                        vm->registers[varStrReg] = (dint)d_realloc(
                            (char *)vm->registers[varStrReg], len2);
                    }
                    memcpy((char *)vm->registers[varStrReg],
                           (char *)vm->registers[cpyStrReg], len2);
                    break;

                case SYS_PRINT:;
                    reg_t regToPrint = (reg_t)vm->argRegisters[1];
                    switch (vm->argRegisters[0]) {
                        case 0: // Integer
                            printf("%" DINT_PRINTF_d,
                                   vm->registers[regToPrint]);
                            break;
                        case 1: // Float
                            printf("%g",
                                   vm->floatRegisters[regToPrint -
                                                      VM_REG_FLOAT_START]);
                            break;
                        case 2: // String
                            printf("%s", (char *)(intptr_t)(
                                             vm->registers[regToPrint]));
                            break;
                        case 3: // Boolean
                            printf("%s", (vm->registers[regToPrint]) ? "true"
                                                                     : "false");
                            break;
                    }
                    if (vm->argRegisters[2])
                        printf("\n");
                    break;

                default:
                    break;
            }
            break;

        case OP_XOR:;
            REG_OP_2(^=);
            break;

        case OP_XORI:;
            REG_OP_I(^=);
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
 * **NOTE:** The VM will ALWAYS increment the PC after every instruction.
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
