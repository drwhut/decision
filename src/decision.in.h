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
 * \file decision.h
 * \brief The main header for general use.
 *
 * **NOTE:** This file is generated from `decision.in.h` with **CMake**.
 */

#ifndef DECISION_H
#define DECISION_H

#include "dcfg.h"
#include <stdbool.h>

#include <stdio.h>

/*
=== HEADER DEFINITIONS ====================================
*/

// clang-format off

/**
 * \def DECISION_VERSION_MAJOR
 * \brief The major verion number.
 */
#define DECISION_VERSION_MAJOR @Decision_VERSION_MAJOR@

/**
 * \def DECISION_VERSION_MINOR
 * \brief The minor verion number.
 */
#define DECISION_VERSION_MINOR @Decision_VERSION_MINOR@

/**
 * \def DECISION_VERSION_PATCH
 * \brief The patch verion number.
 */
#define DECISION_VERSION_PATCH @Decision_VERSION_PATCH@

#define _STR(x)         #x
#define _VERSION_STR(x) _STR(x)

/**
 * \def DECISION_VERSION
 * \brief The current version of Decision.
 */
#define DECISION_VERSION \
    _VERSION_STR(DECISION_VERSION_MAJOR) "." \
    _VERSION_STR(DECISION_VERSION_MINOR) "." \
    _VERSION_STR(DECISION_VERSION_PATCH)

// clang-format on

/* A forward declaration of the Sheet struct from dsheet.h */
struct _sheet;

/* A forward declaration of the DVM struct from dvm.h */
struct _DVM;

/**
 * \struct _compileOptions
 * \brief A set of options for when a sheet is compiled.
 *
 * By default, there are no initial includes, and the sheet is not compiled in
 * debug mode.
 *
 * \typedef struct _compileOptions CompileOptions
 */
typedef struct _compileOptions {
    struct _sheet **includes; ///< A NULL-terminated array. Can itself be NULL
                              ///< to signify no initial includes.
    struct _sheet **priors;   ///< A NULL-terminated array. If the sheet
                              ///< includes a sheet already in this list,
                              ///< an error is thrown.
    bool debug;               ///< Compiles sheets with debug information.
                              ///< This allows for debugging the sheets,
                              ///< but sheets are not longer optimised.
                              ///< Note that compiled sheets do not store debug
                              ///< information, and thus cannot be debugged.
} CompileOptions;

/**
 * \def DEFAULT_COMPILE_OPTIONS
 * \brief The default compile options.
 */
#define DEFAULT_COMPILE_OPTIONS \
    (CompileOptions) {          \
        NULL, NULL, false       \
    }

/*
=== FUNCTIONS =============================================
*/

/**
 * \fn char d_get_verbose_level()
 * \brief Get the current verbose level.
 *
 * \return The verbose level. It will be a number between 0 and 5.
 */
DECISION_API char d_get_verbose_level();

/**
 * \fn void d_set_verbose_level(char level)
 * \brief Set the current verbose level.
 *
 * \param level The verbose level to set. If the number is bigger than 5,
 * then the verbose level is set to 5.
 */
DECISION_API void d_set_verbose_level(char level);

/**
 * \def VERBOSE(level, ...)
 * \brief A macro to print verbose information depending on the level.
 *
 * The arguments after `level` are fed into a `printf` statement.
 *
 * If the level given is greater than or equal to the current verbose level,
 * the message will be printed. Otherwise, it will not be printed.
 *
 * \param level The verbose level at which to start displaying the message.
 */
#define VERBOSE(level, ...)               \
    if (d_get_verbose_level() >= (level)) \
        printf(__VA_ARGS__);

/**
 * \fn bool d_run_sheet(Sheet *sheet)
 * \brief Run the code in a given sheet, given that it has gone through
 * `d_codegen_compile`, and it has exactly one defined `Start` function.
 *
 * \return If the sheet ran without any errors.
 *
 * \param sheet The sheet to run.
 */
DECISION_API bool d_run_sheet(struct _sheet *sheet);

/**
 * \fn bool d_run_function(DVM *vm, Sheet *sheet, const char *funcName)
 * \brief Run the specified function/subroutine in a given sheet, given the
 * sheet has gone through `d_codegen_compile`.
 *
 * \return If the function/subroutine ran without any errors.
 *
 * \param vm The VM to run the function on. The reason it is a seperate
 * argument is because it allows you to push and pop arguments and return values
 * seperately.
 * \param sheet The sheet the function lives in.
 * \param funcName The name of the function/subroutine to run.
 */
DECISION_API bool d_run_function(struct _DVM *vm, struct _sheet *sheet,
                                 const char *funcName);

/**
 * \fn Sheet *d_load_string(const char *source, const char *name,
 *                          CompileOptions *options)
 * \brief Take Decision source code and compile it into bytecode, but do not
 * run it.
 *
 * \return A malloc'd sheet containing all of the compilation info.
 *
 * \param source The source code to compile.
 * \param name The name of the sheet. If NULL, it is set to `"source"`.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API struct _sheet *d_load_string(const char *source, const char *name,
                                          CompileOptions *options);

/**
 * \fn bool d_run_string(const char *source, const char *name,
 *                       CompileOptions *options)
 * \brief Take Decision source code and compile it into bytecode. If it
 * compiled successfully, run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param source The source code the compile.
 * \param name The name of the sheet. If `NULL`, it is set to `"source"`.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API bool d_run_string(const char *source, const char *name,
                               CompileOptions *options);

/**
 * \fn bool d_compile_string(const char *source, const char *filePath,
 *                           CompileOptions *options)
 * \brief Take Decision source code and compile it into bytecode. Then save
 * it into a binary file if it compiled successfully.
 *
 * \return If the code compiled without any errors.
 *
 * \param source The source code to compile.
 * \param filePath Where to write the object file to.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API bool d_compile_string(const char *source, const char *filePath,
                                   CompileOptions *options);

/**
 * \fn Sheet *d_load_source_file(const char *filePath, CompileOptions *options)
 * \brief Take Decision source code from a file and compile it into bytecode,
 * but do not run it.
 *
 * \return A malloc'd sheet containing all of the compilation info.
 *
 * \param filePath The file path of the source file to compile.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API struct _sheet *d_load_source_file(const char *filePath,
                                               CompileOptions *options);

/**
 * \fn bool d_run_source_file(const char *filePath, CompileOptions *options)
 * \brief Take Decision source code in a file and compile it into bytecode. If
 * it compiled successfully, run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param filePath The file path of the source file to compile.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API bool d_run_source_file(const char *filePath,
                                    CompileOptions *options);

/**
 * \fn bool d_compile_file(const char *filePathIn, const char *filePathOut
 *                         CompileOptions *options)
 * \brief Take Decision source code from a file and compile it into bytecode.
 * If it compiled successfully, save it into a binary file.
 *
 * \return If the code compiled without any errors.
 *
 * \param filePathIn The file path of the source file to compile.
 * \param filePathOut Where to write the object file to.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API bool d_compile_file(const char *filePathIn,
                                 const char *filePathOut,
                                 CompileOptions *options);

/**
 * \fn Sheet *d_load_object_file(const char *filePath, CompileOptions *options)
 * \brief Take a Decision object file and load it into memory.
 *
 * \return A malloc'd sheet object containing all of the compilation info.
 *
 * \param filePath The file path of the object file.
 * \param options A set of compile options. If NULL, the default settings are
 * used. The debug setting is ignored, as object files cannot be debugged.
 */
DECISION_API struct _sheet *d_load_object_file(const char *filePath,
                                               CompileOptions *options);

/**
 * \fn bool d_run_object_file(const char *filePath, CompileOptions *options)
 * \brief Take a Decision object file, load it into memory, and run it in the
 * virtual machine.
 *
 * \return If the code ran without any errors.
 *
 * \param filePath The file path of the object file.
 * \param options A set of compile options. If NULL, the default settings are
 * used. The debug setting is ignored, as object files cannot be debugged.
 */
DECISION_API bool d_run_object_file(const char *filePath,
                                    CompileOptions *options);

/**
 * \fn short d_is_object_file(const char *filePath)
 * \brief Decide whether a given file is a Decision object file, a Decision
 * source file, or neither.
 *
 * \return `1` if it is an object file, `0` if it is a source file, `-1` if
 * the file does not exist.
 *
 * \param filePath The file path of the file to examine.
 */
DECISION_API short d_is_object_file(const char *filePath);

/**
 * \fn Sheet *d_load_file(const char *filePath, CompileOptions *options)
 * \brief Take a Decision file, decide whether it is a source or an object file
 * based on its contents, and load it into memory.
 *
 * \return A malloc'd sheet object containing all of the compilation info.
 *
 * \param filePath The file path of the file to load.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API struct _sheet *d_load_file(const char *filePath,
                                        CompileOptions *options);

/**
 * \fn bool d_run_file(const char *filePath, CompileOptions *options)
 * \brief Take a Decision file, decide whether it is a source or an object file
 * based on its contents, and run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param filePath The file path of the file to load.
 * \param options A set of compile options. If NULL, the default settings are
 * used.
 */
DECISION_API bool d_run_file(const char *filePath, CompileOptions *options);

#endif // DECISION_H
