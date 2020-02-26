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
 * \fn void d_debug_add_call_info(DebugInfo *debugInfo, size_t ins,
 *                                InsCallInfo callInfo)
 * \brief Add call information to a list of debug information.
 *
 * \param debugInfo The debug info to add the call info to.
 * \param callInfo The call info to add.
 */
void d_debug_add_call_info(DebugInfo *debugInfo, size_t ins,
                           InsCallInfo callInfo) {
    InsDebugInfo insInfo;
    insInfo.ins           = ins;
    insInfo.infoType      = INFO_CALL;
    insInfo.info.callInfo = callInfo;

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

        printf("\nExecution info:\n");

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

        printf("\nNode info:\n");

        for (size_t i = 0; i < debugInfo.debugInfoSize; i++) {
            InsDebugInfo insInfo = debugInfo.debugInfoList[i];

            if (insInfo.infoType == INFO_NODE) {
                InsNodeInfo nodeInfo = insInfo.info.nodeInfo;

                printf("* Ins %zx activates node %zu.\n", insInfo.ins,
                       nodeInfo.node);
            }
        }

        printf("\nCall info:\n");

        for (size_t i = 0; i < debugInfo.debugInfoSize; i++) {
            InsDebugInfo insInfo = debugInfo.debugInfoList[i];

            if (insInfo.infoType == INFO_CALL) {
                InsCallInfo callInfo = insInfo.info.callInfo;

                printf("* Ins %zx calls %s defined in %s.\n", insInfo.ins,
                       callInfo.funcDef->name, callInfo.sheet->filePath);
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
 * \fn DebugSession d_debug_create_session(Sheet *sheet, DebugAgenda agenda)
 * \brief Create a debugging session.
 *
 * \return A debugging session in it's starting state.
 *
 * \param sheet The sheet to debug.
 * \param agenda The agenda the session should use.
 */
DebugSession d_debug_create_session(Sheet *sheet, DebugAgenda agenda) {
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

    // Zero-fill the sheet stack.
    memset(out.sheetStack, 0, DEBUG_SHEET_STACK_SIZE * sizeof(DebugStackEntry));

    // Set the first element to be the sheet we've been given.
    DebugStackEntry firstSheet;
    firstSheet.sheet            = sheet;
    firstSheet.numInternalCalls = 0;

    out.sheetStack[0] = firstSheet;
    out.stackPtr      = 0;

    out.vm     = vm;
    out.agenda = agenda;

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
 * \fn bool d_debug_continue_session(DebugSession *session)
 * \brief Continue a debugging session until either a breakpoint is hit, or the
 * VM halts.
 *
 * \return True if the debugger hit a breakpoint, false if the VM halted.
 *
 * \param session The session to continue.
 */
bool d_debug_continue_session(DebugSession *session) {
    DVM *vm = &(session->vm);

    DebugAgenda agenda = session->agenda;

    bool breakpointHit = false;

    // This will be very similar to d_vm_run.
    while (!vm->halted && !breakpointHit) {

        // Get the current sheet from the sheet stack.
        Sheet *sheet = session->sheetStack[session->stackPtr].sheet;

        bool inDebuggableSheet = (sheet->_debugInfo.debugInfoSize > 0);

        // Get the instruction index relative to the sheet.
        size_t ins = vm->pc - sheet->_text;

        DebugInfo debugInfo = sheet->_debugInfo;

        // Does this instruction transfer a value over a wire?
        int valueIndex = info_at_ins(debugInfo, ins, INFO_VALUE);

        if (valueIndex >= 0) {
            // If the index is valid, keep going through each of the items
            // with the same instruction in case there are more.
            while (valueIndex < (int)debugInfo.debugInfoSize &&
                   debugInfo.debugInfoList[valueIndex].ins == ins) {

                if (debugInfo.debugInfoList[valueIndex].infoType ==
                    INFO_VALUE) {

                    InsValueInfo valueInfo =
                        debugInfo.debugInfoList[valueIndex].info.valueInfo;

                    Wire wire = valueInfo.valueWire;
                    const SocketMeta meta =
                        d_get_socket_meta(sheet->graph, wire.socketFrom);
                    DType type    = meta.type;
                    LexData value = {0};

                    if (type == TYPE_INT) {
                        value.integerValue =
                            d_vm_get(&(session->vm), valueInfo.stackIndex);
                    } else if (type == TYPE_FLOAT) {
                        value.floatValue = d_vm_get_float(&(session->vm),
                                                          valueInfo.stackIndex);
                    } else if (type == TYPE_STRING) {
                        value.stringValue =
                            d_vm_get_ptr(&(session->vm), valueInfo.stackIndex);
                    } else if (type == TYPE_BOOL) {
                        value.booleanValue =
                            d_vm_get(&(session->vm), valueInfo.stackIndex);
                    }

                    if (agenda.onWireValue) {
                        agenda.onWireValue(sheet, wire, type, value);
                    }

                    if (agenda.wireBreakpoints != NULL) {
                        DebugWireBreakpoint *wireBreakpoint =
                            agenda.wireBreakpoints;

                        while (wireBreakpoint->sheet != NULL) {
                            if (d_wire_cmp(wire, wireBreakpoint->wire) == 0) {
                                if (agenda.onWireBreakpoint) {
                                    agenda.onWireBreakpoint(sheet, wire);
                                }

                                breakpointHit = true;
                            }

                            wireBreakpoint++;
                        }
                    }
                }

                valueIndex++;
            }
        }

        // Does this instruction activate an execution wire?
        int execIndex = info_at_ins(debugInfo, ins, INFO_EXEC);

        if (execIndex >= 0) {

            InsExecInfo execInfo =
                debugInfo.debugInfoList[execIndex].info.execInfo;

            if (agenda.onExecutionWire) {
                agenda.onExecutionWire(sheet, execInfo.execWire);
            }

            if (agenda.wireBreakpoints != NULL) {
                DebugWireBreakpoint *wireBreakpoint = agenda.wireBreakpoints;

                while (wireBreakpoint->sheet != NULL) {
                    if (d_wire_cmp(execInfo.execWire, wireBreakpoint->wire) ==
                        0) {
                        if (agenda.onWireBreakpoint) {
                            agenda.onWireBreakpoint(sheet, execInfo.execWire);
                        }

                        breakpointHit = true;
                    }

                    wireBreakpoint++;
                }
            }
        }

        // Is this instruction entering a new node?
        int nodeIndex = info_at_ins(debugInfo, ins, INFO_NODE);

        if (nodeIndex >= 0) {

            InsNodeInfo nodeInfo =
                debugInfo.debugInfoList[nodeIndex].info.nodeInfo;

            if (agenda.onNodedActivated) {
                agenda.onNodedActivated(sheet, nodeInfo.node);
            }

            if (agenda.nodeBreakpoints != NULL) {
                DebugNodeBreakpoint *nodeBreakpoint = agenda.nodeBreakpoints;

                while (nodeBreakpoint->sheet != NULL) {
                    if (nodeInfo.node == nodeBreakpoint->nodeIndex) {
                        if (agenda.onNodeBreakpoint) {
                            agenda.onNodeBreakpoint(sheet, nodeInfo.node);

                            breakpointHit = true;
                        }
                    }

                    nodeBreakpoint++;
                }
            }
        }

        DIns opcode = *(vm->pc);

        // Is this instruction calling a function?
        int callIndex = info_at_ins(debugInfo, ins, INFO_CALL);

        if (callIndex >= 0) {

            InsCallInfo callInfo =
                debugInfo.debugInfoList[callIndex].info.callInfo;

            // If the sheet the function belongs to is not the sheet we're
            // currently on, push it to the stack.
            if (callInfo.sheet != sheet) {
                if (session->stackPtr >= DEBUG_SHEET_STACK_SIZE) {
                    printf(
                        "Error: The debugger has hit the sheet stack limit\n");
                    return false;
                }

                DebugStackEntry newEntry;
                newEntry.sheet            = callInfo.sheet;
                newEntry.numInternalCalls = 0;

                session->sheetStack[++session->stackPtr] = newEntry;
            } else {
                session->sheetStack[session->stackPtr].numInternalCalls++;
            }

            if (agenda.onCall) {
                agenda.onCall(callInfo.sheet, callInfo.funcDef, callInfo.isC);
            }
        } else if (!inDebuggableSheet) {
            // If we're not in a debuggable sheet, it won't have call info in
            // it. But in order for us to track returns correctly, we still
            // need to know when it calls.
            switch (opcode) {
                case OP_CALL:
                case OP_CALLC:
                case OP_CALLCI:
                case OP_CALLI:
                case OP_CALLR:
                case OP_CALLRB:
                case OP_CALLRH:
                case OP_CALLRF:
                    session->sheetStack[session->stackPtr].numInternalCalls++;
                default:
                    break;
            }
        }

        // Is this instruction a return?
        if (opcode == OP_RET || opcode == OP_RETN) {
            // If there have been no internal calls in this sheet, we're going
            // back to the previous sheet.
            if (session->sheetStack[session->stackPtr].numInternalCalls == 0) {
                session->stackPtr--;
            } else {
                session->sheetStack[session->stackPtr].numInternalCalls--;
            }

            if (agenda.onReturn) {
                agenda.onReturn();
            }
        }

        d_vm_parse_ins_at_pc(vm);
        d_vm_inc_pc(vm);
    }

    // TODO: Have a way of knowing if the VM errored.
    return breakpointHit;
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

    memset(session->sheetStack, 0,
           DEBUG_SHEET_STACK_SIZE * sizeof(DebugStackEntry));
    session->stackPtr = -1;
    d_vm_free(&(session->vm));
    session->agenda = NO_AGENDA;
}