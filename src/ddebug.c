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

/**
 * \fn static void add_ins_info(DebugInfo *debugInfo, InsDebugInfo insInfo)
 * \brief Add instruction debugging information to a collection of debug
 * information such that it is sorted in ascending order of instruction
 * indexes.
 *
 * \param debugInfo The collection of debugging info to add to.
 * \param insInfo The instruction information to add.
 */
static void add_ins_info(DebugInfo *debugInfo, InsDebugInfo insInfo) {
    if (debugInfo == NULL) {
        return;
    }

    /// The list is being sorted in ascending order of instruction, so we need
    // to insert it into the correct position.
    // NOTE: This will be very similar to add_edge in dgraph.c
    if (debugInfo->debugInfoList == NULL) {
        debugInfo->debugInfoSize    = 1;
        debugInfo->debugInfoList    = d_malloc(sizeof(InsDebugInfo));
        *(debugInfo->debugInfoList) = insInfo;
    } else {
        // Use binary insertion, since the list should be sorted!
        int left   = 0;
        int right  = (int)debugInfo->debugInfoSize - 1;
        int middle = (left + right) / 2;

        while (left <= right) {
            middle = (left + right) / 2;

            if (insInfo.ins > debugInfo->debugInfoList[middle].ins) {
                left = middle + 1;
            } else if (insInfo.ins < debugInfo->debugInfoList[middle].ins) {
                right = middle - 1;
            } else {
                break;
            }
        }

        if (insInfo.ins > debugInfo->debugInfoList[middle].ins) {
            middle++;
        }

        debugInfo->debugInfoSize++;
        debugInfo->debugInfoList =
            d_realloc(debugInfo->debugInfoList,
                      debugInfo->debugInfoSize * sizeof(InsDebugInfo));

        if (middle < (int)debugInfo->debugInfoSize - 1) {
            memmove(debugInfo->debugInfoList + middle + 1,
                    debugInfo->debugInfoList + middle,
                    (debugInfo->debugInfoSize - middle - 1) *
                        sizeof(InsDebugInfo));
        }

        debugInfo->debugInfoList[middle] = insInfo;
    }
}

/**
 * \fn void d_debug_add_value_info(DebugInfo *debugInfo, size_t ins,
 *                                 InsValueInfo valueInfo)
 * \brief Add value information to a list of debug information.
 *
 * \param debugInfo The debug info to add the value info to.
 * \param valueInfo The value info to add.
 */
void d_debug_add_value_info(DebugInfo *debugInfo, size_t ins,
                            InsValueInfo valueInfo) {
    InsDebugInfo insInfo;
    insInfo.ins            = ins;
    insInfo.infoType       = INFO_VALUE;
    insInfo.info.valueInfo = valueInfo;

    add_ins_info(debugInfo, insInfo);
}

/**
 * \fn void d_debug_add_exec_info(DebugInfo *debugInfo, size_t ins,
 *                                InsExecInfo execInfo)
 * \brief Add execution information to a list of debug information.
 *
 * \param debugInfo The debug into to add the execution info to.
 * \param execInfo The execution info to add.
 */
void d_debug_add_exec_info(DebugInfo *debugInfo, size_t ins,
                           InsExecInfo execInfo) {
    InsDebugInfo insInfo;
    insInfo.ins           = ins;
    insInfo.infoType      = INFO_EXEC;
    insInfo.info.execInfo = execInfo;

    add_ins_info(debugInfo, insInfo);
}

/**
 * \fn void d_debug_add_node_info(DebugInfo *debugInfo, size_t ins,
 *                                InsNodeInfo nodeInfo)
 * \brief Add node information to a list of debug information.
 *
 * \param debugInfo The debug info to add the node info to.
 * \param nodeInfo The node info to add.
 */
void d_debug_add_node_info(DebugInfo *debugInfo, size_t ins,
                           InsNodeInfo nodeInfo) {
    InsDebugInfo insInfo;
    insInfo.ins           = ins;
    insInfo.infoType      = INFO_NODE;
    insInfo.info.nodeInfo = nodeInfo;

    add_ins_info(debugInfo, insInfo);
}

/**
 * \fn void d_debug_dump_info(DebugInfo debugInfo)
 * \brief Dump the debugging information to `stdout`.
 *
 * \param debugInfo The debugging info to dump.
 */
void d_debug_dump_info(DebugInfo debugInfo) {
    printf("DEBUG INFO DUMP\n");

    if (debugInfo.debugInfoList != NULL && debugInfo.debugInfoSize > 0) {
        printf("Value info:\n");

        for (size_t i = 0; i < debugInfo.debugInfoSize; i++) {
            InsDebugInfo insInfo = debugInfo.debugInfoList[i];

            if (insInfo.infoType == INFO_VALUE) {
                InsValueInfo valueInfo = insInfo.info.valueInfo;

                printf(
                    "* Ins %zx transfers value @ index %d in stack over wire "
                    "(Node %zu Socket %zu) -> (Node %zu Socket %zu).\n",
                    insInfo.ins, valueInfo.stackIndex,
                    valueInfo.valueWire.socketFrom.nodeIndex,
                    valueInfo.valueWire.socketFrom.socketIndex,
                    valueInfo.valueWire.socketTo.nodeIndex,
                    valueInfo.valueWire.socketTo.socketIndex);
            }
        }

        printf("\n");

        printf("Execution info:\n");

        for (size_t i = 0; i < debugInfo.debugInfoSize; i++) {
            InsDebugInfo insInfo = debugInfo.debugInfoList[i];

            if (insInfo.infoType == INFO_EXEC) {
                InsExecInfo execInfo = insInfo.info.execInfo;

                printf(
                    "* Ins %zx activates execution wire (Node %zu Socket %zu) "
                    "-> (Node %zu Socket %zu).\n",
                    insInfo.ins, execInfo.execWire.socketFrom.nodeIndex,
                    execInfo.execWire.socketFrom.socketIndex,
                    execInfo.execWire.socketTo.nodeIndex,
                    execInfo.execWire.socketTo.socketIndex);
            }
        }

        printf("\n");

        printf("Node info:\n");

        for (size_t i = 0; i < debugInfo.debugInfoSize; i++) {
            InsDebugInfo insInfo = debugInfo.debugInfoList[i];

            if (insInfo.infoType == INFO_NODE) {
                InsNodeInfo nodeInfo = insInfo.info.nodeInfo;

                printf("* Ins %zx activates node %zu.\n", insInfo.ins,
                       nodeInfo.node);
            }
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
    if (debugInfo->debugInfoList != NULL) {
        free(debugInfo->debugInfoList);
    }

    *debugInfo = NO_DEBUG_INFO;
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
    if (sheet->_debugInfo.debugInfoSize == 0) {
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

/**
 * \fn static int info_at_ins(DebugInfo debugInfo, size_t ins, InsInfoType type)
 * \brief Given an instruction, and the type of debug info you want, return
 * the index of first instance of that information.
 *
 * \return The index of to the information at that instruction, -1 if there is
 * no information of the given type at that instruction.
 *
 * \param debugInfo The debug info to query.
 * \param ins The instruction to query.
 * \param type The type of information to query.
 */
static int info_at_ins(DebugInfo debugInfo, size_t ins, InsInfoType type) {
    // NOTE: The list of InsValueInfo should be in order of instructions.
    if (debugInfo.debugInfoList == NULL || debugInfo.debugInfoSize == 0) {
        return -1;
    }

    int left   = 0;
    int right  = debugInfo.debugInfoSize - 1;
    int middle = (left + right) / 2;

    while (left <= right) {
        middle = (left + right) / 2;

        if (ins > debugInfo.debugInfoList[middle].ins) {
            left = middle + 1;
        } else if (ins < debugInfo.debugInfoList[middle].ins) {
            right = middle - 1;
        } else {
            break;
        }
    }

    if (debugInfo.debugInfoList[middle].ins == ins) {
        // We've found SOME information at the instruction, but we want to find
        // the first instance of the same type of information.

        // Go to the start of the list of info with the same instruction.
        while (debugInfo.debugInfoList[middle].ins == ins) {
            middle--;

            if (middle < 0) {
                break;
            }
        }

        middle++;

        // Go through the info until we find one with our type.
        while (debugInfo.debugInfoList[middle].ins == ins) {
            if (debugInfo.debugInfoList[middle].infoType == type) {
                return middle;
            }

            middle++;

            if (middle >= (int)debugInfo.debugInfoSize) {
                break;
            }
        }

        // If we don't find any with our type, welp...
        return -1;
    } else {
        return -1;
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

        DebugInfo debugInfo = session->sheet->_debugInfo;

        // Does this instruction transfer a value over a wire?
        if (session->onWireValue) {
            int valueIndex = info_at_ins(debugInfo, ins, INFO_VALUE);

            if (valueIndex >= 0) {
                // If the index is valid, keep going through each of the items
                // with the same instruction in case there are more.
                while (debugInfo.debugInfoList[valueIndex].ins == ins &&
                       valueIndex < (int)debugInfo.debugInfoSize) {

                    if (debugInfo.debugInfoList[valueIndex].infoType ==
                        INFO_VALUE) {

                        InsValueInfo valueInfo =
                            debugInfo.debugInfoList[valueIndex].info.valueInfo;

                        Wire wire             = valueInfo.valueWire;
                        const SocketMeta meta = d_get_socket_meta(
                            session->sheet->graph, wire.socketFrom);
                        DType type    = meta.type;
                        LexData value = {0};

                        if (type == TYPE_INT) {
                            value.integerValue =
                                d_vm_get(&(session->vm), valueInfo.stackIndex);
                        } else if (type == TYPE_FLOAT) {
                            value.floatValue = d_vm_get_float(
                                &(session->vm), valueInfo.stackIndex);
                        } else if (type == TYPE_STRING) {
                            value.stringValue = d_vm_get_ptr(
                                &(session->vm), valueInfo.stackIndex);
                        } else if (type == TYPE_BOOL) {
                            value.booleanValue =
                                d_vm_get(&(session->vm), valueInfo.stackIndex);
                        }

                        session->onWireValue(session->sheet, wire, type, value);
                    }

                    valueIndex++;
                }
            }
        }

        // Does this instruction activate an execution wire?
        if (session->onExecutionWire) {
            int execIndex = info_at_ins(debugInfo, ins, INFO_EXEC);

            if (execIndex >= 0) {
                while (debugInfo.debugInfoList[execIndex].ins == ins &&
                       execIndex < (int)debugInfo.debugInfoSize) {

                    if (debugInfo.debugInfoList[execIndex].infoType ==
                        INFO_EXEC) {

                        InsExecInfo execInfo =
                            debugInfo.debugInfoList[execIndex].info.execInfo;

                        session->onExecutionWire(session->sheet,
                                                 execInfo.execWire);
                    }

                    execIndex++;
                }
            }
        }

        // Is this instruction entering a new node?
        if (session->onNodedActivated) {
            int nodeIndex = info_at_ins(debugInfo, ins, INFO_NODE);

            if (nodeIndex >= 0) {
                while (debugInfo.debugInfoList[nodeIndex].ins == ins &&
                       nodeIndex < (int)debugInfo.debugInfoSize) {

                    if (debugInfo.debugInfoList[nodeIndex].infoType ==
                        INFO_NODE) {

                        InsNodeInfo nodeInfo =
                            debugInfo.debugInfoList[nodeIndex].info.nodeInfo;

                        session->onNodedActivated(session->sheet,
                                                  nodeInfo.node);
                    }

                    nodeIndex++;
                }
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