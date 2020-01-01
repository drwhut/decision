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

#include "decision.h"

#include "dasm.h"
#include "dcodegen.h"
#include "derror.h"
#include "dlex.h"
#include "dlink.h"
#include "dmalloc.h"
#include "doptimize.h"
#include "dsemantic.h"
#include "dsheet.h"
#include "dsyntax.h"
#include "dvm.h"

#include <stdlib.h>
#include <string.h>

/* String constant for displaying the version and copyright notice. */
static const char *VERSION =
    "Decision " DECISION_VERSION "\n"
    "Copyright (C) 2019  Benjamin Beddows\n"
    "This program comes with ABSOLUTELY NO WARRANTY.\n"
    "This is free software, and you are welcome to redistribute it under the\n"
    "conditions of the GNU GPLv3: "
    "<https://www.gnu.org/licenses/gpl-3.0.html>\n";

/* String constant for the contents of the help screen. */
static const char *HELP =
    "USAGE: decision [option]... <FILE>\n"
    "Run a Decision source file or object file FILE.\n"
    "\n"
    "OPTIONS:\n"
    "  -c, --compile:                    Compile all source file(s) into .dco\n"
    "                                      object files.\n"
    "  -D, --disassemble:                Disassemble a given object file.\n"
    "  -h, -?, --help:                   Display this screen and exit.\n"
    "  -V[=LEVEL], --verbose[=LEVEL]:    Output verbose debugging information "
    "as\n"
    "                                      source code is being compiled. See\n"
    "                                      VERBOSE LEVELS for different levels "
    "of\n"
    "                                      information.\n"
    "  -v, --version:                    Output version information and exit.\n"
    "\n"
    "VERBOSE LEVELS:\n"
    "  Each level has the properties of the levels before it.\n"
    "  The default level is 3, if no level is given, or it is invalid.\n"
    "  0:    No verbose output.\n"
    "  1:    State vaguely what is happening at each stage.\n"
    "  2:    Show detailed information about the sheet after it is checked.\n"
    "  3:    Show the compiled bytecode after linkage (before running).\n"
    "  4:    Show other data like the syntax tree, lexical stream, etc.\n"
    "  5:    Explain in great detail what is happening at each stage.\n"
    "\n";

/* Definition of the VERBOSE_LEVEL extern. */
char VERBOSE_LEVEL = 0;

/*
    void print_version()
    Display the version number to the screen.
    TODO: Add license, copyright, etc.
*/
static void print_version() {
    printf("%s", VERSION);
}

/*
    void print_help()
    Display the list of arguments to the screen, in case the user didn't
    provide any, or actually want to see it.
*/
static void print_help() {
    printf("%s", HELP);
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

#ifdef DECISION_SAFE_FUNCTIONS
    if (fopen_s(&f, filePath, (binary) ? "rb" : "r") != 0) {
        printf("Can't open the file!\n");
        return NULL;
    }
    if (f == NULL) {
        return NULL;
    }
#else
    f = fopen(filePath, (binary) ? "rb" : "r");
    if (f == NULL) {
        printf("Can't open the file!\n");
        return NULL;
    }
#endif // DECISION_SAFE_FUNCTIONS

    fseek(f, 0, SEEK_END);
    size_t fSize = ftell(f);
    *size        = fSize;
    fseek(f, 0, SEEK_SET);

    char *string = (char *)d_malloc(fSize + 2);

    // There were problems on Windows with trailing rogue charatcers.
    memset(string, 0, fSize + 2);

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

#ifdef DECISION_SAFE_FUNCTIONS
    if (fopen_s(&f, filePath, "wb") != 0) {
        printf("Can't open the file!\n");
        return;
    }
    if (f == NULL) {
        return;
    }
#else
    f = fopen(filePath, "wb");
    if (f == NULL) {
        printf("Can't open the file!\n");
        return;
    }
#endif // DECISION_SAFE_FUNCTIONS

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
                DVM vm;
                d_vm_reset(&vm);
                return d_vm_run(&vm, sheet->_text + sheet->_main);
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
                    d_semantic_get_name_definitions(sheet, funcName);

                NameDefinition definition;
                if (d_semantic_select_name_definition(funcName, nameDefs,
                                                      &definition)) {
                    if (definition.type == NAME_FUNCTION) {
                        Sheet *extSheet = definition.sheet;
                        return d_run_function(vm, extSheet, funcName);
                    }
                } else {
                    printf("Fatal: Sheet %s has no function %s defined",
                           sheet->filePath, funcName);
                }

                d_semantic_free_name_definitions(nameDefs);
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
 * \fn Sheet *d_load_string(const char *source, const char *name)
 * \brief Take Decision source code and compile it into bytecode, but do not
 * run it.
 *
 * \return A malloc'd sheet containing all of the compilation info.
 *
 * \param source The source code to compile.
 * \param name The name of the sheet. If NULL, it is set to `"source"`.
 */
Sheet *d_load_string(const char *source, const char *name) {
    // If name is NULL, set a name.
    name = (name != NULL) ? name : "source";

    // Represent the sheet in memory, and check for errors along the way.
    Sheet *sheet = d_sheet_create(name);

    VERBOSE(1, "--- STAGE 1: Creating lexical stream...\n")
    LexStream stream = d_lex_create_stream(source, name);
    if (VERBOSE_LEVEL >= 4)
        d_lex_dump_stream(stream);

    if (stream.numTokens > 0) {

        VERBOSE(1, "--- STAGE 2: Checking syntax...\n")
        SyntaxResult result = d_syntax_parse(stream, name);
        SyntaxNode *root    = result.node;

        // Syntax analysis may have thrown errors, in which case semantic
        // analysis is unreliable.
        if (result.success) {
            if (VERBOSE_LEVEL >= 4)
                d_syntax_dump_tree(root);

            VERBOSE(1, "--- STAGE 3: Checking semantics...\n")
            d_semantic_scan(sheet, root);
            if (VERBOSE_LEVEL >= 2)
                d_sheet_dump(sheet);

            // After checking the sheet, see if we got any errors.
            bool hasErrors   = d_error_report();
            sheet->hasErrors = hasErrors;

            // Compile only if there were no errors.
            if (!hasErrors) {
                VERBOSE(1, "--- STAGE 4: Generating bytecode...\n")
                d_codegen_compile(sheet);

                VERBOSE(1, "--- STAGE 5: Optimising bytecode...\n")
                d_optimize_all(sheet);

                VERBOSE(1, "--- STAGE 6: Linking...\n")
                d_link_sheet(sheet);
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
 * \fn bool d_run_string(const char *source, const char *name)
 * \brief Take Decision source code and compile it into bytecode. If it
 * compiled successfully, run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param source The source code the compile.
 * \param name The name of the sheet. If `NULL`, it is set to `"source"`.
 */
bool d_run_string(const char *source, const char *name) {
    Sheet *sheet   = d_load_string(source, name);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        if (VERBOSE_LEVEL >= 3)
            d_asm_dump_all(sheet);

        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

/**
 * \fn bool d_compile_string(const char *source, const char *filePath)
 * \brief Take Decision source code and compile it into bytecode. Then save
 * it into a binary file if it compiled successfully.
 *
 * \return If the code compiled without any errors.
 *
 * \param source The source code to compile.
 * \param filePath Where to write the object file to.
 */
bool d_compile_string(const char *source, const char *filePath) {
    Sheet *sheet   = d_load_string(source, NULL);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        size_t objSize;
        const char *obj = d_asm_generate_object(sheet, &objSize);
        save_object_to_file(filePath, obj, objSize);

        free((char *)obj);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

/**
 * \fn Sheet *d_load_source_file(const char *filePath)
 * \brief Take Decision source code from a file and compile it into bytecode,
 * but do not run it.
 *
 * \return A malloc'd sheet containing all of the compilation info.
 *
 * \param filePath The file path of the source file to compile.
 */
Sheet *d_load_source_file(const char *filePath) {
    size_t _size; // Not needed.
    const char *source = load_string_from_file(filePath, &_size, false);
    Sheet *sheet       = NULL;

    if (source != NULL) {
        sheet = d_load_string(source, filePath);
        free((void *)source);
    } else {
        // We errored loading the file.
        sheet            = d_sheet_create(filePath);
        sheet->hasErrors = true;
    }

    return sheet;
}

/**
 * \fn bool d_run_source_file(const char *filePath)
 * \brief Take Decision source code in a file and compile it into bytecode. If
 * it compiled successfully, run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param filePath The file path of the source file to compile.
 */
bool d_run_source_file(const char *filePath) {
    Sheet *sheet   = d_load_source_file(filePath);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        if (VERBOSE_LEVEL >= 3)
            d_asm_dump_all(sheet);

        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

/**
 * \fn bool d_compile_file(const char *filePathIn, const char *filePathOut)
 * \brief Take Decision source code from a file and compile it into bytecode.
 * If it compiled successfully, save it into a binary file.
 *
 * \return If the code compiled without any errors.
 *
 * \param filePathIn The file path of the source file to compile.
 * \param filePathOut Where to write the object file to.
 */
bool d_compile_file(const char *filePathIn, const char *filePathOut) {
    Sheet *sheet   = d_load_source_file(filePathIn);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        size_t objSize;
        const char *obj = d_asm_generate_object(sheet, &objSize);
        save_object_to_file(filePathOut, obj, objSize);

        free((char *)obj);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

/**
 * \fn Sheet *d_load_object_file(const char *filePath)
 * \brief Take a Decision object file and load it into memory.
 *
 * \return A malloc'd sheet object containing all of the compilation info.
 *
 * \param filePath The file path of the object file.
 */
Sheet *d_load_object_file(const char *filePath) {
    size_t size;
    const char *obj = load_string_from_file(filePath, &size, true);
    Sheet *out      = NULL;

    if (obj != NULL) {
        out = d_asm_load_object(obj, size, filePath);
        free((char *)obj);

        if (!out->hasErrors) {
            d_link_sheet(out);
        }
    } else {
        // We errored loading the file.
        out            = d_sheet_create(filePath);
        out->hasErrors = true;
    }

    return out;
}

/**
 * \fn bool d_run_object_file(const char *filePath)
 * \brief Take a Decision object file, load it into memory, and run it in the
 * virtual machine.
 *
 * \return If the code ran without any errors.
 *
 * \param filePath The file path of the object file.
 */
bool d_run_object_file(const char *filePath) {
    Sheet *sheet   = d_load_object_file(filePath);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        if (VERBOSE_LEVEL >= 3)
            d_asm_dump_all(sheet);

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
 * \fn Sheet *d_load_file(const char *filePath)
 * \brief Take a Decision file, decide whether it is a source or an object file
 * based on its contents, and load it into memory.
 *
 * \return A malloc'd sheet object containing all of the compilation info.
 *
 * \param filePath The file path of the file to load.
 */
Sheet *d_load_file(const char *filePath) {
    short fileType = d_is_object_file(filePath);

    Sheet *out;

    switch (fileType) {
        case 0:
            // It is a source file.
            out = d_load_source_file(filePath);

            break;

        case 1:
            // It is an object file.
            out = d_load_object_file(filePath);

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
 * \fn bool d_run_file(const char *filePath)
 * \brief Take a Decision file, decide whether it is a source or an object file
 * based on its contents, and run it in the virtual machine.
 *
 * \return If the code compiled/ran without any errors.
 *
 * \param filePath The file path of the file to load.
 */
bool d_run_file(const char *filePath) {
    Sheet *sheet   = d_load_file(filePath);
    bool hadErrors = sheet->hasErrors;

    if (!hadErrors) {
        d_link_sheet(sheet);

        if (VERBOSE_LEVEL >= 3)
            d_asm_dump_all(sheet);

        hadErrors = !d_run_sheet(sheet);
    }

    d_sheet_free(sheet);

    return hadErrors;
}

/* Macro to help check an argument in main(). */
#define ARG(test) strcmp(arg, #test) == 0

#ifndef DECISION_LIBRARY
int main(int argc, char *argv[]) {
    char *filePath   = NULL;
    bool compile     = false;
    bool disassemble = false;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        // -c, --compile
        if (ARG(-c) || ARG(--compile)) {
            compile = true;
        }
        // -D, --disassemble
        else if (ARG(-D) || ARG(--disassemble)) {
            disassemble = true;
        }
        // -h, -?, --help
        else if (ARG(-h) || ARG(-?) || ARG(--help))
        {
            print_help();
            return 0;
        }
        // -V, --verbose
        else if (strncmp(arg, "-V", 2) == 0 ||
                 strncmp(arg, "--verbose", 9) == 0) {
            // The default verbose level.
            VERBOSE_LEVEL = 3;

            size_t flagSize = (strncmp(arg, "-V", 2) == 0) ? 2 : 9;

            // The +2 is for the = and integer.
            if (strlen(arg) == flagSize + 2) {
                if (arg[flagSize] == '=') {
                    switch (arg[flagSize + 1]) {
                        case '0':
                            VERBOSE_LEVEL = 0;
                            break;
                        case '1':
                            VERBOSE_LEVEL = 1;
                            break;
                        case '2':
                            VERBOSE_LEVEL = 2;
                            break;
                        case '3':
                            VERBOSE_LEVEL = 3;
                            break;
                        case '4':
                            VERBOSE_LEVEL = 4;
                            break;
                        case '5':
                            VERBOSE_LEVEL = 5;
                            break;
                        default:
                            break;
                    }
                }
            }

            printf("Verbose level set to %d.\n", VERBOSE_LEVEL);
        }
        // -v, --version
        else if (ARG(-v) || ARG(--version)) {
            print_version();
            return 0;
        } else {
            // If it's something we don't recognise, assume it's the source
            // file.
            if (filePath != NULL && !compile) {
                // A "file" has already been given.
                printf("More than one file has been given!\n");
                return 1;
            }

            filePath = argv[i];

            if (compile) {
                // We want to change the file path so it has an extension of
                // .dco, instead of .dc
                // If it's a different extension, just add it on.
                bool dcExtension = false;
                if (strlen(filePath) >= 3) {
                    char *extension = filePath + strlen(filePath) - 3;
                    if (strncmp(extension, ".dc", 3) == 0)
                        dcExtension = true;
                }

                size_t addedSpace     = (dcExtension) ? 1 : 4;
                size_t filePathLen    = strlen(filePath);
                size_t objFilePathLen = filePathLen + addedSpace;

                // Copy the filePath into a bigger char array.
                char *objFilePath = (char *)d_malloc(objFilePathLen + 1);
                memcpy(objFilePath, filePath, filePathLen);

                if (dcExtension) {
                    // The only difference is that there is an 'o' at the end
                    // of the file path.
                    objFilePath[objFilePathLen - 1] = 'o';
                } else {
                    // We want to put the extension AT THE END of the old file
                    // path.
                    memcpy(objFilePath + filePathLen, ".dco", 4);
                }

                objFilePath[objFilePathLen] = 0;

                // Now we have our new objFilePath, let's compile!
                d_compile_file((const char *)filePath,
                               (const char *)objFilePath);

                free(objFilePath);
            }
        }
    }

    if (filePath != NULL) {
        if (!compile) {
            // If the file path ends in .dco, it's an object file.
            bool isObjectFile = false;
            if (strlen(filePath) >= 4) {
                char *extension = filePath + strlen(filePath) - 4;
                if (strncmp(extension, ".dco", 4) == 0)
                    isObjectFile = true;
            }

            if (isObjectFile) {
                if (disassemble) {
                    Sheet *sheet = d_load_object_file((const char *)filePath);
                    d_asm_dump_all(sheet);
                    d_sheet_free(sheet);

                    return 0;
                } else
                    return d_run_object_file((const char *)filePath);
            } else {
                // Check that we are not disassembling a source file.
                if (disassemble) {
                    printf("Cannot disassemble any file other than a Decision "
                           "object file!\n");
                    return 1;
                } else
                    return d_run_source_file((const char *)filePath);
            }
        }
    } else {
        print_help();
        return 1;
    }
}
#endif // DECISION_LIBRARY
