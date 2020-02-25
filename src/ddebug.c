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

#include "dmalloc.h"
#include "dsheet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Find a way to combine the next 3 functions. */

/**
 * \fn void d_debug_add_value_info(DebugInfo *debugInfo, InsValueInfo valueInfo)
 * \brief Add value information to a list of debug information.
 *
 * \param debugInfo The debug info to add the value info to.
 * \param valueInfo The value info to add.
 */
void d_debug_add_value_info(DebugInfo *debugInfo, InsValueInfo valueInfo) {
    if (debugInfo == NULL) {
        return;
    }

    // NOTE: The list of InsValueInfo should be in order of instructions.
    // NOTE: This is very similar to add_edge in dgraph.c, since that has to
    // add wires in a sorted list.
    if (debugInfo->insValueInfoList == NULL) {
        debugInfo->insValueInfoSize    = 1;
        debugInfo->insValueInfoList    = d_malloc(sizeof(InsValueInfo));
        *(debugInfo->insValueInfoList) = valueInfo;
    } else {
        // Use binary insertion, since the list should be sorted!
        int left   = 0;
        int right  = (int)debugInfo->insValueInfoSize - 1;
        int middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            if (valueInfo.ins > debugInfo->insValueInfoList[middle].ins) {
                left = middle + 1;
            } else if (valueInfo.ins <
                       debugInfo->insValueInfoList[middle].ins) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (valueInfo.ins > debugInfo->insValueInfoList[middle].ins) {
            middle++;
        }

        // Even if two entries point to the same instruction, that's fine.
        debugInfo->insValueInfoSize++;
        debugInfo->insValueInfoList =
            d_realloc(debugInfo->insValueInfoList,
                      debugInfo->insValueInfoSize * sizeof(InsValueInfo));

        if (middle < (int)debugInfo->insValueInfoSize - 1) {
            memmove(debugInfo->insValueInfoList + middle + 1,
                    debugInfo->insValueInfoList + middle,
                    (debugInfo->insValueInfoSize - middle - 1) *
                        sizeof(InsValueInfo));
        }

        debugInfo->insValueInfoList[middle] = valueInfo;
    }
}

/**
 * \fn void d_debug_add_exec_info(DebugInfo *debugInfo, InsExecInfo execInfo)
 * \brief Add execution information to a list of debug information.
 *
 * \param debugInfo The debug into to add the execution info to.
 * \param execInfo The execution info to add.
 */
void d_debug_add_exec_info(DebugInfo *debugInfo, InsExecInfo execInfo) {
    if (debugInfo == NULL) {
        return;
    }

    // NOTE: The list of InsExecInfo should be in order of instructions.
    // NOTE: This is very similar to add_edge in dgraph.c, since that has to
    // add wires in a sorted list.
    if (debugInfo->insExecInfoList == NULL) {
        debugInfo->insExecInfoSize    = 1;
        debugInfo->insExecInfoList    = d_malloc(sizeof(InsExecInfo));
        *(debugInfo->insExecInfoList) = execInfo;
    } else {
        // Use binary insertion, since the list should be sorted!
        int left   = 0;
        int right  = (int)debugInfo->insExecInfoSize - 1;
        int middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            if (execInfo.ins > debugInfo->insExecInfoList[middle].ins) {
                left = middle + 1;
            } else if (execInfo.ins < debugInfo->insExecInfoList[middle].ins) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (execInfo.ins > debugInfo->insExecInfoList[middle].ins) {
            middle++;
        }

        // We can't activate two execution wires with the same instruction!
        // If this node info has the same instruction, replace the entry in the
        // list.
        if (execInfo.ins != debugInfo->insExecInfoList[middle].ins) {
            debugInfo->insExecInfoSize++;
            debugInfo->insExecInfoList =
                d_realloc(debugInfo->insExecInfoList,
                          debugInfo->insExecInfoSize * sizeof(InsExecInfo));

            if (middle < (int)debugInfo->insExecInfoSize - 1) {
                memmove(debugInfo->insExecInfoList + middle + 1,
                        debugInfo->insExecInfoList + middle,
                        (debugInfo->insExecInfoSize - middle - 1) *
                            sizeof(InsExecInfo));
            }
        }

        debugInfo->insExecInfoList[middle] = execInfo;
    }
}

/**
 * \fn void d_debug_add_node_info(DebugInfo *debugInfo, InsNodeInfo nodeInfo)
 * \brief Add node information to a list of debug information.
 *
 * \param debugInfo The debug info to add the node info to.
 * \param nodeInfo The node info to add.
 */
void d_debug_add_node_info(DebugInfo *debugInfo, InsNodeInfo nodeInfo) {
    if (debugInfo == NULL) {
        return;
    }

    // NOTE: The list of InsNodeInfo should be in order of instructions.
    // NOTE: This is very similar to add_edge in dgraph.c, since that has to
    // add wires in a sorted list.
    if (debugInfo->insNodeInfoList == NULL) {
        debugInfo->insNodeInfoSize    = 1;
        debugInfo->insNodeInfoList    = d_malloc(sizeof(InsNodeInfo));
        *(debugInfo->insNodeInfoList) = nodeInfo;
    } else {
        // Use binary insertion, since the list should be sorted!
        int left   = 0;
        int right  = (int)debugInfo->insNodeInfoSize - 1;
        int middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            if (nodeInfo.ins > debugInfo->insNodeInfoList[middle].ins) {
                left = middle + 1;
            } else if (nodeInfo.ins < debugInfo->insNodeInfoList[middle].ins) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (nodeInfo.ins > debugInfo->insNodeInfoList[middle].ins) {
            middle++;
        }

        // We can't enter two nodes with the same instruction!
        // If this node info has the same instruction, replace the entry in the
        // list.
        if (nodeInfo.ins != debugInfo->insNodeInfoList[middle].ins) {
            debugInfo->insNodeInfoSize++;
            debugInfo->insNodeInfoList =
                d_realloc(debugInfo->insNodeInfoList,
                          debugInfo->insNodeInfoSize * sizeof(InsNodeInfo));

            if (middle < (int)debugInfo->insNodeInfoSize - 1) {
                memmove(debugInfo->insNodeInfoList + middle + 1,
                        debugInfo->insNodeInfoList + middle,
                        (debugInfo->insNodeInfoSize - middle - 1) *
                            sizeof(InsNodeInfo));
            }
        }

        debugInfo->insNodeInfoList[middle] = nodeInfo;
    }
}

/**
 * \fn void d_debug_dump_info(DebugInfo debugInfo)
 * \brief Dump the debugging information to `stdout`.
 *
 * \param debugInfo The debugging info to dump.
 */
void d_debug_dump_info(DebugInfo debugInfo) {
    printf("DEBUG INFO DUMP\n");

    if (debugInfo.insValueInfoList != NULL && debugInfo.insValueInfoSize > 0) {
        printf("Value info:\n");

        for (size_t i = 0; i < debugInfo.insValueInfoSize; i++) {
            InsValueInfo valueInfo = debugInfo.insValueInfoList[i];

            printf("* Ins %zx transfers value @ index %d in stack over wire "
                   "(Node %zu Socket %zu) -> (Node %zu Socket %zu).\n",
                   valueInfo.ins, valueInfo.stackIndex,
                   valueInfo.valueWire.socketFrom.nodeIndex,
                   valueInfo.valueWire.socketFrom.socketIndex,
                   valueInfo.valueWire.socketTo.nodeIndex,
                   valueInfo.valueWire.socketTo.socketIndex);
        }

        printf("\n");
    }

    if (debugInfo.insExecInfoList != NULL && debugInfo.insExecInfoSize > 0) {
        printf("Execution info:\n");

        for (size_t i = 0; i < debugInfo.insExecInfoSize; i++) {
            InsExecInfo execInfo = debugInfo.insExecInfoList[i];

            printf("* Ins %zx activates execution wire (Node %zu Socket %zu) "
                   "-> (Node %zu Socket %zu).\n",
                   execInfo.ins, execInfo.execWire.socketFrom.nodeIndex,
                   execInfo.execWire.socketFrom.socketIndex,
                   execInfo.execWire.socketTo.nodeIndex,
                   execInfo.execWire.socketTo.socketIndex);
        }

        printf("\n");
    }

    if (debugInfo.insNodeInfoList != NULL && debugInfo.insNodeInfoSize > 0) {
        printf("Node info:\n");

        for (size_t i = 0; i < debugInfo.insNodeInfoSize; i++) {
            InsNodeInfo nodeInfo = debugInfo.insNodeInfoList[i];

            printf("* Ins %zx activates node %zu.\n", nodeInfo.ins,
                   nodeInfo.node);
        }

        printf("\n");
    }
}

/**
 * \fn void d_debug_free_info(DebugInfo *debugInfo)
 * \brief Free debugging information.
 *
 * \param debugInfo The debugging info to free.
 */
void d_debug_free_info(DebugInfo *debugInfo) {
    if (debugInfo->insValueInfoList != NULL) {
        free(debugInfo->insValueInfoList);
    }

    if (debugInfo->insExecInfoList != NULL) {
        free(debugInfo->insExecInfoList);
    }

    if (debugInfo->insNodeInfoList != NULL) {
        free(debugInfo->insNodeInfoList);
    }

    *debugInfo = NO_DEBUG_INFO;
}

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
 *                                         OnNodeActivated onNodeActivated)
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
                Wire wire = valueInfo->valueWire;
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
                session->onExecutionWire(session->sheet, execInfo->execWire);
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