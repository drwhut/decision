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

/**
 * \file dsheet.h
 * \brief This header contains definitions for Sheets in Decision.
 */

#ifndef DSHEET_H
#define DSHEET_H

#include <stdbool.h>
#include "dcfg.h"
#include "dlex.h"
#include "dlink.h"
#include "dsemantic.h"
#include "dtype.h"
#include "dvm.h"

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from later on. */
struct _sheet;

/* Forward declaration of the SheetNode struct from later on. */
struct _sheetNode;

/**
 * \struct _insToLink
 * \brief A struct to describe which instructions needs to be replaced with
 * linked pointers.
 *
 * \typedef struct _insToLink InstructionToLink
 */
typedef struct _insToLink {
    size_t ins;  ///< Index of the LOADUI instruction in the text section.
    size_t link; ///< Index of the LinkMeta structure in the LinkMetaList.
} InstructionToLink;

/**
 * \struct _sheetSocket
 * \brief A struct for storing sockets.
 *
 * \typedef struct _sheetSocket SheetSocket
 */
typedef struct _sheetSocket {
    DType type;
    LexData defaultValue; ///< If there is no input wire, use a given value.
    bool isInput;         ///< If it's an input socket, it can only have up to 1
                          ///< connection.
    struct _sheetNode *node; ///< The node we're a part of.
    struct _sheetSocket **connections;
    size_t numConnections;

    reg_t _reg; ///< Used in code generation.
} SheetSocket;

/**
 * \struct _sheetNode
 * \brief A struct for storing node data.
 *
 * \typedef struct _sheetNode SheetNode
 */
typedef struct _sheetNode {
    const char *name;
    size_t lineNum;
    SheetSocket **sockets;
    size_t numSockets;
    bool isExecution;
    struct _sheet *sheet; ///< The sheet we're a part of.

    NameDefinition definition; ///< * If the node is the getter or setter of a
                               ///< variable, it states where the variable is
                               ///< defined.
                               ///< * Else, if the node is either `Define` or
                               ///< `Return`, it states wherer the function
                               ///< being defined or returned from is defined
                               ///< (should be on the same sheet).
                               ///< * Otherwise, it states where the function
                               ///< being called is defined from.
} SheetNode;

/**
 * \struct _sheetVariable
 * \brief A struct for storing variable data.
 *
 * \typedef struct _sheetVariable SheetVariable.
 */
typedef struct _sheetVariable {
    const char *name;
    DType dataType;
    LexData defaultValue;
    struct _sheet *sheet; ///< The sheet we're defined in.
} SheetVariable;

/**
 * \struct _sheetFunction
 * \brief A struct for storing function data.
 *
 * \typedef struct _sheetFunction SheetFunction.
 */
typedef struct _sheetFunction {
    const char *name;
    bool isSubroutine;

    SheetVariable *arguments;
    size_t numArguments;
    SheetVariable *returns;
    size_t numReturns;

    struct _sheet *sheet; ///< The sheet we're defined in.

    struct _sheetNode *defineNode; ///< Used in Semantic Analysis.
    size_t numDefineNodes;         ///< Used in Semantic Analysis.

    struct _sheetNode *lastReturnNode; ///< Used in Semantic Analysis.
    size_t numReturnNodes;             ///< Used in Semantic Analysis.
} SheetFunction;

/**
 * \struct _sheet
 * \brief A struct for storing sheet data.
 *
 * \typedef struct _sheet Sheet
 */
typedef struct _sheet {
    const char *filePath;
    const char *includePath; ///< i.e. what was the argument of the Include
                             ///< property that included this sheet. Default
                             ///< value is `NULL`.
    bool hasErrors;

    bool _isCompiled;
    bool _isLinked;

    struct _sheet **includes;
    size_t numIncludes;

    SheetVariable *variables;
    size_t numVariables;
    SheetFunction *functions;
    size_t numFunctions;

    SheetNode **nodes;
    size_t numNodes;

    SheetNode *startNode;
    size_t numStarts;
    size_t _main; ///< Points to the index of the first instruction of Start,
                  ///< *not* the `RET` instruction one before.

    char *_text;
    size_t _textSize;
    char *_data;
    size_t _dataSize;

    LinkMetaList _link;
    InstructionToLink *_insLinkList;
    size_t _insLinkListSize;

} Sheet;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_socket_add_connection(SheetSocket *socket,
 *                                  SheetSocket *connection, Sheet *sheet,
 *                                  SheetNode *node)
 * \brief Add a connection to a socket. If sheet or node are `NULL`, it will
 * not display any errors if they occur.
 *
 * \param socket The socket to add the connection to.
 * \param connection The connection to add.
 * \param sheet In case we error, say what sheet we errored on.
 * \param node In case we error, say what node caused the error.
 */
DECISION_API void d_socket_add_connection(SheetSocket *socket,
                                          SheetSocket *connection, Sheet *sheet,
                                          SheetNode *node);

/**
 * \fn void d_socket_connect(SheetSocket *from, SheetSocket *to, Sheet *sheet,
 *                           SheetNode *nodeTo)
 * \brief Connect 2 sockets together with a wire.
 *
 * \param from Where the connection is from. `from->isInput` must be `false`.
 * \param to Where the connection goes to. `from->isInput` must be `true.`
 * \param sheet In case we error, say what sheet we errored from.
 * \param nodeTo In case we error, say what node caused the error.
 */
DECISION_API void d_socket_connect(SheetSocket *from, SheetSocket *to,
                                   Sheet *sheet, SheetNode *nodeTo);

/**
 * \fn SheetSocket *d_socket_create(DType dataType, LexData defaultValue,
 *                                  bool isInput)
 * \brief Create a socket, with no initial connections.
 *
 * \return A malloc'd socket with no connections.
 *
 * \param dataType The data type of the socket.
 * \param defaultValue If there is no input connection, use this value as input.
 * \param isInput Is the socket an input socket? This determines how many
 * connections it can have.
 */
DECISION_API SheetSocket *d_socket_create(DType dataType, LexData defaultValue,
                                          bool isInput);

/**
 * \fn void d_socket_free(SheetSocket *socket)
 * \brief Free a malloc'd socket.
 *
 * \param socket The socket to free.
 */
DECISION_API void d_socket_free(SheetSocket *socket);

/**
 * \fn void d_node_add_socket(SheetNode *node, SheetSocket *socket)
 * \brief Add a socket definition to a node.
 *
 * \param node The node to add the socket onto.
 * \param socket The socket definition to add.
 */
DECISION_API void d_node_add_socket(SheetNode *node, SheetSocket *socket);

/**
 * \fn void d_node_free(SheetNode *node)
 * \brief Free a malloc'd node.
 *
 * \param node The node to free.
 */
DECISION_API void d_node_free(SheetNode *node);

/**
 * \fn SheetNode *d_node_create(const char *name, size_t lineNum,
 *                              bool isExecution)
 * \brief Create a node with no sockets by default.
 *
 * \return A malloc'd node with no sockets.
 *
 * \param name The name of the node, i.e. the function or variable name.
 * \param lineNum The line number the node was defined on.
 * \param isExecution Is the node an execution node?
 */
DECISION_API SheetNode *d_node_create(const char *name, size_t lineNum,
                                      bool isExecution);

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, SheetVariable variable)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param variable The variable to add.
 */
DECISION_API void d_sheet_add_variable(Sheet *sheet, SheetVariable variable);

/**
 * \fn void d_sheet_free_variable(SheetVariable variable)
 * \brief Free malloc'd elements of a variable structure.
 *
 * \param variable The variable whose elements to free.
 */
DECISION_API void d_sheet_free_variable(SheetVariable variable);

/**
 * \fn void d_sheet_create_function(Sheet *sheet, const char *name,
 *                                  bool isSubroutine)
 * \brief Add a template function to a sheet, with no arguments or returns.
 *
 * \param sheet The sheet to add the function to.
 * \param name The name of the function.
 * \param isSubroutine Is the function we're adding a subroutine (execution)
 * function?
 */
DECISION_API void d_sheet_create_function(Sheet *sheet, const char *name,
                                          bool isSubroutine);

/**
 * \fn void d_sheet_function_add_argument(Sheet *sheet, const char *funcName,
 *                                        const char *argName, DType argType,
 *                                        LexData defaultValue)
 * \brief Add an argument to the last occurance of a sheet's function, i.e.
 * the one created last with the name `funcName`.
 *
 * \param sheet The sheet where the function lives.
 * \param funcName The name of the function to add the argument to.
 * \param argName The name of the argument, if any.
 * \param argType The data type(s) of the argument that are allowed.
 * \param defaultValue The default value of the argument if a value / wire is
 * not passed.
 */
DECISION_API void d_sheet_function_add_argument(Sheet *sheet,
                                                const char *funcName,
                                                const char *argName,
                                                DType argType,
                                                LexData defaultValue);

/**
 * \fn void d_sheet_function_add_return(Sheet *sheet, const char *funcName,
 *                                      const char *retName, DType retType)
 * \brief Add an return value to the last occurance of a sheet's function, i.e.
 * the one created last with the name funcName.
 *
 * \param sheet The sheet where the function lives.
 * \param funcName The name of the function to add the return value to.
 * \param retName The name of the return value, if any.
 * \param retType The data type(s) of the return value that are allowed.
 */
DECISION_API void d_sheet_function_add_return(Sheet *sheet,
                                              const char *funcName,
                                              const char *retName,
                                              DType retType);

/**
 * \fn void d_sheet_free_function(SheetFunction func)
 * \brief Free malloc'd elements of a function structure.
 *
 * \param func The function whose elements to free.
 */
DECISION_API void d_sheet_free_function(SheetFunction func);

/**
 * \fn void d_sheet_add_node(Sheet *sheet, SheetNode *node)
 * \brief Add a node to a sheet.
 *
 * \param sheet The sheet to add the node to.
 * \param node The node to add.
 */
DECISION_API void d_sheet_add_node(Sheet *sheet, SheetNode *node);

/**
 * \fn void d_sheet_add_include(Sheet *sheet, Sheet *include)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \param sheet The sheet to add the include to.
 * \param include The sheet to include.
 */
DECISION_API void d_sheet_add_include(Sheet *sheet, Sheet *include);

/**
 * \fn Sheet *d_sheet_add_include_from_path(Sheet *sheet,
 *                                          const char *includePath)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \return A pointer to the sheet that was created from the include path.
 *
 * \param sheet The sheet to add the include to.
 * \param includePath The path from sheet to the sheet being included.
 * Note that this should be equivalent to the argument of the Include property.
 */
DECISION_API Sheet *d_sheet_add_include_from_path(Sheet *sheet,
                                                  const char *includePath);

/**
 * \fn Sheet *d_sheet_create(const char *filePath)
 * \brief Create a malloc'd sheet object.
 *
 * \return The malloc'd sheet object.
 *
 * \param filePath The file where this sheet originated.
 */
DECISION_API Sheet *d_sheet_create(const char *filePath);

/**
 * \fn void d_sheet_free(Sheet *sheet)
 * \brief Free malloc'd memory in a sheet.
 *
 * \param sheet The sheet to free from memory.
 */
DECISION_API void d_sheet_free(Sheet *sheet);

/**
 * \fn void d_variables_dump(SheetVariable *variables, size_t numVariables)
 * \brief Dump the details of an array of variables to `stdout`.
 *
 * \param variables The array of variables.
 * \param numVariables The number of variables in the array.
 */
DECISION_API void d_variables_dump(SheetVariable *variables,
                                   size_t numVariables);

/**
 * \fn void d_functions_dump(SheetFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of functions to `stdout`.
 *
 * \param functions The array of functions.
 * \param numFunctions The number of functions in the array.
 */
DECISION_API void d_functions_dump(SheetFunction *functions,
                                   size_t numFunctions);

/**
 * \fn void d_sheet_dump(Sheet *sheet)
 * \brief Dump the contents of a `Sheet` struct to `stdout`.
 *
 * \param sheet The sheet to dump.
 */
DECISION_API void d_sheet_dump(Sheet *sheet);

#endif // DSHEET_H
