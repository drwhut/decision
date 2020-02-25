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

#include "dasm.h"
#include "dcfg.h"
#include "dcore.h"
#include "ddebug.h"
#include "decision.h"
#include "dmalloc.h"
#include "dsheet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* String constant for displaying the version and copyright notice. */
static const char *VERSION =
    "Decision " DECISION_VERSION "\n"
    "Copyright (C) 2019-2020  Benjamin Beddows\n"
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
    "  --export-core:                    Output the core reference in JSON\n"
    "                                      format.\n"
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

/* Macro to help check an argument in main(). */
#define ARG(test) strcmp(arg, #test) == 0

// TEMP

void onWireValue(Sheet *sheet, Wire wire, DType type, LexData value) {
    printf("Value %d transfered from (%zu, %zu) to (%zu, %zu)!\n",
           value.integerValue, wire.socketFrom.nodeIndex,
           wire.socketFrom.socketIndex, wire.socketTo.nodeIndex,
           wire.socketTo.socketIndex);
}

void onExecutionWire(Sheet *sheet, Wire wire) {
    printf("Execution wire activated! (%zu, %zu) -> (%zu, %zu)\n",
           wire.socketFrom.nodeIndex, wire.socketFrom.socketIndex,
           wire.socketTo.nodeIndex, wire.socketTo.socketIndex);
}

void onNodeActivated(Sheet *sheet, size_t nodeIndex) {
    printf("Node %zu activated!\n", nodeIndex);
}

int main(int argc, char *argv[]) {

    CompileOptions options = DEFAULT_COMPILE_OPTIONS;
    options.debug          = true;

    // TEMP
    Sheet *sheet = d_load_file("../../tests/examples/factorial.dc", &options);

    d_sheet_dump(sheet);
    d_asm_dump_all(sheet);
    d_debug_dump_info(sheet->_debugInfo);

    DebugSession session = d_debug_create_session(
        sheet, &onWireValue, &onExecutionWire, &onNodeActivated);

    d_debug_continue_session(&session);

    d_debug_stop_session(&session);

    d_sheet_free(sheet);

    return 0;
    /*
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
        // --export-core
        else if (ARG(--export - core)) {
            d_core_dump_json();
            return 0;
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
                char *objFilePath = d_calloc(objFilePathLen + 1, sizeof(char));
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
                               (const char *)objFilePath, NULL);

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
                    Sheet *sheet =
                        d_load_object_file((const char *)filePath, NULL);
                    d_asm_dump_all(sheet);
                    d_sheet_free(sheet);

                    return 0;
                } else
                    return d_run_object_file((const char *)filePath, NULL);
            } else {
                // Check that we are not disassembling a source file.
                if (disassemble) {
                    printf("Cannot disassemble any file other than a Decision "
                           "object file!\n");
                    return 1;
                } else
                    return d_run_source_file((const char *)filePath, NULL);
            }
        }
    } else {
        print_help();
        return 1;
    }
    */
}
