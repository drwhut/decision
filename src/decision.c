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

#include "decision.h"

#include "dasm.h"
#include "dcodegen.h"
#include "derror.h"
#include "dlex.h"
#include "dlink.h"
#include "dmalloc.h"
#include "dobj.h"
#include "doptimize.h"
#include "dsemantic.h"
#include "dsheet.h"
#include "dsyntax.h"
#include "dvm.h"

#include <stdlib.h>
#include <string.h>

/* Definition of the verbose level. */
char verboseLevel = 0;

/**
 * \fn char d_get_verbose_level()
 * \brief Get the current verbose level.
 *
 * \return The verbose level. It will be a number between 0 and 5.
 */
char d_get_verbose_level() {
    return verboseLevel;
}

/**
 * \fn void d_set_verbose_level(char level)
 * \brief Set the current verbose level.
 *
 * \param level The verbose level to set. If the number is bigger than 5,
 * then the verbose level is set to 5.
 */
void d_set_verbose_level(char level) {
    if (level > 5) {
        level = 5;
    } else if (level < 0) {
        level = 0;
    }

    verboseLevel = level;
}

/*
    const char* load_string_from_file(const char* filePath, size_t* size)
    Take the contents of a file and put it into a malloc'd string.

    Returns: A malloc'd string that contains the contents of the file at
    filePath.

    const char* filePath: The file to read.
    size_t* size: A reference to a size that is overwritten with the size of
    the file content in bytes.
    bool binary: Is the file we want to read a binary file?
*/
static const char *load_string_from_file(const char *filePath, size_t *size,
                                         bool binary) {
    FILE *f;

    f = fopen(filePath, (binary) ? "rb" : "r");
    if (f == NULL) {
        printf("Can't open the file!\n");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t fSize = ftell(f);
    *size        = fSize;
    fseek(f, 0, SEEK_SET);

    char *string = d_calloc(fSize + 2, sizeof(char));

    fread(string, fSize, 1, f);

    // Place a newline after the last non-null character, so lexical analysis
    // can stop having a hissy fit. Unless the file is a binary file, in which
    // case... don't.
    if (!binary) {
        for (int i = (int)fSize; i >= 0; i--) {
            if (string[i] != 0) {
                string[i + 1] = '\n';
                break;
            }
        }
    }

    fclose(f);

    return (const char *)string;
}

/*
    static void save_object_to_file(
        const char* filePath,
        const char* content,
        size_t len
    )
    Given some binary content, save it to a file.

    const char* filePath: Where to save the content.
    const char* content: A pointer to the content to save.
    size_t len: The length of the content in bytes.
*/
static void save_object_to_file(const char *filePath, const char *content,
                                size_t len) {
    FILE *f;

    f = fopen(filePath, "wb");
    if (f == NULL) {
        printf("Can't open the file!\n");
        return;
    }

    fwrite(content, sizeof(char), len, f);

    fclose(f);
}

/**
 * \fn bool d_run_sheet(Sheet *sheet)
 * \brief Run the code in a given sheet, given that it has gone through
 * `d_codegen_compile`, and it has exactly one defined `Start` function.
 *
 * \return If the sheet ran without any errors.
 *
 * \param sheet The sheet to run.
 */
bool d_run_sheet(Sheet *sheet) {
    if (sheet->_text != NULL && sheet->_textSize > 0 && sheet->_isCompiled) {
        if (sheet->_isLinked) {
            if (sheet->_main > 0) // A Start function exists.
            {
                DVM vm       = d_vm_create();
                bool success = d_vm_run(&vm, sheet->_text + sheet->_main);
                d_vm_free(&vm);
                return success;
            } else {
                printf("Fatal: Sheet %s has no Start function defined",
                       sheet->filePath);
            }
        } else {
            printf("Fatal: Sheet %s has not been linked", sheet->filePath);
        }
    } else {
        printf("Fatal: Sheet %s has not been compiled", sheet->filePath);
    }

    return false;
}

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
bool d_run_function(DVM *vm, Sheet *sheet, const char *funcName) {
    if (sheet->_text != NULL && sheet->_textSize > 0 && sheet->_isCompiled) {
        if (sheet->_isLinked) {
            void *funcPtr = NULL;

            // Firstly check that the name we've been given isn't that of a
            // core function/subroutine.
            CoreFunction isCoreFunc = d_core_find_name(funcName);
            if ((int)isCoreFunc != -1) {
                printf("Fatal: %s is a core function", funcName);
                return false;
            }

            // If the function is already in the meta list of the sheet, we
            // might as well use that pointer.
            for (size_t metaIndex = 0; metaIndex < sheet->_link.size;
                 metaIndex++) {
                LinkMeta meta = sheet->_link.list[metaIndex];

                if (meta.type == LINK_FUNCTION) {
                    if (strcmp(meta.name, funcName) == 0) {
                        SheetFunction *func = (SheetFunction *)meta.meta;
                        Sheet *extSheet     = func->sheet;

                        if (sheet == extSheet) {
                            // If the function lives inside this sheet, then
                            // the pointer is actually an index.
                            funcPtr = sheet->_text + (size_t)meta._ptr;
                        } else {
                            // Otherwise, the pointer should be accurate
                            // already.
                            funcPtr = meta._ptr;
                        }

                        break;
                    }
                }
            }

            // If we couldn't find it in our link list, then this sheet didn't
            // use the function. So, we're going to have to find out manually
            // where this sheet is and run it there.
            if (funcPtr == NULL) {
                AllNameDefinitions nameDefs =
                    d_get_name_definitions(sheet, funcName);

                if (nameDefs.numDefinitions == 1) {
                    NameDefinition definition = nameDefs.definitions[0];
                    if (definition.type == NAME_FUNCTION) {
                        Sheet *extSheet = definition.sheet;
                        return d_run_function(vm, extSheet, funcName);
                    }
                } else if (nameDefs.numDefinitions == 0) {
                    printf("Fatal: Sheet %s has no function %s defined",
                           sheet->filePath, funcName);
                } else {
                    printf("Fatal: Sheet %s has multiple definitions of the "
                           "function %s defined",
                           sheet->filePath, funcName);
                }

                d_free_name_definitions(&nameDefs);
            } else {
                // We know where it lives, so we can run it!
                return d_vm_run(vm, funcPtr);
            }
        } else {
            printf("Fatal: Sheet %s has not been linked", sheet->filePath);
        }
    } else {
        printf("Fatal: Sheet %s has not been compiled", sheet->filePath);
    }

    return false;
}

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
Sheet *d_load_string(const char *source, const char *name,
                     CompileOptions *options) {
    // If name is NULL, set a name.
    name = (name != NULL) ? name : "source";

    // Represent the sheet in memory, and check for errors along the way.
    Sheet *sheet = d_sheet_create(name);

    // By default, have no initial includes, and don't compile in debug mode.
    CompileOptions opts = DEFAULT_COMPILE_OPTIONS;

    if (options != NULL) {
        opts = *options;
    }

    if (opts.includes != NULL) {
        Sheet **include = opts.includes;

        while (*include) {
            d_sheet_add_include(sheet, *include);
            include++;
        }
    }

    VERBOSE(1, "--- STAGE 1: Creating lexical stream...\n")
    LexStream stream = d_lex_create_stream(source, name);
    if (d_get_verbose_level() >= 4)
        d_lex_dump_stream(stream);

    if (stream.numTokens > 0) {

        VERBOSE(1, "--- STAGE 2: Checking syntax...\n")
        SyntaxResult result = d_syntax_parse(stream, name);
        SyntaxNode *root    = result.node;

        // Syntax analysis may have thrown errors, in which case semantic
        // analysis is unreliable.
        if (result.success) {
            if (d_get_verbose_level() >= 4)
                d_syntax_dump_tree(root);

            VERBOSE(1, "--- STAGE 3: Checking semantics...\n")
            d_semantic_scan(sheet, root, opts.priors, opts.debug);
            if (d_get_verbose_level() >= 2)
                d_sheet_dump(sheet);

            // After checking the sheet, see if we got any errors.
            bool hasErrors   = d_error_report();
            sheet->hasErrors = hasErrors;

            // Compile only if there were no errors.
            if (!hasErrors) {
                VERBOSE(1, "--- STAGE 4: Generating bytecode...\n")
                d_codegen_compile(sheet, opts.debug);

                // Do not optimise the bytecode if we are debugging!
                if (opts.debug) {
                    VERBOSE(
                        5,
                        "--- Skipping optimisation, compiling in debug mode.\n")
                } else {
                    VERBOSE(1, "--- STAGE 5: Optimising bytecode...\n")
                    d_optimize_all(sheet);
                }

                VERBOSE(1, "--- STAGE 6: Linking...\n")
                d_link_sheet(sheet);

                // Dump the compiled content.
                if (d_get_verbose_level() >= 3) {
                    d_asm_dump_all(sheet);

                    if (opts.debug) {
                        d_debug_dump_info(sheet->_debugInfo);
                    }
                }
            }

            // Only free the tree if syntax analysis was successful, as
            // dsyntax.c frees the root node if it failed.
            d_syntax_free_tree(root);

        } else {
            sheet->hasErrors = d_error_report();
        }

    } else {
        ERROR_COMPILER(name, 1, true, "Sheet %s is empty", name);
        sheet->hasErrors = d_error_report();
    }

    d_error_free();
    d_lex_free_stream(stream);

    return sheet;
}

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
bool d_run_string(const char *source, const char *name,
                  CompileOptions *options) {
    Sheet *sheet   = d_load_string(source, name, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

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
bool d_compile_string(const char *source, const char *filePath,
                      CompileOptions *options) {
    Sheet *sheet   = d_load_string(source, NULL, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        size_t objSize;
        const char *obj = d_obj_generate(sheet, &objSize);
        save_object_to_file(filePath, obj, objSize);

        free((char *)obj);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

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
Sheet *d_load_source_file(const char *filePath, CompileOptions *options) {
    size_t _size; // Not needed.
    const char *source = load_string_from_file(filePath, &_size, false);
    Sheet *sheet       = NULL;

    if (source != NULL) {
        sheet = d_load_string(source, filePath, options);
        free((void *)source);
    } else {
        // We errored loading the file.
        sheet            = d_sheet_create(filePath);
        sheet->hasErrors = true;
    }

    return sheet;
}

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
bool d_run_source_file(const char *filePath, CompileOptions *options) {
    Sheet *sheet   = d_load_source_file(filePath, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

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
bool d_compile_file(const char *filePathIn, const char *filePathOut,
                    CompileOptions *options) {
    Sheet *sheet   = d_load_source_file(filePathIn, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        size_t objSize;
        const char *obj = d_obj_generate(sheet, &objSize);
        save_object_to_file(filePathOut, obj, objSize);

        free((char *)obj);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

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
Sheet *d_load_object_file(const char *filePath, CompileOptions *options) {
    size_t size;
    const char *obj = load_string_from_file(filePath, &size, true);
    Sheet *out      = NULL;

    // Ignore the debug option.
    Sheet **includes = NULL;
    if (options != NULL) {
        includes = options->includes;
    }

    Sheet **priors = NULL;
    if (options != NULL) {
        priors = options->priors;
    }

    if (obj != NULL) {
        out = d_obj_load(obj, size, filePath, includes, priors);
        free((char *)obj);

        out->hasErrors = d_error_report();

        if (!out->hasErrors) {
            d_link_sheet(out);
        }

        if (d_get_verbose_level() >= 3) {
            d_asm_dump_all(out);
        }
    } else {
        // We errored loading the file.
        out            = d_sheet_create(filePath);
        out->hasErrors = true;
    }

    return out;
}

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
bool d_run_object_file(const char *filePath, CompileOptions *options) {
    Sheet *sheet   = d_load_object_file(filePath, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

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
short d_is_object_file(const char *filePath) {
    size_t size;
    const char *str = load_string_from_file(filePath, &size, true);

    if (str != NULL) {
        bool isObjectFile = false;

        if (size > 3) {
            if (str[0] == 'D') {
                if (str[1] == '3' && str[2] == '2')
                    isObjectFile = true;
                else if (str[1] == '6' && str[2] == '4')
                    isObjectFile = true;
            }
        }

        free((char *)str);

        return isObjectFile;
    } else
        return -1;
}

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
Sheet *d_load_file(const char *filePath, CompileOptions *options) {
    short fileType = d_is_object_file(filePath);

    Sheet *out;

    switch (fileType) {
        case 0:
            // It is a source file.
            out = d_load_source_file(filePath, options);

            break;

        case 1:
            // It is an object file.
            out = d_load_object_file(filePath, options);

            break;

        default:
            // The file does not exist.
            out            = d_sheet_create(filePath);
            out->hasErrors = true;

            break;
    }

    return out;
}

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
bool d_run_file(const char *filePath, CompileOptions *options) {
    Sheet *sheet   = d_load_file(filePath, options);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}