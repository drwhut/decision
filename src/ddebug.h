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

/**
 * \file ddebug.h
 * \brief This header contains functions to debug Decision sheets.
 */

#ifndef DDEBUG_H
#define DDEBUG_H

#include "dcfg.h"
#include "dgraph.h"
#include "dlex.h"
#include "dtype.h"
#include "dvm.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* Forward declaration of the NodeDefinition struct from dgraph.h */
struct _nodeDefinition;

/**
 * \struct _insValueInfo
 * \brief Describes when a value wire has a value in bytecode.
 *
 * \typedef struct _insValueInfo InsValueInfo
 */
typedef struct _insValueInfo {
    Wire valueWire;
    int stackIndex;
} InsValueInfo;

/**
 * \struct _insExecInfo
 * \brief Describes when an execution wire gets activated in bytecode.
 *
 * \typedef struct _insExecInfo InsExecInfo
 */
typedef struct _insExecInfo {
    Wire execWire;
} InsExecInfo;

/**
 * \struct _insNodeInfo
 * \brief Describes when a node "starts" in bytecode.
 *
 * \typedef struct _insNodeInfo InsNodeInfo
 */
typedef struct _insNodeInfo {
    size_t node;
} InsNodeInfo;

/**
 * \struct _insCallInfo
 * \brief Describes a call in bytecode.
 *
 * \typedef struct _insCallInfo InsCallInfo
 */
typedef struct _insCallInfo {
    struct _sheet *sheet;
    const struct _nodeDefinition *funcDef;
    bool isC;
} InsCallInfo;

/**
 * \enum _insInfoType
 * \brief The different kinds of information debugging can store and output.
 *
 * \typedef enum _insInfoType InsInfoType
 */
typedef enum _insInfoType {
    INFO_VALUE,
    INFO_EXEC,
    INFO_NODE,
    INFO_CALL
} InsInfoType;

/**
 * \union _insInfoCollection
 * \brief A union collection of all the different kinds of debug information
 * that can be stored.
 *
 * \typedef union _insInfoCollection InsInfoCollection
 */
typedef union _insInfoCollection {
    InsValueInfo valueInfo;
    InsExecInfo execInfo;
    InsNodeInfo nodeInfo;
    InsCallInfo callInfo;
} InsInfoCollection;

/**
 * \struct _insDebugInfo
 * \brief Describes what an instruction does.
 *
 * \typedef struct _insDebugInfo InsDebugInfo
 */
typedef struct _insDebugInfo {
    InsInfoCollection info;
    size_t ins;
    InsInfoType infoType;
} InsDebugInfo;

/**
 * \struct _debugInfo
 * \brief A collection of info used for debugging.
 *
 * \typedef struct _debugInfo DebugInfo
 */
typedef struct _debugInfo {
    InsDebugInfo *debugInfoList;
    size_t debugInfoSize;
} DebugInfo;

/**
 * \def NO_DEBUG_INFO
 * \brief An empty DebugInfo struct.
 */
#define NO_DEBUG_INFO \
    (DebugInfo) {     \
        NULL, 0       \
    }

/**
 * \typedef void (*OnWireValue)(Sheet *sheet, Wire wire, DType type,
 *                              LexData value)
 * \brief Called when a value is transfered over a wire during a debugging
 * session.
 */
typedef void (*OnWireValue)(struct _sheet *sheet, Wire wire, DType type,
                            LexData value);

/**
 * \typedef void (*OnExecutionWire)(Sheet *sheet, Wire wire)
 * \brief Called when an execution wire is activated during a debugging session.
 */
typedef void (*OnExecutionWire)(struct _sheet *sheet, Wire wire);

/**
 * \typedef void (*OnNodeActivated)(Sheet *sheet, size_t nodeIndex)
 * \brief Called when a node is activated during a debugging session.
 */
typedef void (*OnNodeActivated)(struct _sheet *sheet, size_t nodeIndex);

/**
 * \typedef void (*OnCall)(Sheet *sheet, const NodeDefinition *funcDef,
 *                         bool isC)
 * \brief Called when a call occurs during a debugging session.
 */
typedef void (*OnCall)(struct _sheet *sheet,
                       const struct _nodeDefinition *funcDef, bool isC);

/**
 * \typedef void (*OnReturn)()
 * \brief Called when a return occurs during a debugging session.
 */
typedef void (*OnReturn)();

/**
 * \struct _debugAgenda
 * \brief Describes the "agenda" of how a debugging session should handle
 * certain events.
 *
 * \typedef struct _debugAgenda DebugAgenda
 */
typedef struct _debugAgenda {
    OnWireValue onWireValue;
    OnExecutionWire onExecutionWire;
    OnNodeActivated onNodedActivated;
    OnCall onCall;
    OnReturn onReturn;
} DebugAgenda;

/**
 * \def NO_AGENDA
 * \brief An empty agenda, i.e. nothing will happen when debugging.
 */
#define NO_AGENDA                    \
    (DebugAgenda) {                  \
        NULL, NULL, NULL, NULL, NULL \
    }

/**
 * \def DEBUG_SHEET_STACK_SIZE
 * \brief The size of a sheet stack in a debugging session. Since the stack is
 * not dynamically allocated, this number should be just above the realistic
 * number of sheet "hops".
 */
#define DEBUG_SHEET_STACK_SIZE 64

/**
 * \struct _debugStackEntry
 * \brief Describes an entry in the debugging session's stack.
 *
 * \typedef struct _debugStackEntry DebugStackEntry
 */
typedef struct _debugStackEntry {
    struct _sheet *sheet;
    size_t numInternalCalls;
} DebugStackEntry;

/**
 * \struct _debugSession
 * \brief A struct which keeps track of a debugging session.
 *
 * **NOTE:** A sheet is pushed to the sheet stack if and only if the sheet has
 * changed. If a function is called that is defined in the same sheet, this
 * doesn't count.
 *
 * \typedef struct _debugSession DebugSession
 */
typedef struct _debugSession {
    DebugStackEntry sheetStack[DEBUG_SHEET_STACK_SIZE];

    DVM vm;
    DebugAgenda agenda;

    int stackPtr;
} DebugSession;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_debug_add_value_info(DebugInfo *debugInfo, size_t ins,
 *                                 InsValueInfo valueInfo)
 * \brief Add value information to a list of debug information.
 *
 * \param debugInfo The debug info to add the value info to.
 * \param valueInfo The value info to add.
 */
DECISION_API void d_debug_add_value_info(DebugInfo *debugInfo, size_t ins,
                                         InsValueInfo valueInfo);

/**
 * \fn void d_debug_add_exec_info(DebugInfo *debugInfo, size_t ins,
 *                                InsExecInfo execInfo)
 * \brief Add execution information to a list of debug information.
 *
 * \param debugInfo The debug into to add the execution info to.
 * \param execInfo The execution info to add.
 */
DECISION_API void d_debug_add_exec_info(DebugInfo *debugInfo, size_t ins,
                                        InsExecInfo execInfo);

/**
 * \fn void d_debug_add_node_info(DebugInfo *debugInfo, size_t ins,
 *                                InsNodeInfo nodeInfo)
 * \brief Add node information to a list of debug information.
 *
 * \param debugInfo The debug info to add the node info to.
 * \param nodeInfo The node info to add.
 */
DECISION_API void d_debug_add_node_info(DebugInfo *debugInfo, size_t ins,
                                        InsNodeInfo nodeInfo);

/**
 * \fn void d_debug_add_call_info(DebugInfo *debugInfo, size_t ins,
 *                                InsCallInfo callInfo)
 * \brief Add call information to a list of debug information.
 *
 * \param debugInfo The debug info to add the call info to.
 * \param callInfo The call info to add.
 */
DECISION_API void d_debug_add_call_info(DebugInfo *debugInfo, size_t ins,
                                        InsCallInfo callInfo);

/**
 * \fn void d_debug_dump_info(DebugInfo debugInfo)
 * \brief Dump the debugging information to `stdout`.
 *
 * \param debugInfo The debugging info to dump.
 */
DECISION_API void d_debug_dump_info(DebugInfo debugInfo);

/**
 * \fn void d_debug_free_info(DebugInfo *debugInfo)
 * \brief Free debugging information.
 *
 * \param debugInfo The debugging info to free.
 */
DECISION_API void d_debug_free_info(DebugInfo *debugInfo);

/**
 * \fn DebugSession d_debug_create_session(Sheet *sheet, DebugAgenda agenda)
 * \brief Create a debugging session.
 *
 * \return A debugging session in it's starting state.
 *
 * \param sheet The sheet to debug.
 * \param agenda The agenda the session should use.
 */
DECISION_API DebugSession d_debug_create_session(struct _sheet *sheet,
                                                 DebugAgenda agenda);

/**
 * \fn void d_debug_continue_session(DebugSession *session)
 * \brief Continue a debugging session until either a breakpoint is hit, or the
 * VM halts.
 *
 * \param session The session to continue.
 */
DECISION_API void d_debug_continue_session(DebugSession *session);

/**
 * \fn void d_debug_stop_session(DebugSession *session)
 * \brief Stop a debugging session, freeing all of the memory malloc'd by the
 * session. The session should not be used afterwards.
 *
 * \param session The session to stop.
 */
DECISION_API void d_debug_stop_session(DebugSession *session);

#endif // DDEBUG_H