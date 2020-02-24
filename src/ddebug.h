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
#include "dvm.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

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

    OnNodeActivated onNodedActivated;
} DebugSession;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn DebugSession d_debug_create_session(Sheet *sheet,
 *                                         OnNodeActivated onNodeActivated)
 * \brief Create a debugging session.
 *
 * \return A debugging session in it's starting state.
 *
 * \param sheet The sheet to debug.
 * \param onNodeActivated A pointer to a function that is called when a
 * debuggable node is activated during the session. If NULL, the function is
 * not called.
 */
DECISION_API DebugSession
d_debug_create_session(struct _sheet *sheet, OnNodeActivated onNodeActivated);

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