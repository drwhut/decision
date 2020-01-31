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

#include "dsheet.h"

#include "dcfunc.h"
#include "decision.h"
#include "derror.h"
#include "dmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    A macro for adding an item to a dynamic array.
    This exists because it happens a lot in this file.
*/
#define LIST_PUSH(array, arrayType, numCurrentItems, newItem)               \
    size_t newNumItems = (numCurrentItems) + 1;                             \
    if (newNumItems == 1)                                                   \
        array = (arrayType *)d_malloc(sizeof(arrayType));                   \
    else                                                                    \
        array =                                                             \
            (arrayType *)d_realloc(array, newNumItems * sizeof(arrayType)); \
    array[numCurrentItems++] = newItem;

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
void d_socket_add_connection(SheetSocket *socket, SheetSocket *connection,
                             Sheet *sheet, SheetNode *node) {
    LIST_PUSH(socket->connections, SheetSocket *, socket->numConnections,
              connection);

    const char *filePath = NULL;
    size_t lineNum       = 0;

    if (sheet != NULL && node != NULL) {
        filePath = sheet->filePath;
        lineNum  = node->lineNum;
    }

    // Build up a string of the connection's line numbers, in case we error.
    char connLineNums[MAX_ERROR_SIZE];

    size_t lineNumIndex = 0;

    for (size_t i = 0; i < socket->numConnections; i++) {
        SheetSocket *conn  = socket->connections[i];
        size_t connLineNum = conn->node->lineNum;

        if (i > 0) {
#ifdef DECISION_SAFE_FUNCTIONS
            sprintf_s(connLineNums + lineNumIndex,
                      MAX_ERROR_SIZE - lineNumIndex, ", ");
#else
            sprintf(connLineNums + lineNumIndex, ", ");
#endif // DECISION_SAFE_FUNCTIONS
            lineNumIndex += 2;

            if (lineNumIndex >= MAX_ERROR_SIZE) {
                break;
            }
        }

#ifdef DECISION_SAFE_FUNCTIONS
        sprintf_s(connLineNums + lineNumIndex, MAX_ERROR_SIZE - lineNumIndex,
                  "%zu", connLineNum);
#else
        sprintf(connLineNums + lineNumIndex, "%zu", connLineNum);
#endif // DECISION_SAFE_FUNCTIONS

        while (connLineNums[lineNumIndex] != 0 &&
               lineNumIndex < MAX_ERROR_SIZE) {
            lineNumIndex++;
        }

        if (lineNumIndex >= MAX_ERROR_SIZE) {
            break;
        }
    }

    // If the socket is non-execution, an input socket, and we have more than
    // one connection...
    if (socket->numConnections > 1 && socket->type != TYPE_EXECUTION &&
        socket->isInput) {
        ERROR_COMPILER(
            filePath, lineNum, true,
            "Input non-execution socket on line %zu has more than one "
            "connection (has %zu, on lines %s)",
            socket->node->lineNum, socket->numConnections, connLineNums);
    }

    // If the socket is execution, an output socket, and we have more than one
    // connection...
    else if (socket->numConnections > 1 && socket->type == TYPE_EXECUTION &&
             !socket->isInput) {
        ERROR_COMPILER(
            filePath, lineNum, true,
            "Output execution socket on line %zu has more than one connection "
            "(has %zu, on lines %s)",
            socket->node->lineNum, socket->numConnections, connLineNums);
    }
}

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
void d_socket_connect(SheetSocket *from, SheetSocket *to, Sheet *sheet,
                      SheetNode *nodeTo) {
    if (!from->isInput && to->isInput) {
        d_socket_add_connection(from, to, sheet, nodeTo);
        d_socket_add_connection(to, from, sheet, nodeTo);

        // We need to check that the data types of both ends are the same!
        if ((from->type & to->type) == TYPE_NONE) {

            // Find out information about the sockets before giving the error.
            // NOTE: If you change it so that sockets are stored directly in
            // their sockets, CHANGE THIS FUNCTION!
            size_t outIndex      = 0;
            size_t numOutSockets = 0;

            for (size_t i = 0; i < from->node->numSockets; i++) {
                if (!from->node->sockets[i]->isInput) {
                    numOutSockets++;

                    if (from == from->node->sockets[i]) {
                        outIndex = numOutSockets;
                    }
                }
            }

            size_t inIndex      = 0;
            size_t numInSockets = 0;

            for (size_t i = 0; i < to->node->numSockets; i++) {
                if (to->node->sockets[i]->isInput) {
                    numInSockets++;

                    if (to == to->node->sockets[i]) {
                        inIndex = numInSockets;
                    }
                }
            }

            ERROR_COMPILER(sheet->filePath, nodeTo->lineNum, true,
                           "Wire data type mismatch between socket of type %s "
                           "(Output %zu/%zu of node %s on line %zu) and socket "
                           "of type %s (Input %zu/%zu of node %s on line %zu)",
                           d_type_name(from->type), outIndex, numOutSockets,
                           from->node->name, from->node->lineNum,
                           d_type_name(to->type), inIndex, numInSockets,
                           to->node->name, to->node->lineNum);
        }
    }
}

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
SheetSocket *d_socket_create(DType dataType, LexData defaultValue,
                             bool isInput) {
    SheetSocket *socket = (SheetSocket *)d_malloc(sizeof(SheetSocket));

    socket->type           = dataType;
    socket->defaultValue   = defaultValue;
    socket->isInput        = isInput;
    socket->node           = NULL;
    socket->connections    = NULL;
    socket->numConnections = 0;

    socket->_reg = 0;

    return socket;
}

/**
 * \fn void d_socket_free(SheetSocket *socket)
 * \brief Free a malloc'd socket.
 *
 * \param socket The socket to free.
 */
void d_socket_free(SheetSocket *socket) {
    // If the default value is a string (or the name of something), free it.
    if ((socket->type == TYPE_STRING || socket->type == TYPE_NAME) &&
        socket->defaultValue.stringValue != NULL) {
        free(socket->defaultValue.stringValue);
    }

    if (socket->connections != NULL) {
        free(socket->connections);
    }

    free(socket);
}

/**
 * \fn void d_node_add_socket(SheetNode *node, SheetSocket *socket)
 * \brief Add a socket definition to a node.
 *
 * \param node The node to add the socket onto.
 * \param socket The socket definition to add.
 */
void d_node_add_socket(SheetNode *node, SheetSocket *socket) {
    LIST_PUSH(node->sockets, SheetSocket *, node->numSockets, socket);
    socket->node = node;
}

/**
 * \fn void d_node_free(SheetNode *node)
 * \brief Free a malloc'd node.
 *
 * \param node The node to free.
 */
void d_node_free(SheetNode *node) {
    if (node->sockets != NULL) {
        for (size_t i = 0; i < node->numSockets; i++) {
            SheetSocket *socket = node->sockets[i];
            d_socket_free(socket);
        }

        free(node->sockets);
    }

    free((char *)node->name);
    free(node);
}

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
SheetNode *d_node_create(const char *name, size_t lineNum, bool isExecution) {
    SheetNode *node = (SheetNode *)d_malloc(sizeof(SheetNode));

    node->name        = name;
    node->lineNum     = lineNum;
    node->sockets     = NULL;
    node->numSockets  = 0;
    node->isExecution = isExecution;
    node->definition  = (NameDefinition){NULL, -1, -1, NULL, NULL};

    return node;
}

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, SheetVariable variable)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param variable The variable to add.
 */
void d_sheet_add_variable(Sheet *sheet, SheetVariable variable) {
    if (variable.name != NULL && (int)variable.dataType != -1) {
        // Set the variable's parent sheet pointer.
        variable.sheet = sheet;

        LIST_PUSH(sheet->variables, SheetVariable, sheet->numVariables,
                  variable);
    }
}

/**
 * \fn void d_sheet_free_variable(SheetVariable variable)
 * \brief Free malloc'd elements of a variable structure.
 *
 * \param variable The variable whose elements to free.
 */
void d_sheet_free_variable(SheetVariable variable) {
    // If the variable's default value is a string, free it.
    if (variable.dataType == TYPE_STRING &&
        variable.defaultValue.stringValue != NULL) {
        free(variable.defaultValue.stringValue);
    }

    // Free the variable name.
    free((char *)variable.name);
}

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
void d_sheet_create_function(Sheet *sheet, const char *name,
                             bool isSubroutine) {
    SheetFunction func;
    func.name           = name;
    func.isSubroutine   = isSubroutine;
    func.arguments      = NULL;
    func.numArguments   = 0;
    func.returns        = NULL;
    func.numReturns     = 0;
    func.sheet          = sheet;
    func.defineNode     = NULL;
    func.numDefineNodes = 0;
    func.lastReturnNode = NULL;
    func.numReturnNodes = 0;

    LIST_PUSH(sheet->functions, SheetFunction, sheet->numFunctions, func);
}

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
void d_sheet_function_add_argument(Sheet *sheet, const char *funcName,
                                   const char *argName, DType argType,
                                   LexData defaultValue) {
    SheetFunction *func = sheet->functions;
    SheetFunction *last = NULL;
    bool foundFunc      = false;

    for (size_t i = 0; i < sheet->numFunctions; i++) {
        if (strcmp(funcName, func->name) == 0) {
            foundFunc = true;
            last      = func;
        }

        func++;
    }

    if (foundFunc && last != NULL) {
        SheetVariable arg;
        arg.name         = argName;
        arg.dataType     = argType;
        arg.defaultValue = defaultValue;

        LIST_PUSH(last->arguments, SheetVariable, last->numArguments, arg);
    }
}

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
void d_sheet_function_add_return(Sheet *sheet, const char *funcName,
                                 const char *retName, DType retType) {
    SheetFunction *func = sheet->functions;
    SheetFunction *last = NULL;
    bool foundFunc      = false;

    for (size_t i = 0; i < sheet->numFunctions; i++) {
        if (strcmp(funcName, func->name) == 0) {
            foundFunc = true;
            last      = func;
        }

        func++;
    }

    if (foundFunc && last != NULL) {
        SheetVariable ret;
        ret.name                      = retName;
        ret.dataType                  = retType;
        ret.defaultValue.integerValue = 0;

        LIST_PUSH(last->returns, SheetVariable, last->numReturns, ret);
    }
}

/**
 * \fn void d_sheet_free_function(SheetFunction func)
 * \brief Free malloc'd elements of a function structure.
 *
 * \param func The function whose elements to free.
 */
void d_sheet_free_function(SheetFunction func) {
    // TODO: Since below we are freeing SheetVariables, shouldn't we free them
    // with d_sheet_free_variable?

    if (func.arguments != NULL) {
        free(func.arguments);
    }

    if (func.returns != NULL) {
        free(func.returns);
    }
}

/**
 * \fn void d_sheet_add_node(Sheet *sheet, SheetNode *node)
 * \brief Add a node to a sheet.
 *
 * \param sheet The sheet to add the node to.
 * \param node The node to add.
 */
void d_sheet_add_node(Sheet *sheet, SheetNode *node) {
    LIST_PUSH(sheet->nodes, SheetNode *, sheet->numNodes, node);

    // Let the node know what sheet it's a part of.
    node->sheet = sheet;
}

/**
 * \fn void d_sheet_add_include(Sheet *sheet, Sheet *include)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \param sheet The sheet to add the include to.
 * \param include The sheet to include.
 */
void d_sheet_add_include(Sheet *sheet, Sheet *include) {
    LIST_PUSH(sheet->includes, Sheet *, sheet->numIncludes, include);
}

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
Sheet *d_sheet_add_include_from_path(Sheet *sheet, const char *includePath) {
    Sheet *includeSheet = NULL;

    // TODO: Implement standard library paths as well.

    // If the current sheet was accessed from a different directory,
    // we need to stick to that directory for this include as well.
    // i.e. If we ran: decision ../main.dc, we need to include ../include.dc.
    // If main.dc and include.dc were in the same directory.

    // Copy the file path of the current sheet.
    const size_t filePathLen = strlen(sheet->filePath);
    char *dir                = (char *)d_malloc(filePathLen + 1);
    memcpy(dir, sheet->filePath, filePathLen + 1);

    // Find the last / or \ character.
    long lastSeperator = (long)filePathLen - 1;

    for (; lastSeperator >= 0; lastSeperator--) {
        if (dir[lastSeperator] == '/' || dir[lastSeperator] == '\\') {
            dir[lastSeperator + 1] = 0; // While we're here, put a NULL in front
                                        // of the last seperator.
            break;
        }
    }

    if ((int)lastSeperator >= 0) {
        // Concatenate the dir string (with the NULL inserted) with the contents
        // of the literal string.
        const size_t newPathLength =
            (size_t)lastSeperator + 1 + strlen(includePath);
        dir = (char *)d_realloc(dir, newPathLength + 1);

#ifdef DECISION_SAFE_FUNCTIONS
        strcat_s(dir, newPathLength + 1, includePath);
#else
        strcat(dir, includePath);
#endif // DECISION_SAFE_FUNCTIONS

        includeSheet = d_load_file((const char *)dir);
    }
    // If there isn't either character, we don't need to worry about
    // changing the directory.
    else {
        includeSheet = d_load_file(includePath);
    }

    d_sheet_add_include(sheet, includeSheet);

    free(dir);

    // Set the includePath property of the included sheet, as we need to save
    // that value, instead of the directory, for if we run the sheet including
    // this sheet from a different working directory.
    const size_t includePathLen = strlen(includePath);
    char *cpyIncludePath        = (char *)d_malloc(includePathLen + 1);
    memcpy(cpyIncludePath, includePath, includePathLen + 1);

    includeSheet->includePath = (const char *)cpyIncludePath;

    return includeSheet;
}

/**
 * \fn Sheet *d_sheet_create(const char *filePath)
 * \brief Create a malloc'd sheet object.
 *
 * \return The malloc'd sheet object.
 *
 * \param filePath The file where this sheet originated.
 */
Sheet *d_sheet_create(const char *filePath) {
    Sheet *sheet = (Sheet *)d_malloc(sizeof(Sheet));

    // Copy the file path string into heap memory so we know for sure we can
    // free it later.
    size_t filePathLen = strlen(filePath);
    char *cpyFilePath  = (char *)d_malloc(filePathLen + 1);
    memcpy(cpyFilePath, filePath, filePathLen + 1);

    sheet->filePath         = (const char *)cpyFilePath;
    sheet->includePath      = NULL;
    sheet->hasErrors        = false;
    sheet->_isCompiled      = false;
    sheet->_isLinked        = false;
    sheet->includes         = NULL;
    sheet->numIncludes      = 0;
    sheet->variables        = NULL;
    sheet->numVariables     = 0;
    sheet->functions        = NULL;
    sheet->numFunctions     = 0;
    sheet->nodes            = NULL;
    sheet->numNodes         = 0;
    sheet->startNode        = NULL;
    sheet->numStarts        = 0;
    sheet->_main            = 0;
    sheet->_text            = NULL;
    sheet->_textSize        = 0;
    sheet->_data            = NULL;
    sheet->_dataSize        = 0;
    sheet->_link            = d_link_new_meta_list();
    sheet->_insLinkList     = NULL;
    sheet->_insLinkListSize = 0;

    return sheet;
}

/**
 * \fn void d_sheet_free(Sheet *sheet)
 * \brief Free malloc'd memory in a sheet.
 *
 * \param sheet The sheet to free from memory.
 */
void d_sheet_free(Sheet *sheet) {
    if (sheet != NULL) {
        // Dereferenced sheet here so VS stops giving us a warning about this
        // one line.
        Sheet sheetDeref = *sheet;
        free((char *)sheetDeref.filePath);

        if (sheet->includePath != NULL)
            free((char *)sheet->includePath);

        if (sheet->variables != NULL) {
            for (size_t i = 0; i < sheet->numVariables; i++) {
                SheetVariable variable = sheet->variables[i];
                d_sheet_free_variable(variable);
            }

            free(sheet->variables);
        }

        if (sheet->nodes != NULL) {
            for (size_t i = 0; i < sheet->numNodes; i++) {
                SheetNode *node = sheet->nodes[i];
                d_node_free(node);
            }

            free(sheet->nodes);
        }

        if (sheet->functions != NULL) {
            for (size_t i = 0; i < sheet->numFunctions; i++) {
                SheetFunction func = sheet->functions[i];
                d_sheet_free_function(func);
            }

            free(sheet->functions);
        }

        if (sheet->includes != NULL) {
            for (size_t i = 0; i < sheet->numIncludes; i++) {
                Sheet *include = sheet->includes[i];
                d_sheet_free(include);
            }

            free(sheet->includes);
        }

        if (sheet->_text != NULL)
            free(sheet->_text);

        // Before we free the data section, we need to free any string variables
        // that will have been malloc'd. These pointers should only be malloc'd
        // when linking has taken place.
        if (sheet->_isLinked) {
            for (size_t i = 0; i < sheet->_link.size; i++) {
                LinkMeta meta = sheet->_link.list[i];

                if (meta.type == LINK_VARIABLE_POINTER) {
                    SheetVariable *var = (SheetVariable *)meta.meta;
                    Sheet *extSheet    = var->sheet;

                    // We can only free it if it is from our sheet.
                    if (sheet == extSheet) {
                        char *varLoc = (char *)sheet->_data + (size_t)meta._ptr;

                        // We needed to convert it from a char* so it shifted
                        // the corrent amount.
                        char *strPtr = *((char **)varLoc);

                        // Make sure it doesn't point to somewhere else in the
                        // .data section, we can't free it then!
                        if (strPtr < sheet->_data ||
                            strPtr >= sheet->_data + (size_t)sheet->_dataSize) {
                            if (strPtr != NULL) {
                                free(strPtr);
                            }
                        }
                    }
                }
            }
        }

        if (sheet->_data != NULL)
            free(sheet->_data);

        d_link_free_list(&(sheet->_link));

        if (sheet->_insLinkList != NULL)
            free(sheet->_insLinkList);

        free(sheet);
    }
}

/**
 * \fn void d_variables_dump(SheetVariable *variables, size_t numVariables)
 * \brief Dump the details of an array of variables to `stdout`.
 *
 * \param variables The array of variables.
 * \param numVariables The number of variables in the array.
 */
void d_variables_dump(SheetVariable *variables, size_t numVariables) {
    printf("# Variables: %zu\n", numVariables);

    // Dump the variables, if there are any.
    if (variables != NULL && numVariables > 0) {
        for (size_t i = 0; i < numVariables; i++) {
            SheetVariable var = variables[i];

            printf("\tVariable %s is of type %d with default value ", var.name,
                   var.dataType);
            switch (var.dataType) {
                case TYPE_INT:
                    printf("%" DINT_PRINTF_d, var.defaultValue.integerValue);
                    break;
                case TYPE_FLOAT:
                    printf("%f", var.defaultValue.floatValue);
                    break;
                case TYPE_STRING:
                    printf("%s", var.defaultValue.stringValue);
                    break;
                case TYPE_BOOL:
                    printf("%d", var.defaultValue.booleanValue);
                    break;
                default:
                    break;
            }
            printf("\n");
        }
    }
}

/**
 * \fn void d_functions_dump(SheetFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of functions to `stdout`.
 *
 * \param functions The array of functions.
 * \param numFunctions The number of functions in the array.
 */
void d_functions_dump(SheetFunction *functions, size_t numFunctions) {
    printf("# Functions: %zu\n", numFunctions);

    if (functions != NULL && numFunctions > 0) {
        for (size_t i = 0; i < numFunctions; i++) {
            SheetFunction function = functions[i];

            printf(
                "\tFunction %s is %s with %zu arguments:\n", function.name,
                ((function.isSubroutine) ? "an EXECUTION" : "a NON-EXECUTION"),
                function.numArguments);

            if (function.arguments != NULL && function.numArguments > 0) {
                for (size_t j = 0; j < function.numArguments; j++) {
                    SheetVariable arg = function.arguments[j];

                    printf("\t\tArgument %s is of type %d with default value ",
                           arg.name, arg.dataType);
                    switch (arg.dataType) {
                        case TYPE_INT:
                            printf("%" DINT_PRINTF_d,
                                   arg.defaultValue.integerValue);
                            break;
                        case TYPE_FLOAT:
                            printf("%f", arg.defaultValue.floatValue);
                            break;
                        case TYPE_STRING:
                            printf("%s", arg.defaultValue.stringValue);
                            break;
                        case TYPE_BOOL:
                            printf("%d", arg.defaultValue.booleanValue);
                            break;
                        default:
                            break;
                    }
                    printf("\n");
                }
            }

            printf("\tand %zu returns:\n", function.numReturns);

            if (function.returns != NULL && function.numReturns > 0) {
                for (size_t j = 0; j < function.numReturns; j++) {
                    SheetVariable ret = function.returns[j];

                    printf("\t\tReturn %s is of type %d\n", ret.name,
                           ret.dataType);
                }
            }
        }
    }
}

/**
 * \fn void d_sheet_dump(Sheet *sheet)
 * \brief Dump the contents of a `Sheet` struct to `stdout`.
 *
 * \param sheet The sheet to dump.
 */
void d_sheet_dump(Sheet *sheet) {
    printf("\nSHEET %s DUMP\n", sheet->filePath);
    printf("# Start functions: %zu\n", sheet->numStarts);
    printf("# Includes: %zu\n", sheet->numIncludes);

    if (sheet->includes != NULL && sheet->numIncludes > 0) {
        for (size_t i = 0; i < sheet->numIncludes; i++) {
            printf("\t%s\n", sheet->includes[i]->filePath);
        }
    }

    // Dump the variables, if there are any.
    d_variables_dump(sheet->variables, sheet->numVariables);

    // Dump the functions, if there are any.
    d_functions_dump(sheet->functions, sheet->numFunctions);

    // Dump the nodes.
    printf("# Nodes: %zu\n", sheet->numNodes);

    if (sheet->nodes != NULL && sheet->numNodes > 0) {
        for (size_t i = 0; i < sheet->numNodes; i++) {
            SheetNode *node = sheet->nodes[i];

            printf(
                "Node #%zd named %s is %s node on line %zu with %zu sockets\n",
                i, node->name,
                (node->isExecution) ? "an execution" : "a non-execution",
                node->lineNum, node->numSockets);

            if (node->sockets != NULL && node->numSockets > 0) {
                for (size_t j = 0; j < node->numSockets; j++) {
                    SheetSocket *socket = node->sockets[j];

                    printf("\n\tSocket #%zd (%p) is of type %d, isInput = "
                           "%d\n\tConnections: ",
                           j, (void *)socket, socket->type, socket->isInput);

                    if (socket->connections != NULL &&
                        socket->numConnections > 0) {
                        for (size_t k = 0; k < socket->numConnections; k++) {
                            printf("%p ", (void *)socket->connections[k]);
                        }
                    }

                    printf("\n");
                }
            }

            // Variables to print to explain the definition.
            const char *sheetName = (node->definition.sheet != NULL)
                                        ? node->definition.sheet->filePath
                                        : "NULL";

            const char *varName = (node->definition.variable != NULL)
                                      ? node->definition.variable->name
                                      : "NULL";

            const char *funcName = (node->definition.function != NULL)
                                       ? node->definition.function->name
                                       : "NULL";

            const char *cFuncName = (node->definition.cFunction != NULL)
                                        ? node->definition.cFunction->name
                                        : "NULL";

            printf("\n\tDefinition:\n\t\tSheet: %s\n\t\tType: %d"
                   "\n\t\tCore Function: %d\n\t\tVariable: %s\n\t\tFunction: "
                   "%s\n\t\tC Function: %s\n",
                   sheetName, node->definition.type, node->definition.coreFunc,
                   varName, funcName, cFuncName);

            printf("\n");
        }
    }
}
