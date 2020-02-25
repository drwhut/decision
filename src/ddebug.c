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

#include "ddebug.h"

#include "dsheet.h"

#include <stdio.h>

/**
 * \fn static bool debug_info_empty(DebugInfo debugInfo)
 * \brief Check if there is no debug info.
 *
 * \return If there is no debugging info.
 *
 * \param debugInfo The debug info to check for emptiness.
 */
static bool debug_info_empty(DebugInfo debugInfo) {
    return (debugInfo.insValueInfoSize == 0 && debugInfo.insExecInfoSize == 0 &&
            debugInfo.insNodeInfoSize == 0);
}

/**
 * \fn DebugSession d_debug_create_session(Sheet *sheet,
 *                                         OnWireValue onWireValues,
 *                                         OnExecutionWire onExecutionWire,
 *                                         OnNodeActivated onNodeActivated
 *                                         )
 * \brief Create a debugging session.
 *
 * \return A debugging session in it's starting state.
 *
 * \param sheet The sheet to debug.
 * \param onWireValues A pointer to a function that is called when a value
 * is transfered over a wire during the session. If NULL, the function is not
 * called.
 * \param onExecutionWire A pointer to a function that is called when an
 * execution wire is activated during the session. If NULL, the function is
 * not called.
 * \param onNodeActivated A pointer to a function that is called when a
 * debuggable node is activated during the session. If NULL, the function is
 * not called.
 */
DebugSession d_debug_create_session(Sheet *sheet, OnWireValue onWireValue,
                                    OnExecutionWire onExecutionWire,
                                    OnNodeActivated onNodeActivated) {
    // Warn the user if the sheet does not have any debug info.
    if (debug_info_empty(sheet->_debugInfo)) {
        printf("Warning: %s does not contain debug information\n",
               sheet->filePath);
    }

    // Set up the VM that will run the sheet.
    DVM vm = d_vm_create();

    // TODO: What if the sheet doesn't have a Start node?
    vm.pc     = sheet->_text + sheet->_main;
    vm.halted = false;

    DebugSession out;

    out.vm               = vm;
    out.sheet            = sheet;
    out.onNodedActivated = onNodeActivated;
    out.onExecutionWire  = onExecutionWire;
    out.onWireValue      = onWireValue;

    return out;
}

/* TODO: Think of a way to combine the next 3 functions. */

/**
 * \fn static InsValueInfo *value_at_ins(DebugInfo debugInfo, size_t ins)
 * \brief Given an instruction, return the value wire information about it.
 *
 * \return A pointer to the value wire info at that instruction, NULL if there
 * is no info at that instruction.
 *
 * \param debugInfo The debug info to query.
 * \param ins The instruction to query.
 */
static InsValueInfo *value_at_ins(DebugInfo debugInfo, size_t ins) {
    // NOTE: The list of InsValueInfo should be in order of instructions.
    if (debugInfo.insValueInfoList == NULL) {
        return NULL;
    }

    int left   = 0;
    int right  = debugInfo.insValueInfoSize - 1;
    int middle = (left + right) / 2;

    while (left <= right) {
        middle = (left + right) / 2;

        if (ins > debugInfo.insValueInfoList[middle].ins) {
            left = middle + 1;
        } else if (ins < debugInfo.insValueInfoList[middle].ins) {
            right = middle - 1;
        } else {
            break;
        }
    }

    if (debugInfo.insValueInfoList[middle].ins == ins) {
        return debugInfo.insValueInfoList + middle;
    } else {
        return NULL;
    }
}

/**
 * \fn static InsExecInfo *exec_at_ins(DebugInfo debugInfo, size_t ins)
 * \brief Given an instruction, return the execution wire information about it.
 *
 * \return A pointer to the execution wire info at that instruction, NULL if
 * there is no info at that instruction.
 *
 * \param debugInfo The debug info to query.
 * \param ins The instruction to query.
 */
static InsExecInfo *exec_at_ins(DebugInfo debugInfo, size_t ins) {
    // NOTE: The list of InsExecInfo should be in order of instructions.
    if (debugInfo.insExecInfoList == NULL) {
        return NULL;
    }

    int left   = 0;
    int right  = debugInfo.insExecInfoSize - 1;
    int middle = (left + right) / 2;

    while (left <= right) {
        middle = (left + right) / 2;

        if (ins > debugInfo.insExecInfoList[middle].ins) {
            left = middle + 1;
        } else if (ins < debugInfo.insExecInfoList[middle].ins) {
            right = middle - 1;
        } else {
            break;
        }
    }

    if (debugInfo.insExecInfoList[middle].ins == ins) {
        return debugInfo.insExecInfoList + middle;
    } else {
        return NULL;
    }
}

/**
 * \fn static InsNodeInfo *node_at_ins(DebugInfo debugInfo, size_t ins)
 * \brief Given an instruction, return the node information about it.
 *
 * \return A pointer to the node info at that instruction, NULL if there is no
 * info at that instruction.
 *
 * \param debugInfo The debug info to query.
 * \param ins The instruction to query.
 */
static InsNodeInfo *node_at_ins(DebugInfo debugInfo, size_t ins) {
    // NOTE: The list of InsNodeInfo should be in order of instructions.
    if (debugInfo.insNodeInfoList == NULL) {
        return NULL;
    }

    int left   = 0;
    int right  = debugInfo.insNodeInfoSize - 1;
    int middle = (left + right) / 2;

    while (left <= right) {
        middle = (left + right) / 2;

        if (ins > debugInfo.insNodeInfoList[middle].ins) {
            left = middle + 1;
        } else if (ins < debugInfo.insNodeInfoList[middle].ins) {
            right = middle - 1;
        } else {
            break;
        }
    }

    if (debugInfo.insNodeInfoList[middle].ins == ins) {
        return debugInfo.insNodeInfoList + middle;
    } else {
        return NULL;
    }
}

/**
 * \fn void d_debug_continue_session(DebugSession *session)
 * \brief Continue a debugging session until either a breakpoint is hit, or the
 * VM halts.
 *
 * \param session The session to continue.
 */
void d_debug_continue_session(DebugSession *session) {
    DVM *vm = &(session->vm);

    // This will be very similar to d_vm_run.
    while (!vm->halted) {

        // Get the instruction index relative to the sheet.
        size_t ins = vm->pc - session->sheet->_text;

        // Does this instruction transfer a value over a wire?
        if (session->onWireValue) {
            InsValueInfo *valueInfo =
                value_at_ins(session->sheet->_debugInfo, ins);
            if (valueInfo) {
                Wire wire = session->sheet->graph.wires[valueInfo->valueWire];
                const SocketMeta meta =
                    d_get_socket_meta(session->sheet->graph, wire.socketFrom);
                DType type    = meta.type;
                LexData value = {0};

                if (type == TYPE_INT) {
                    value.integerValue =
                        d_vm_get(&(session->vm), valueInfo->stackIndex);
                } else if (type == TYPE_FLOAT) {
                    value.floatValue =
                        d_vm_get_float(&(session->vm), valueInfo->stackIndex);
                } else if (type == TYPE_STRING) {
                    value.stringValue =
                        d_vm_get_ptr(&(session->vm), valueInfo->stackIndex);
                } else if (type == TYPE_BOOL) {
                    value.booleanValue =
                        d_vm_get(&(session->vm), valueInfo->stackIndex);
                }

                session->onWireValue(session->sheet, wire, type, value);
            }
        }

        // Does this instruction activate an execution wire?
        if (session->onExecutionWire) {
            InsExecInfo *execInfo =
                exec_at_ins(session->sheet->_debugInfo, ins);
            if (execInfo) {
                Wire wire = session->sheet->graph.wires[execInfo->execWire];
                session->onExecutionWire(session->sheet, wire);
            }
        }

        // Is this instruction entering a new node?
        if (session->onNodedActivated) {
            InsNodeInfo *nodeInfo =
                node_at_ins(session->sheet->_debugInfo, ins);
            if (nodeInfo) {
                session->onNodedActivated(session->sheet, nodeInfo->node);
            }
        }

        d_vm_parse_ins_at_pc(vm);
        d_vm_inc_pc(vm);
    }

    // TODO: Have a way of knowing if the VM errored.
}

/**
 * \fn void d_debug_stop_session(DebugSession *session)
 * \brief Stop a debugging session, freeing all of the memory malloc'd by the
 * session. The session should not be used afterwards.
 *
 * \param session The session to stop.
 */
void d_debug_stop_session(DebugSession *session) {
    if (session == NULL) {
        return;
    }

    d_vm_free(&(session->vm));
    session->sheet            = NULL;
    session->onNodedActivated = NULL;
}