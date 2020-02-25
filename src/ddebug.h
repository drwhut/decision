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

/**
 * \struct _insValueInfo
 * \brief Describes when a value wire has a value in bytecode.
 *
 * \typedef struct _insValueInfo InsValueInfo
 */
typedef struct _insValueInfo {
    size_t ins;
    size_t valueWire;
    int stackIndex;
} InsValueInfo;

/**
 * \struct _insExecInfo
 * \brief Describes when an execution wire gets activated in bytecode.
 *
 * \typedef struct _insExecInfo InsExecInfo
 */
typedef struct _insExecInfo {
    size_t ins;
    size_t execWire;
} InsExecInfo;

/**
 * \struct _insNodeInfo
 * \brief Describes when a node "starts" in bytecode.
 *
 * \typedef struct _insNodeInfo InsNodeInfo
 */
typedef struct _insNodeInfo {
    size_t ins;
    size_t node;
} InsNodeInfo;

/**
 * \struct _debugInfo
 * \brief A collection of info used for debugging.
 *
 * \typedef struct _debugInfo DebugInfo
 */
typedef struct _debugInfo {
    InsValueInfo *insValueInfoList;
    size_t insValueInfoSize;

    InsExecInfo *insExecInfoList;
    size_t insExecInfoSize;

    InsNodeInfo *insNodeInfoList;
    size_t insNodeInfoSize;
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
 * \struct _debugSession
 * \brief A struct which keeps track of a debugging session.
 *
 * \typedef struct _debugSession DebugSession
 */
typedef struct _debugSession {
    DVM vm;

    struct _sheet *sheet;

    OnWireValue onWireValue;
    OnExecutionWire onExecutionWire;
    OnNodeActivated onNodedActivated;
} DebugSession;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_debug_add_node_info(DebugInfo *debugInfo, InsNodeInfo nodeInfo)
 * \brief Add node information to a list of debug information.
 * 
 * \param debugInfo The debug info to add the node info to.
 * \param nodeInfo The node info to add.
 */
DECISION_API void d_debug_add_node_info(DebugInfo *debugInfo,
                                        InsNodeInfo nodeInfo);

/**
 * \fn void d_debug_free_info(DebugInfo *debugInfo)
 * \brief Free debugging information.
 *
 * \param debugInfo The debugging info to free.
 */
DECISION_API void d_debug_free_info(DebugInfo *debugInfo);

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
DECISION_API DebugSession d_debug_create_session(
    struct _sheet *sheet, OnWireValue onWireValue,
    OnExecutionWire onExecutionWire, OnNodeActivated onNodeActivated);

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