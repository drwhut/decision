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
 * \file dsheet.h
 * \brief This header contains definitions for Sheets in Decision.
 */

#ifndef DSHEET_H
#define DSHEET_H

#include "dcfg.h"
#include "dcfunc.h"
#include "ddebug.h"
#include "dgraph.h"
#include "dlink.h"
#include <stdbool.h>

#include <stddef.h>

/*
=== HEADER DEFINITIONS ====================================
*/

/* Forward declaration of the Sheet struct from later on. */
struct _sheet;

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
 * \struct _sheetVariable
 * \brief A struct for storing variable data.
 *
 * \typedef struct _sheetVariable SheetVariable.
 */
typedef struct _sheetVariable {
    const NodeDefinition getterDefinition; ///< The definition of the variable's
                                           ///< getter node. This is determined
                                           ///< automatically when a variable
                                           ///< is added to a sheet.

    const SocketMeta variableMeta; ///< The variable metadata, i.e. it's name,
                                   ///< it's type, etc.

    struct _sheet *sheet; ///< The sheet the variable belongs to.
} SheetVariable;

/**
 * \struct _sheetFunction
 * \brief A struct for storing function data.
 * 
 * Note that if you want to know if the function is a subroutine, you can use
 * `d_is_subroutine`.
 *
 * \typedef struct _sheetFunction SheetFunction.
 */
typedef struct _sheetFunction {
    const NodeDefinition functionDefinition; ///< The definition of the
                                             ///< function, i.e. it's name,
                                             ///< it's sockets, etc.

    const NodeDefinition defineDefinition; ///< The definition of the function's
                                           ///< Define node. This is determined
                                           ///< automatically when adding a
                                           ///< function to a sheet.

    const NodeDefinition returnDefinition; ///< The definition of the function's
                                           ///< Return node. This is determined
                                           ///< automatically when adding a
                                           ///< function to a sheet.

    size_t defineNodeIndex; ///< Used in Semantic Analysis.
    size_t numDefineNodes;  ///< Used in Semantic Analysis.

    size_t lastReturnNodeIndex; ///< Used in Semantic Analysis.
    size_t numReturnNodes;      ///< Used in Semantic Analysis.

    struct _sheet *sheet; ///< The sheet the function belongs to.
} SheetFunction;

/**
 * \struct _sheet
 * \brief A struct for storing sheet data.
 *
 * \typedef struct _sheet Sheet
 */
typedef struct _sheet {
    Graph graph; ///< Can be empty if the sheet came from a Decision object
                 ///< file.

    DebugInfo _debugInfo; ///< If the sheet is compiled in debug mode, this
                          ///< will contain debugging information.

    LinkMetaList _link; ///< What can be linked in this sheet?

    const char *filePath; ///< Essentially the name of the sheet.

    const char *includePath; ///< i.e. what was the argument of the Include
                             ///< property that included this sheet. Default
                             ///< value is `NULL`.

    struct _sheet **includes; ///< The list of included sheets.
    size_t numIncludes;       ///< The number of included sheets.

    SheetVariable *variables; ///< The list of variables defined in this sheet.
    size_t numVariables;      ///< The number of variables in this sheet.
    SheetFunction *functions; ///< The list of functions defined in this sheet.
    size_t numFunctions;      ///< The number of functions in this sheet.
    CFunction *cFunctions;    ///< The list of C functions defined in this
                              ///< sheet.
    size_t numCFunctions;     ///< The number of C functions in this sheet.

    size_t _main; ///< Points to the index of the first instruction of Start,
                  ///< *not* the `RET` instruction one before.

    char *_text;      ///< The compiled bytecode.
    size_t _textSize; ///< The number of bytes the compiled bytecode has.
    char *_data;      ///< The compiled data section.
    size_t _dataSize; ///< The number of bytes the data section has.

    InstructionToLink *_insLinkList; ///< A list of which instructions should
                                     ///< link to which items.
    size_t _insLinkListSize;         ///< The number of instructions to link.

    size_t numStarts;   ///< Used by Semantic Analysis.
    int startNodeIndex; ///< If this value is `-1`, then no Start node exists.

    bool hasErrors; ///< If true, the sheet cannot be run.

    bool allowFree; ///< Allow sheets that include this sheet to free it?

    bool _isCompiled; ///< Has the sheet been compiled?
    bool _isLinked;   ///< Has the sheet been linked?

} Sheet;

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta)
 * \brief Add a variable property to the sheet.
 *
 * \param sheet The sheet to add the variable onto.
 * \param varMeta The variable metadata to add.
 */
DECISION_API void d_sheet_add_variable(Sheet *sheet, const SocketMeta varMeta);

/**
 * \fn void d_sheet_add_function(Sheet *sheet, const NodeDefinition funcDef)
 * \brief Add a function to a sheet.
 *
 * \param sheet The sheet to add the function to.
 * \param funcDef The function definition to add.
 */
DECISION_API void d_sheet_add_function(Sheet *sheet,
                                       const NodeDefinition funcDef);

/**
 * \fn void d_sheet_add_c_function(Sheet *sheet, CFunction cFunction)
 * \brief Add a C function to a sheet.
 *
 * **NOTE:** To create a C function, have a look at `dcfunc.h`.
 *
 * \param sheet The sheet to add the C function to.
 * \param cFunction The C function to add.
 */
DECISION_API void d_sheet_add_c_function(Sheet *sheet, CFunction cFunction);

/**
 * \fn bool d_is_subroutine(SheetFunction func)
 * \brief Is the given function a subroutine?
 *
 * \return If the function is a subroutine.
 *
 * \param func The function to query.
 */
DECISION_API bool d_is_subroutine(SheetFunction func);

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
 *                                          const char *includePath,
 *                                          Sheet **priors,
 *                                          bool debugInclude)
 * \brief Add a reference to another sheet to the current sheet, which can be
 * used to get extra functionality.
 *
 * \return A pointer to the sheet that was created from the include path.
 *
 * \param sheet The sheet to add the include to.
 * \param includePath The path from sheet to the sheet being included.
 * Note that this should be equivalent to the argument of the Include property.
 * \param priors A NULL-terminated list of sheets that, if included, will throw
 * an error. This is to prevent circular includes.
 * \param debugInclude If we can compile the included sheet in debug mode,
 * do so if set to true.
 */
DECISION_API Sheet *d_sheet_add_include_from_path(Sheet *sheet,
                                                  const char *includePath,
                                                  Sheet **priors,
                                                  bool debugInclude);

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
 * **NOTE:** This will also free all included sheets recursively that have
 * the `allowFree` property set to `true`, which is the default!
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
 * \fn void d_c_functions_dump(CFunction *functions, size_t numFunctions)
 * \brief Dump the details of an array of C functions to `stdout`.
 *
 * \param functions The array of C functions.
 * \param numFunctions The number of functions in the array.
 */
DECISION_API void d_c_functions_dump(CFunction *functions, size_t numFunctions);

/**
 * \fn void d_sheet_dump(Sheet *sheet)
 * \brief Dump the contents of a `Sheet` struct to `stdout`.
 *
 * \param sheet The sheet to dump.
 */
DECISION_API void d_sheet_dump(Sheet *sheet);

#endif // DSHEET_H
