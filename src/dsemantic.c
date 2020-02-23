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

#include "dsemantic.h"

#include "decision.h"
#include "derror.h"
#include "dmalloc.h"
#include "dname.h"

#include <stdlib.h>
#include <string.h>

/* A union for holding the different kinds of property argument data. */
typedef union {
    const char *name;
    LexToken *literal;
    DType dataType;
} PropertyArgumentData;

/* A struct for a property argument and it's data. */
typedef struct {
    SyntaxDefinition type;
    PropertyArgumentData data;
} PropertyArgument;

/* A struct for holding property arguments. */
typedef struct {
    PropertyArgument *args;
    size_t numArgs;
} PropertyArgumentList;

/* Macros for checking property arguments. */
#define PROPERTY_ARGUMENT_NAME_DEFINED(arg) \
    ((arg).type == STX_TOKEN && (arg).data.name != NULL)
#define PROPERTY_ARGUMENT_TYPE_DEFINED(arg) \
    (((arg).type == STX_dataType) && (arg).data.dataType != TYPE_NONE)
#define PROPERTY_ARGUMENT_LITERAL_DEFINED(arg) \
    ((arg).type == STX_literal && (arg).data.literal != NULL)

#define PROPERTY_ARGUMENT_TYPE_IS_VAR(arg) \
    (((arg).data.dataType | TYPE_VAR_ANY) == TYPE_VAR_ANY)

/*
    The following functions are all about adding properties to a sheet,
    depending on the name of the property. These are used, along with the
    following macro, in d_semantic_scan_properties().
*/
#define IF_PROPERTY(property)                 \
    if (strcmp(propertyName, #property) == 0) \
        add_property_##property(sheet, lineNum, argList);

static void add_property_Variable(Sheet *sheet, size_t lineNum,
                                  PropertyArgumentList argList) {
    bool defaultValue = true;
    bool description  = true;

    if (argList.numArgs < 2) {
        d_error_compiler_push("Variable property needs at least 2 arguments.",
                              sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 4) {
        d_error_compiler_push("Variable property needs at most 4 arguments.",
                              sheet->filePath, lineNum, true);
    } else {
        if (argList.numArgs <= 3) {
            description = false;

            if (argList.numArgs == 2) {
                d_error_compiler_push(
                    "No default value specified in Variable property",
                    sheet->filePath, lineNum, false);
                defaultValue = false;
            }
        }

        PropertyArgument variableNameArg = argList.args[0];
        PropertyArgument dataTypeArg     = argList.args[1];
        PropertyArgument defaultValueArg;
        PropertyArgument descriptionArg;
        defaultValueArg.type        = 0;
        defaultValueArg.data.name   = NULL;
        descriptionArg.type         = 0;
        descriptionArg.data.literal = NULL;

        if (defaultValue) {
            defaultValueArg = argList.args[2];
        }

        if (description) {
            descriptionArg = argList.args[3];
        }

        const char *varName        = NULL;
        const char *varDescription = NULL;
        DType varType              = TYPE_NONE;
        LexData varDefault         = {0};

        if (PROPERTY_ARGUMENT_NAME_DEFINED(variableNameArg)) {
            varName = variableNameArg.data.name;

            // We need to check if the variable name is already defined.
            bool alreadyDefined = false;

            for (size_t i = 0; i < sheet->numVariables; i++) {
                if (strcmp(varName, sheet->variables[i].variableMeta.name) ==
                    0) {
                    alreadyDefined = true;
                    break;
                }
            }

            if (alreadyDefined) {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Variable %s already defined", varName);
            }
        } else {
            ERROR_COMPILER(
                sheet->filePath, lineNum, true,
                "Variable name argument (argument 1/%zu) is not a name",
                argList.numArgs);
        }

        if (PROPERTY_ARGUMENT_TYPE_DEFINED(dataTypeArg)) {
            // We need to check that the data type is not "vague",
            // i.e. it is between TYPE_VAR_MIN and TYPE_VAR_MAX, and
            // discretely one of the values inbetween.
            if (PROPERTY_ARGUMENT_TYPE_IS_VAR(dataTypeArg)) {
                DType finalType = dataTypeArg.data.dataType;

                if (d_type_is_vague(finalType)) {
                    d_error_compiler_push("Variable data types cannot be vague",
                                          sheet->filePath, lineNum, true);
                } else {
                    varType = finalType;
                }
            } else {
                d_error_compiler_push(
                    "Variable data type is not a valid data type",
                    sheet->filePath, lineNum, true);
            }
        } else {
            ERROR_COMPILER(
                sheet->filePath, lineNum, true,
                "Variable data type argument (argument 2/%zu) is not "
                "a data type",
                argList.numArgs);
        }

        // Get the default value, otherwise make our own.
        if (defaultValue) {
            if (PROPERTY_ARGUMENT_LITERAL_DEFINED(defaultValueArg)) {
                LexToken *defaultValueToken = defaultValueArg.data.literal;

                switch (varType) {
                    case TYPE_INT:
                        if (defaultValueToken->type == TK_INTEGERLITERAL) {
                            varDefault.integerValue =
                                defaultValueToken->data.integerValue;
                        } else {
                            d_error_compiler_push(
                                "Set variable to Integer, got non-integer "
                                "default value",
                                sheet->filePath, lineNum, true);
                        }
                        break;

                    case TYPE_FLOAT:
                        if (defaultValueToken->type == TK_FLOATLITERAL) {
                            varDefault.floatValue =
                                defaultValueToken->data.floatValue;
                        } else if (defaultValueToken->type ==
                                   TK_INTEGERLITERAL) {
                            // If we've been given an integer, that's fine,
                            // we can easily convert it.
                            varDefault.floatValue =
                                (dfloat)defaultValueToken->data.integerValue;
                        } else {
                            d_error_compiler_push("Set variable to Float, got "
                                                  "non-float default value",
                                                  sheet->filePath, lineNum,
                                                  true);
                        }
                        break;

                    case TYPE_STRING:
                        if (defaultValueToken->type == TK_STRINGLITERAL) {
                            varDefault.stringValue =
                                defaultValueToken->data.stringValue;
                        } else {
                            d_error_compiler_push("Set variable to String, got "
                                                  "non-string default value",
                                                  sheet->filePath, lineNum,
                                                  true);
                        }
                        break;

                    case TYPE_BOOL:
                        if (defaultValueToken->type == TK_BOOLEANLITERAL) {
                            varDefault.booleanValue =
                                defaultValueToken->data.booleanValue;
                        } else {
                            d_error_compiler_push(
                                "Set variable to Boolean, got non-boolean "
                                "default value",
                                sheet->filePath, lineNum, true);
                        }
                        break;

                    default:
                        break;
                }
            } else {
                d_error_compiler_push(
                    "Default value of variable is not a literal",
                    sheet->filePath, lineNum, true);
            }
        } else {
            switch (varType) {
                case TYPE_INT:
                    varDefault.integerValue = 0;
                    break;

                case TYPE_FLOAT:
                    varDefault.floatValue = 0.0;
                    break;

                case TYPE_STRING:
                    varDefault.stringValue = "";
                    break;

                case TYPE_BOOL:
                    varDefault.booleanValue = false;
                    break;

                default:
                    break;
            }
        }

        if (description) {
            if (PROPERTY_ARGUMENT_LITERAL_DEFINED(descriptionArg)) {
                LexToken *descriptionToken = descriptionArg.data.literal;

                if (TYPE_FROM_LEX_LITERAL(descriptionToken->type) ==
                    TYPE_STRING) {
                    varDescription = descriptionToken->data.stringValue;
                } else {
                    d_error_compiler_push("Description is not a literal string",
                                          sheet->filePath, lineNum, true);
                }
            } else {
                d_error_compiler_push("Description is not a literal string",
                                      sheet->filePath, lineNum, true);
            }
        }

        // We've gotten the variable, now we add it to the sheet if everything
        // went ok.
        if (varName != NULL && varType != TYPE_NONE) {
            SocketMeta varMeta;
            varMeta.name         = varName;
            varMeta.description  = varDescription;
            varMeta.type         = varType;
            varMeta.defaultValue = varDefault;
            d_sheet_add_variable(sheet, varMeta);
        }
    }
}

static void add_property_Include(Sheet *sheet, size_t lineNum,
                                 PropertyArgumentList argList) {
    if (argList.numArgs == 1) {
        PropertyArgument arg = argList.args[0];

        // If it's a pure name, look at the include path.
        // If it's a string, look at the file path.
        if (PROPERTY_ARGUMENT_NAME_DEFINED(arg)) {
            // TODO: Implement include path.
        } else if (PROPERTY_ARGUMENT_LITERAL_DEFINED(arg)) {
            if (arg.data.literal->type == TK_STRINGLITERAL) {
                LexToken *token = arg.data.literal;

                Sheet *includedSheet = d_sheet_add_include_from_path(
                    sheet, token->data.stringValue);

                if (includedSheet->hasErrors) {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Included sheet %s produced errors",
                                   includedSheet->filePath);
                }
            } else {
                d_error_compiler_push(
                    "Include argument literal is not a string literal",
                    sheet->filePath, lineNum, true);
            }
        } else {
            d_error_compiler_push("Include argument is not a name or string",
                                  sheet->filePath, lineNum, true);
        }
    } else {
        d_error_compiler_push("1 argument needed for Include properties",
                              sheet->filePath, lineNum, true);
    }
}

/*
    Functions to help build up NodeDefinition structs for functions and
    subroutines.
    TODO: Move the storage of temporary definitions to a struct?
*/

static NodeDefinition *funcs = NULL;
static size_t numFuncs       = 0;

static void add_socket(const char *name, SocketMeta socket, bool isInput) {
    size_t index = 0;
    bool found   = false;

    for (size_t i = 0; i < numFuncs; i++) {
        NodeDefinition def = funcs[i];

        if (strcmp(name, def.name) == 0) {
            index = i;
            found = true;
            break;
        }
    }

    if (found) {
        NodeDefinition func = funcs[index];

        SocketMeta *sockets     = (SocketMeta *)func.sockets;
        size_t numSockets       = func.numSockets;
        size_t startOutputIndex = func.startOutputIndex;

        // Adjust the number of sockets and the start output index.
        numSockets++;
        if (isInput) {
            startOutputIndex++;
        }

        const size_t newAlloc = numSockets * sizeof(SocketMeta);

        if (sockets == NULL) {
            sockets = d_malloc(newAlloc);
        } else {
            sockets = d_realloc(sockets, newAlloc);
        }

        // If it's an input, we need to shift the outputs along!
        if (isInput) {
            const size_t indexFrom      = startOutputIndex - 1;
            const size_t indexTo        = indexFrom + 1;
            const size_t numOutputsPrev = numSockets - startOutputIndex;
            const size_t moveAmt        = numOutputsPrev * sizeof(SocketMeta);
            memmove(sockets + indexTo, sockets + indexFrom, moveAmt);
            sockets[startOutputIndex - 1] = socket;
        } else {
            sockets[numSockets - 1] = socket;
        }

        // Now save the new properties.
        *(SocketMeta **)(&(funcs[index].sockets)) = sockets;
        funcs[index].numSockets                   = numSockets;
        funcs[index].startOutputIndex             = startOutputIndex;
    }
}

// NOTE: This assumes the name and description are malloc'd!
static void create_func(const char *name, const char *desc, bool sub) {
    numFuncs++;
    const size_t newAlloc = numFuncs * sizeof(NodeDefinition);

    if (funcs == NULL) {
        funcs = d_malloc(newAlloc);
    } else {
        funcs = d_realloc(funcs, newAlloc);
    }

    NodeDefinition def = {NULL, NULL, NULL, 0, 0, false};
    def.name           = name;
    def.description    = desc;

    funcs[numFuncs - 1] = def;

    // If it's a subroutine, add execution sockets.
    // TODO: Make these consistent with the ones in dcfunc.c!
    if (sub) {
        char *beforeSocketName = d_malloc(7);
        strcpy(beforeSocketName, "before");

        char *beforeSocketDescription = d_malloc(53);
        strcpy(beforeSocketDescription,
               "The node will activate when this input is activated.");

        SocketMeta beforeSocket;
        beforeSocket.name                      = beforeSocketName;
        beforeSocket.description               = beforeSocketDescription;
        beforeSocket.type                      = TYPE_EXECUTION;
        beforeSocket.defaultValue.integerValue = 0;

        char *afterSocketName = d_malloc(6);
        strcpy(afterSocketName, "after");

        char *afterSocketDescription = d_malloc(64);
        strcpy(
            afterSocketDescription,
            "This output will activate once the node has finished executing.");

        SocketMeta afterSocket;
        afterSocket.name                      = afterSocketName;
        afterSocket.description               = afterSocketDescription;
        afterSocket.type                      = TYPE_EXECUTION;
        afterSocket.defaultValue.integerValue = 0;

        add_socket(name, beforeSocket, true);
        add_socket(name, afterSocket, false);
    }
}

static void add_funcs(Sheet *sheet) {
    for (size_t i = 0; i < numFuncs; i++) {
        d_sheet_add_function(sheet, funcs[i]);
    }
}

static void free_funcs() {
    if (funcs != NULL) {
        free(funcs);
    }

    funcs    = NULL;
    numFuncs = 0;
}

static void add_property_Function(Sheet *sheet, size_t lineNum,
                                  PropertyArgumentList argList) {
    if (argList.numArgs > 2) {
        d_error_compiler_push("Function property needs at most 2 arguments",
                              sheet->filePath, lineNum, true);
    } else if (argList.numArgs < 1) {
        d_error_compiler_push("Function property needs at least 1 argument",
                              sheet->filePath, lineNum, true);
    } else {
        const char *name = NULL;
        const char *desc = NULL;

        PropertyArgument nameArg = argList.args[0];

        if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
            name = nameArg.data.name;

            // Does a function with this name already exist?
            for (size_t i = 0; i < numFuncs; i++) {
                NodeDefinition func = funcs[i];

                if (strcmp(name, func.name) == 0) {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Function %s is already defined", name);
                    return;
                }
            }

            if (argList.numArgs == 2) {
                PropertyArgument descArg = argList.args[1];

                if (PROPERTY_ARGUMENT_LITERAL_DEFINED(descArg)) {
                    LexToken *descToken = descArg.data.literal;

                    if (TYPE_FROM_LEX_LITERAL(descToken->type) == TYPE_STRING) {
                        desc = descToken->data.stringValue;
                    } else {
                        d_error_compiler_push(
                            "Function description is not a literal string",
                            sheet->filePath, lineNum, true);
                    }
                } else {
                    d_error_compiler_push(
                        "Function description is not a literal string",
                        sheet->filePath, lineNum, true);
                }
            }

            // Add the function to the list.
            create_func(name, desc, false);
        } else {
            d_error_compiler_push("Function name argument is not a name",
                                  sheet->filePath, lineNum, true);
        }
    }
}

static void add_property_Subroutine(Sheet *sheet, size_t lineNum,
                                    PropertyArgumentList argList) {
    if (argList.numArgs > 2) {
        d_error_compiler_push("Subroutine property needs at most 2 arguments",
                              sheet->filePath, lineNum, true);
    } else if (argList.numArgs < 1) {
        d_error_compiler_push("Subroutine property needs at least 1 argument",
                              sheet->filePath, lineNum, true);
    } else {
        const char *name = NULL;
        const char *desc = NULL;

        PropertyArgument nameArg = argList.args[0];

        if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
            name = nameArg.data.name;

            // Does a function with this name already exist?
            for (size_t i = 0; i < numFuncs; i++) {
                NodeDefinition func = funcs[i];

                if (strcmp(name, func.name) == 0) {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Subroutine %s is already defined", name);
                    return;
                }
            }

            if (argList.numArgs == 2) {
                PropertyArgument descArg = argList.args[1];

                if (PROPERTY_ARGUMENT_LITERAL_DEFINED(descArg)) {
                    LexToken *descToken = descArg.data.literal;

                    if (TYPE_FROM_LEX_LITERAL(descToken->type) == TYPE_STRING) {
                        desc = descToken->data.stringValue;
                    } else {
                        d_error_compiler_push(
                            "Subroutine description is not a literal string",
                            sheet->filePath, lineNum, true);
                    }
                } else {
                    d_error_compiler_push(
                        "Subroutine description is not a literal string",
                        sheet->filePath, lineNum, true);
                }
            }

            // Add the subroutine to the list.
            create_func(name, desc, true);
        } else {
            d_error_compiler_push("Subroutine name argument is not a name",
                                  sheet->filePath, lineNum, true);
        }
    }
}

// TODO: Add properties AlternateFunction and AlternateSubroutine.

static void add_property_FunctionInput(Sheet *sheet, size_t lineNum,
                                       PropertyArgumentList argList) {
    PropertyArgument funcArg;
    funcArg.type      = 0;
    funcArg.data.name = NULL;

    PropertyArgument nameArg;
    nameArg.type      = 0;
    nameArg.data.name = NULL;

    PropertyArgument typeArg;
    typeArg.type      = 0;
    typeArg.data.name = NULL;

    PropertyArgument defaultArg;
    defaultArg.type      = 0;
    defaultArg.data.name = NULL;

    PropertyArgument descriptionArg;
    descriptionArg.type         = 0;
    descriptionArg.data.literal = NULL;

    LexData defaultValue;
    defaultValue.integerValue = 0;

    bool hasDefault     = true;
    bool hasDescription = true;

    if (argList.numArgs < 3) {
        d_error_compiler_push(
            "FunctionInput property needs at least 3 arguments",
            sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 5) {
        d_error_compiler_push(
            "FunctionInput property needs at most 5 arguments", sheet->filePath,
            lineNum, true);
    } else {
        if (argList.numArgs <= 4) {
            hasDescription = false;

            if (argList.numArgs == 3) {
                d_error_compiler_push(
                    "No default value specified in FunctionInput property",
                    sheet->filePath, lineNum, false);
                hasDefault = false;
            }
        }

        funcArg = argList.args[0];
        nameArg = argList.args[1];
        typeArg = argList.args[2];

        if (hasDefault) {
            defaultArg = argList.args[3];
        }

        if (hasDescription) {
            descriptionArg = argList.args[4];
        }

        const char *funcName = NULL;

        const char *socketName        = NULL;
        const char *socketDescription = NULL;
        DType socketType              = TYPE_NONE;

        if (PROPERTY_ARGUMENT_NAME_DEFINED(funcArg)) {
            funcName = funcArg.data.name;
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionInput function argument (argument 1/%zu) "
                           "is not a name",
                           argList.numArgs);
        }

        if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
            socketName = nameArg.data.name;
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, false,
                           "FunctionInput name argument (argument "
                           "2/%zu) is not a name, ignoring",
                           argList.numArgs);
        }

        if (PROPERTY_ARGUMENT_TYPE_DEFINED(typeArg) &&
            PROPERTY_ARGUMENT_TYPE_IS_VAR(typeArg)) {
            socketType = typeArg.data.dataType;

            // TODO: Add support for vague data types in functions.
            if (d_type_is_vague(socketType)) {
                d_error_compiler_push("Vague data types in functions is "
                                      "not currently supported",
                                      sheet->filePath, lineNum, true);
            }
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionInput data type argument (argument "
                           "3/%zu) is invalid",
                           argList.numArgs);
        }

        if (hasDefault) {
            if (PROPERTY_ARGUMENT_LITERAL_DEFINED(defaultArg)) {
                LexToken *literal   = defaultArg.data.literal;
                DType typeOfLiteral = TYPE_FROM_LEX_LITERAL(literal->type);

                if ((typeOfLiteral & socketType) != TYPE_NONE) {
                    // The data types match, we can set the default
                    // value.
                    defaultValue = literal->data;
                } else if (typeOfLiteral == TYPE_INT &&
                           socketType == TYPE_FLOAT) {
                    // We can easily convert an integer into a float.
                    literal->data.floatValue =
                        (dfloat)literal->data.integerValue;
                    defaultValue = literal->data;
                } else {
                    ERROR_COMPILER(sheet->filePath, lineNum, false,
                                   "FunctionInput default value "
                                   "argument data type %s "
                                   "does not match input data type %s",
                                   d_type_name(typeOfLiteral),
                                   d_type_name(socketType));
                }
            } else {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "FunctionInput default value argument "
                               "(argument 4/%zu) is not a literal",
                               argList.numArgs);
            }
        }

        if (hasDescription) {
            if (PROPERTY_ARGUMENT_LITERAL_DEFINED(descriptionArg)) {
                LexToken *descriptionToken = descriptionArg.data.literal;

                if (TYPE_FROM_LEX_LITERAL(descriptionToken->type) ==
                    TYPE_STRING) {
                    socketDescription = descriptionToken->data.stringValue;
                } else {
                    d_error_compiler_push("Description is not a literal string",
                                          sheet->filePath, lineNum, true);
                }
            } else {
                d_error_compiler_push("Description is not a literal string",
                                      sheet->filePath, lineNum, true);
            }
        }

        // Now we've organised the arguments, we can add the argument!
        if (socketName != NULL && socketType != TYPE_NONE) {
            SocketMeta socket;
            socket.name         = socketName;
            socket.description  = socketDescription;
            socket.type         = socketType;
            socket.defaultValue = defaultValue;

            add_socket(funcName, socket, true);
        }

        if (funcName != NULL) {
            free((char *)funcName);
        }
    }
}

static void add_property_FunctionOutput(Sheet *sheet, size_t lineNum,
                                        PropertyArgumentList argList) {
    PropertyArgument funcArg;
    funcArg.type      = 0;
    funcArg.data.name = NULL;

    PropertyArgument nameArg;
    nameArg.type      = 0;
    nameArg.data.name = NULL;

    PropertyArgument typeArg;
    typeArg.type      = 0;
    typeArg.data.name = NULL;

    PropertyArgument descriptionArg;
    descriptionArg.type         = 0;
    descriptionArg.data.literal = NULL;

    bool description = true;

    if (argList.numArgs < 3) {
        d_error_compiler_push(
            "FunctionOutput property needs at least 3 arguments",
            sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 4) {
        d_error_compiler_push(
            "FunctionOutput property needs at most 4 arguments",
            sheet->filePath, lineNum, true);
    } else {
        if (argList.numArgs <= 3) {
            description = false;
        }

        funcArg = argList.args[0];
        nameArg = argList.args[1];
        typeArg = argList.args[2];

        if (description) {
            descriptionArg = argList.args[3];
        }

        const char *funcName = NULL;

        const char *socketName        = NULL;
        const char *socketDescription = NULL;
        DType socketType              = TYPE_NONE;

        if (PROPERTY_ARGUMENT_NAME_DEFINED(funcArg)) {
            funcName = funcArg.data.name;
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionOutput function argument (argument 1/%zu) "
                           "is not a name",
                           argList.numArgs);
        }

        if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
            socketName = nameArg.data.name;
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, false,
                           "FunctionOutput name argument (argument "
                           "2/%zu) is not a name, ignoring",
                           argList.numArgs);
        }

        if (PROPERTY_ARGUMENT_TYPE_DEFINED(typeArg) &&
            PROPERTY_ARGUMENT_TYPE_IS_VAR(typeArg)) {
            socketType = typeArg.data.dataType;

            // TODO: Add support for vague data types in functions.
            if (d_type_is_vague(socketType)) {
                d_error_compiler_push("Vague data types in functions is "
                                      "not currently supported",
                                      sheet->filePath, lineNum, true);
            }
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionOutput data type argument (argument "
                           "3/%zu) is invalid",
                           argList.numArgs);
        }

        if (description) {
            if (PROPERTY_ARGUMENT_LITERAL_DEFINED(descriptionArg)) {
                LexToken *descriptionToken = descriptionArg.data.literal;

                if (TYPE_FROM_LEX_LITERAL(descriptionToken->type) ==
                    TYPE_STRING) {
                    socketDescription = descriptionToken->data.stringValue;
                } else {
                    d_error_compiler_push("Description is not a literal string",
                                          sheet->filePath, lineNum, true);
                }
            } else {
                d_error_compiler_push("Description is not a literal string",
                                      sheet->filePath, lineNum, true);
            }
        }

        // Now we've organised the arguments, we can add the return value!
        if (socketName != NULL && socketType != TYPE_NONE) {
            SocketMeta socket;
            socket.name                      = socketName;
            socket.description               = socketDescription;
            socket.type                      = socketType;
            socket.defaultValue.integerValue = 0;

            add_socket(funcName, socket, false);
        }

        if (funcName != NULL) {
            free((char *)funcName);
        }
    }
}

/* A helper function for d_semantic_scan_properties */
static void scan_property(Sheet *sheet, const char *propertyName,
                          SyntaxNode *node, size_t lineNum) {
    // Now we have the property name, we can get the
    // arguments.
    PropertyArgumentList argList = (PropertyArgumentList){NULL, 0};
    bool argListMallocd          = false;

    if (node->sibling != NULL) {
        // STX_propertyCall
        node = node->sibling;

        if (node->child != NULL) {
            // STX_listOfPropertyArguments
            node = node->child;

            // STX_propertyArgument
            SyntaxNode *argParentNode = node->child;

            const size_t numArgs = d_syntax_get_num_children(node);

            if (numArgs > 0) {
                argList.numArgs = numArgs;
                argList.args    = (PropertyArgument *)d_malloc(
                    numArgs * sizeof(PropertyArgument));
                argListMallocd = true;

                for (size_t i = 0; i < numArgs; i++) {
                    SyntaxNode *argNode = argParentNode->child;

                    SyntaxDefinition definition = argNode->definition;

                    PropertyArgument arg;
                    arg.type      = definition;
                    arg.data.name = NULL;

                    switch (definition) {
                        case STX_TOKEN:;
                            if (argNode->info->type == TK_NAME)
                                arg.data.name = argNode->info->data.stringValue;
                            break;
                        case STX_literal:;
                            // STX_TOKEN
                            SyntaxNode *literalNode = argNode->child;
                            arg.data.literal        = literalNode->info;
                            break;
                        case STX_dataType:;
                            // STX_dataType
                            SyntaxNode *dataTypeNode = argNode->child;
                            arg.data.dataType =
                                TYPE_FROM_LEX(dataTypeNode->info->type);
                            break;
                        default:
                            d_error_compiler_push("Property arguments "
                                                  "must be either a "
                                                  "name, literal or data "
                                                  "type.",
                                                  sheet->filePath, lineNum,
                                                  true);
                            break;
                    }

                    argList.args[i] = arg;

                    argParentNode = argParentNode->sibling;
                }
            }
        }
    }

    // What is the name of the property?
    IF_PROPERTY(Variable)
    else IF_PROPERTY(Include) else IF_PROPERTY(Function) else IF_PROPERTY(
        Subroutine) else IF_PROPERTY(FunctionInput) else IF_PROPERTY(FunctionOutput) else {
        ERROR_COMPILER(sheet->filePath, lineNum, true,
                       "Unknown property name %s", propertyName);
    }

    if (argListMallocd) {
        free(argList.args);
    }
}

/**
 * \fn void d_semantic_scan_properties(Sheet *sheet, SyntaxNode *root)
 * \brief Sets the properties of the sheet, given the syntax tree.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
void d_semantic_scan_properties(Sheet *sheet, SyntaxNode *root) {
    // Firstly, get all Property Statements from the syntax tree.
    SyntaxSearchResult propertySearchResults =
        d_syntax_get_all_nodes_with(root, STX_propertyStatement, false);

    // For each of the results,
    for (size_t propertyIndex = 0;
         propertyIndex < propertySearchResults.numOccurances; propertyIndex++) {
        // Get the syntax node...
        SyntaxNode *node = propertySearchResults.occurances[propertyIndex];

        if (node != NULL) {
            // And work our way down to the individual elements.
            // STX_propertyExpression
            node = node->child;

            if (node != NULL) {
                // STX_TOKEN
                node = node->child;

                if (node != NULL) {
                    // Get the name of the property.
                    LexToken *nodeInfo = node->info;

                    if (nodeInfo != NULL) {
                        if (nodeInfo->type == TK_NAME) {
                            const char *propertyName =
                                nodeInfo->data.stringValue;
                            size_t lineNum = node->onLineNum;

                            VERBOSE(
                                5,
                                "- Checking property named %s on line %zu...\n",
                                propertyName, lineNum);

                            scan_property(sheet, propertyName, node, lineNum);

                            // Once we've scanned the property, there's no need
                            // to store the name of the property anymore.
                            free((char *)propertyName);
                        }
                    }
                }
            }
        }
    }

    // Next, if any functions were defined, add them to the sheet.
    add_funcs(sheet);
    free_funcs();

    // Lastly, free the search results.
    d_syntax_free_results(propertySearchResults);
}

/*
    const char* get_first_arg_name(SyntaxNode* callNode)
    If possible, get the name of a function from the STX_call node's
    first argument.
    This is mostly used to find the function we're returning from in a
    Return function.

    Return: If it exists, the name data of the first token argument.
    NULL if it does not exist.

    SyntaxNode* callNode: A pointer to a syntax node of type STX_call.
*/
static const char *get_first_arg_name(SyntaxNode *callNode) {
    SyntaxNode *nameNode = callNode;

    if (nameNode != NULL) {
        // STX_listOfArguments
        nameNode = nameNode->child;
        if (nameNode != NULL) {
            // STX_argument
            nameNode = nameNode->child;
            if (nameNode != NULL) {
                // STX_TOKEN (TK_NAME)
                nameNode = nameNode->child;
                if (nameNode != NULL) {
                    LexToken *nameToken = nameNode->info;
                    if (nameToken != NULL) {
                        if (nameToken->type == TK_NAME) {
                            const char *name = nameToken->data.stringValue;

                            return name;
                        }
                    }
                }
            }
        }
    }

    return NULL;
}

/**
 * \struct _lineSocketPair
 * \brief A struct associating a line identifier with a socket.
 *
 * \typedef struct _lineSocketPair LineSocketPair
 */
typedef struct _lineSocketPair {
    dint identifier;
    NodeSocket socket;
} LineSocketPair;

/* A helper function for d_semantic_scan_nodes */
static void scan_node(Sheet *sheet, const NodeDefinition *nodeDef,
                      NameDefinition nameDefinition, SyntaxNode *expr,
                      SyntaxNode *node, size_t lineNum,
                      LineSocketPair **knownLines, size_t *knownLinesCapacity,
                      size_t *numKnownLines, LineSocketPair **unknownLines,
                      size_t *unknownLinesCapacity, size_t *numUnknownLines) {
    size_t numInputs   = d_definition_num_inputs(nodeDef);
    size_t _numOutputs = d_definition_num_outputs(nodeDef);

    // Malloc the array of types.
    DType *types = (DType *)d_malloc(nodeDef->numSockets * sizeof(DType));

    for (size_t i = 0; i < numInputs + _numOutputs; i++) {
        types[i] = nodeDef->sockets[i].type;
    }

    // Malloc the array of literal inputs.
    LexData *literals = (LexData *)d_malloc(numInputs * sizeof(LexData));

    for (size_t i = 0; i < numInputs; i++) {
        literals[i].integerValue = 0;
    }

    size_t startOutputIndex = nodeDef->startOutputIndex;

    // What will the index of the node be when it's stored in
    // the sheet?
    const size_t nodeIndex = sheet->graph.numNodes;

    NodeSocket socket;
    socket.nodeIndex = nodeIndex;

    // Is it the "Start" node? There's a special pointer in the
    // sheet for this node.
    if (strcmp(nodeDef->name, "Start") == 0) {
        sheet->startNodeIndex = nodeIndex;

        // We want to make sure the sheet doesn't have more than
        // one Start function, so we keep track.
        sheet->numStarts++;
    }

    // Or is it a "Define" node? Functions have a pointer to
    // the node that defines their functionality.
    else if (strcmp(nodeDef->name, "Define") == 0) {
        if (nameDefinition.definition.function != NULL) {
            nameDefinition.definition.function->defineNodeIndex = nodeIndex;

            // There can only be 1 define node for each
            // function.
            nameDefinition.definition.function->numDefineNodes++;
        }
    }

    // Or is it a "Return" node? Functions (unlike subroutines)
    // must have 1 return node.
    else if (strcmp(nodeDef->name, "Return") == 0) {
        if (nameDefinition.definition.function != NULL) {
            nameDefinition.definition.function->lastReturnNodeIndex = nodeIndex;

            nameDefinition.definition.function->numReturnNodes++;
        }
    }

    // Now, we can get the inputs.
    // However, it's not as easy as getting the properties,
    // since we also need to look for line identifiers.

    SyntaxSearchResult inputArgs;
    inputArgs.numOccurances = 0;

    // STX_call
    node = node->sibling;

    if (node != NULL) {
        inputArgs = d_syntax_get_all_nodes_with(node, STX_argument, false);

        // If the number of inputs we got was bigger than what we expected,
        // resize the type and literal arrays in the node.
        if (inputArgs.numOccurances > numInputs) {
            types =
                (DType *)d_realloc(types, (inputArgs.numOccurances +
                                           d_definition_num_outputs(nodeDef)) *
                                              sizeof(DType));

            literals = (LexData *)d_realloc(literals, inputArgs.numOccurances *
                                                          sizeof(LexData));

            memmove(types + inputArgs.numOccurances, types + numInputs,
                    d_definition_num_outputs(nodeDef) * sizeof(DType));

            for (size_t i = numInputs; i < inputArgs.numOccurances; i++) {
                types[i]                 = types[numInputs - 1];
                literals[i].integerValue = 0;
            }

            numInputs = inputArgs.numOccurances;

            startOutputIndex = numInputs;
        }

        // If there are no sockets in the definition, stop now!
        if (inputArgs.numOccurances > 0 && (nodeDef->numSockets == 0)) {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "Node %s is defined to have no sockets",
                           nodeDef->name);
            return;
        }

        for (size_t inputIndex = 0; inputIndex < inputArgs.numOccurances;
             inputIndex++) {
            size_t typeIndex = inputIndex;

            // If there can be an infinite amount of inputs, we
            // still need to get the data type.
            if (nodeDef->infiniteInputs) {
                if (inputIndex >= nodeDef->startOutputIndex) {
                    typeIndex = nodeDef->startOutputIndex - 1;
                }
            }

            // Create the socket index.
            socket.socketIndex = inputIndex;

            // The data type the socket should be.
            DType socketType = nodeDef->sockets[typeIndex].type;

            SyntaxNode *inputSyntaxNode = inputArgs.occurances[inputIndex];

            // STX_literal, or STX_lineIdentifier.
            // STX_TOKEN could be a variable name for a function
            // like Set. STX_keyword shouldn't be here.
            inputSyntaxNode = inputSyntaxNode->child;

            switch (inputSyntaxNode->definition) {
                case STX_literal:;
                    // STX_TOKEN
                    inputSyntaxNode = inputSyntaxNode->child;

                    if (inputSyntaxNode != NULL) {
                        LexToken *literalToken = inputSyntaxNode->info;

                        // Make sure the literal data type is
                        // compatable!
                        DType typeOfLiteral =
                            TYPE_FROM_LEX_LITERAL(literalToken->type);

                        if ((typeOfLiteral & socketType) != TYPE_NONE) {
                            // Just set the LexData instead of
                            // testing for each data type.
                            literals[inputIndex] = literalToken->data;

                            types[inputIndex] = typeOfLiteral;
                        } else if (typeOfLiteral == TYPE_INT &&
                                   socketType == TYPE_FLOAT) {
                            // If the compiler expected a float,
                            // but got an integer, that's fine.
                            // We can easily convert it.
                            literalToken->data.floatValue =
                                (dfloat)literalToken->data.integerValue;

                            literals[inputIndex] = literalToken->data;

                            types[inputIndex] = TYPE_FLOAT;
                        } else {
                            ERROR_COMPILER(sheet->filePath, lineNum, true,
                                           "Literal argument type (%s) "
                                           "does not match type of socket "
                                           "(%s)",
                                           d_type_name(typeOfLiteral),
                                           d_type_name(socketType));
                        }
                    }

                    break;

                case STX_lineIdentifier:;
                    // STX_TOKEN (TK_INTEGERLITERAL)
                    inputSyntaxNode = inputSyntaxNode->child;

                    if (inputSyntaxNode != NULL) {
                        LexToken *identifierToken = inputSyntaxNode->info;

                        if (identifierToken != NULL) {
                            dint identifier =
                                identifierToken->data.integerValue;

                            // If we need to up the capacity of
                            // the list, do so.
                            if (*numUnknownLines + 1 > *unknownLinesCapacity) {
                                *unknownLines = (LineSocketPair *)d_realloc(
                                    *unknownLines, (++(*unknownLinesCapacity)) *
                                                       sizeof(LineSocketPair));
                            }

                            LineSocketPair unknownLine;
                            unknownLine.identifier                = identifier;
                            unknownLine.socket                    = socket;
                            (*unknownLines)[(*numUnknownLines)++] = unknownLine;
                        }
                    }

                    // We will check later when connecting up
                    // the sockets if the data types are
                    // correct.

                    break;

                case STX_TOKEN:;
                    // It could be a variable name...
                    LexToken *token = inputSyntaxNode->info;

                    if (token->type == TK_NAME) {
                        const char *name = token->data.stringValue;

                        // Check to see if this input socket is
                        // actually expecting a name.
                        if (socketType == TYPE_NAME) {

                            // Check to see if the name is
                            // defined.
                            AllNameDefinitions nameDefinitions =
                                d_get_name_definitions(sheet, name);

                            if (nameDefinitions.numDefinitions > 0) {
                                // Set the input socket's string
                                // value.
                                literals[inputIndex] = token->data;

                                // NOTE: For Set nodes, the
                                // definition of the node points
                                // to the variable, not the Set
                                // core function.
                                if (nameDefinition.definition.coreFunc ==
                                    CORE_SET) {
                                    NameDefinition varDefinition;

                                    if (d_select_name_definition(
                                            name, nameDefinitions,
                                            &varDefinition)) {
                                        nameDefinition = varDefinition;
                                    }
                                }
                            } else {
                                ERROR_COMPILER(sheet->filePath, lineNum, true,
                                               "Name %s is not defined", name);
                            }

                            d_free_name_definitions(&nameDefinitions);
                        } else {
                            d_error_compiler_push("Name argument given when "
                                                  "socket input isn't a name",
                                                  sheet->filePath, lineNum,
                                                  true);
                        }
                    } else {
                        d_error_compiler_push("Token argument is not a name",
                                              sheet->filePath, lineNum, true);
                    }

                    break;

                default:
                    d_error_compiler_push("Invalid node input argument",
                                          sheet->filePath, lineNum, true);
                    break;
            }
        }

        d_syntax_free_results(inputArgs);
    }

    // Check to see if the number of inputs is what we expected.
    bool validNumInputs = true;
    size_t defNumInputs = d_definition_num_inputs(nodeDef);
    if (nodeDef->infiniteInputs) {
        validNumInputs = (inputArgs.numOccurances >= defNumInputs);
    } else {
        validNumInputs = (inputArgs.numOccurances == defNumInputs);
    }

    if (!validNumInputs) {
        ERROR_COMPILER(sheet->filePath, lineNum, true,
                       "Expected %zu inputs in node %s, got %zu", defNumInputs,
                       nodeDef->name, inputArgs.numOccurances);
    }

    // Now we get the outputs.
    // This will be easier since all outputs are line
    // identifiers.
    size_t numOutputs = 0;

    size_t defNumOutputs = d_definition_num_outputs(nodeDef);

    // STX_listOfLineIdentifier
    node = expr->sibling;

    if (node != NULL) {
        SyntaxNode *leaf    = NULL;
        LexToken *leafToken = NULL;

        // STX_lineIdentifier
        node = node->child;

        // Go through all of the children of the
        // STX_listOfLineIdentifier
        while (node != NULL) {
            if (inputArgs.numOccurances > 0 && (nodeDef->numSockets == 0)) {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Node %s is defined to have no sockets",
                               nodeDef->name);
                return;
            }

            if (numOutputs >= defNumOutputs) {
                // We've got more arguments than we
                // expected.
                ERROR_COMPILER(sheet->filePath, lineNum, false,
                               "Got more outputs than expected "
                               "(expected at most %zu), ignoring "
                               "redundant outputs",
                               defNumOutputs);
                break;
            }

            // Create the socket index.
            size_t socketIndex = numInputs + numOutputs;
            socket.socketIndex = socketIndex;

            numOutputs++;

            // STX_TOKEN
            leaf = node->child;

            if (leaf != NULL) {
                leafToken = leaf->info;

                if (leafToken != NULL) {
                    dint identifier = leafToken->data.integerValue;

                    // If we need to up the capacity of the
                    // list, do so.
                    if (*numKnownLines + 1 > *knownLinesCapacity) {
                        *knownLines = (LineSocketPair *)d_realloc(
                            *knownLines,
                            (++(*knownLinesCapacity)) * sizeof(LineSocketPair));
                    }

                    LineSocketPair knownLine;
                    knownLine.identifier              = identifier;
                    knownLine.socket                  = socket;
                    (*knownLines)[(*numKnownLines)++] = knownLine;
                }
            }

            node = node->sibling;
        }
    }

    // FINALLY, add the node to the sheet.
    Node newNode;
    newNode.definition       = nodeDef;
    newNode.lineNum          = lineNum;
    newNode.reducedTypes     = types;
    newNode.literalValues    = literals;
    newNode.startOutputIndex = startOutputIndex;
    newNode.nameDefinition   = nameDefinition;
    d_graph_add_node(&(sheet->graph), newNode);
}

/**
 * \fn void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root)
 * \param Sets the nodes of the sheet, given the syntax tree.
 *
 * **NOTE:** This function also sets the connections between the nodes.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root) {
    // Firstly, get all Statements from the syntax tree.
    SyntaxSearchResult statementSearchResults =
        d_syntax_get_all_nodes_with(root, STX_statement, false);

    // TODO: THE FOLLOWING IS A VERY INEFFICIENT WAY OF CONNECTING SOCKETS.
    // MAYBE REPLACE WITH ANOTHER DATA STRUCTURE LIKE A DICTIONARY?

    // A list of known lines and their "from" sockets.
    // We put lines into this list as we scan output sockets,
    // and then connect them to their respective input socket
    // using the line identifier.
    LineSocketPair *knownLineDefinitions = (LineSocketPair *)d_malloc(
        3 * statementSearchResults.numOccurances * sizeof(LineSocketPair));
    size_t knownLineDefinitionsCapacity =
        3 * statementSearchResults.numOccurances;
    size_t numKnownLineDefinitions = 0;

    // A list of known lines and their "to" sockets.
    // We put lines in this list as we scan input sockets,
    // and then connect them to their respective output socket
    // using the line identifier.
    LineSocketPair *unknownLineDefinitions = (LineSocketPair *)d_malloc(
        3 * statementSearchResults.numOccurances * sizeof(LineSocketPair));
    size_t unknownLineDefinitionsCapacity =
        3 * statementSearchResults.numOccurances;
    size_t numUnknownLineDefinitions = 0;

    // For each of the results,
    for (size_t statementIndex = 0;
         statementIndex < statementSearchResults.numOccurances;
         statementIndex++) {
        // Get the syntax node...
        SyntaxNode *node = statementSearchResults.occurances[statementIndex];

        // And work our way down to the individual elements.
        // STX_expression
        // We need this one to get to the outputs.
        SyntaxNode *expr = node->child;

        // STX_TOKEN
        node = expr->child;

        // Get the name of the node.
        LexToken *nodeInfo = node->info;

        if (nodeInfo != NULL) {
            if (nodeInfo->type == TK_NAME) {
                const char *nodeName = nodeInfo->data.stringValue;
                size_t lineNum       = node->onLineNum;

                VERBOSE(5, "- Checking node named %s on line %zu...\n",
                        nodeName, lineNum);

                // What if the first argument is something like a function name
                // for Return? We need to let get_node_properties know if so.
                const char *funcName = get_first_arg_name(node->sibling);

                // A variable that tells us exactly where this node's definition
                // comes from, so we can link to it later.
                NameDefinition nameDefinition;

                // Get the node's true properties!
                const NodeDefinition *nodeDef = d_get_definition(
                    sheet, nodeName, lineNum, funcName, &nameDefinition);

                if (nodeDef != NULL) {
                    scan_node(sheet, nodeDef, nameDefinition, expr, node,
                              lineNum, &knownLineDefinitions,
                              &knownLineDefinitionsCapacity,
                              &numKnownLineDefinitions, &unknownLineDefinitions,
                              &unknownLineDefinitionsCapacity,
                              &numUnknownLineDefinitions);
                } else {
                    // We can't recognise the node name!
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Undefined node %s", nodeName);
                }

                // Free the node's name from the lexical token, as it will no
                // longer be needed - it will be in the node's definition.
                free(nodeInfo->data.stringValue);

                // If there was a name argument, free that too.
                if (funcName != NULL) {
                    free((char *)funcName);
                }
            }
        }
    }

    // Now we've gone through all of the nodes, we need to do some checks.
    if (sheet->numStarts > 1) {
        Node startNode = sheet->graph.nodes[sheet->startNodeIndex];
        ERROR_COMPILER(sheet->filePath, startNode.lineNum, true,
                       "Found %zu Start functions, only 1 is allowed",
                       sheet->numStarts);
    }

    for (size_t i = 0; i < sheet->numFunctions; i++) {
        SheetFunction func = sheet->functions[i];

        bool funcIsSubroutine = d_is_subroutine(func);

        // All functions and subroutines must at most 1 Define node, and
        // subroutines must have 1 and only 1 Define node.
        if (func.numDefineNodes == 0 && funcIsSubroutine) {
            ERROR_COMPILER(sheet->filePath, 0, true,
                           "Function %s has no Define "
                           "node defined",
                           func.functionDefinition.name);
        } else if (func.numDefineNodes > 1) {
            Node defineNode = sheet->graph.nodes[func.defineNodeIndex];
            ERROR_COMPILER(sheet->filePath, defineNode.lineNum, true,
                           "Function %s has %zu Define nodes defined when only "
                           "1 is allowed",
                           func.functionDefinition.name, func.numDefineNodes);
        }

        // All functions (not subroutines) must have 1 and only 1 Return node.
        if (!funcIsSubroutine) {
            if (func.numReturnNodes == 0) {
                ERROR_COMPILER(sheet->filePath, 0, true,
                               "Function %s has no Return "
                               "node defined",
                               func.functionDefinition.name);
            } else if (func.numReturnNodes > 1) {
                Node lastReturnNode =
                    sheet->graph.nodes[func.lastReturnNodeIndex];
                ERROR_COMPILER(sheet->filePath, lastReturnNode.lineNum, true,
                               "Function %s has %zu Return nodes defined when "
                               "only 1 is allowed",
                               func.functionDefinition.name,
                               func.numReturnNodes)
            }
        }
    }

    // Now we will connect the input and output sockets that have the same line
    // identifier.
    // Note that if an input socket cannot find a connection, it means that the
    // line is undefined, which is an error.

    VERBOSE(5, "Connecting %zu defined lines with %zu undefined lines... ",
            numKnownLineDefinitions, numUnknownLineDefinitions)

    for (size_t i = 0; i < numUnknownLineDefinitions; i++) {
        bool foundMatch = false;

        LineSocketPair unknownLine = unknownLineDefinitions[i];

        for (size_t j = 0; j < numKnownLineDefinitions; j++) {
            LineSocketPair knownLine = knownLineDefinitions[j];

            // We found a match!
            if (knownLine.identifier == unknownLine.identifier) {
                foundMatch = true;

                // Create a wire and add it to the sheet.
                Wire wire;
                wire.socketFrom = knownLine.socket;
                wire.socketTo   = unknownLine.socket;
                d_graph_add_wire(&(sheet->graph), wire, sheet->filePath);
            }
        }

        if (!foundMatch) {
            Node node = sheet->graph.nodes[unknownLine.socket.nodeIndex];
            ERROR_COMPILER(sheet->filePath, node.lineNum, true,
                           "Undefined line identifier %" DINT_PRINTF_u,
                           unknownLine.identifier)
        }
    }

    VERBOSE(5, "done.\n")

    // Lastly, free the search results and line definitions.
    d_syntax_free_results(statementSearchResults);
    free(knownLineDefinitions);
    free(unknownLineDefinitions);
}

/*
    A macro for the next function to tell if a type has been reduced.
    i.e. is only one bit 1?
*/
#define IS_TYPE_REDUCED(t) (t && !(t & (t - 1)))

/* A helper function for d_semantic_reduce_types */
static void reduce_core_node(Sheet *sheet, const CoreFunction coreFunc,
                             size_t nodeIndex, size_t numSockets,
                             bool *nodeReduced) {
    bool reducedAllInputs = true;

    NodeSocket socket;
    socket.nodeIndex   = nodeIndex;
    socket.socketIndex = 0;

    switch (coreFunc) {
        // Arithmetic core operations return a Float if any one of their inputs
        // is a Float, otherwise an Integer, except for CORE_DIVIDE, which
        // always returns a Float. This also applies to For, where if any of the
        // arguments are Floats, the output index will also be a Float. We just
        // need to check that the output we're setting isn't an Execution
        // socket.
        case CORE_ADD:
        case CORE_SUBTRACT:
        case CORE_MULTIPLY:
        case CORE_DIVIDE:
        case CORE_FOR:;
            bool hasFloatInput      = false;
            NodeSocket outputSocket = socket;

            for (size_t socketIndex = 0; socketIndex < numSockets;
                 socketIndex++) {
                socket.socketIndex = socketIndex;
                SocketMeta meta    = d_get_socket_meta(sheet->graph, socket);

                if (meta.type != TYPE_EXECUTION) {
                    // Is the socket an input?
                    if (d_is_input_socket(sheet->graph, socket)) {
                        // If we know it's a Float anyway (from a literal
                        // argument)...
                        if (meta.type == TYPE_FLOAT) {
                            hasFloatInput = true;
                        }
                        // If it's an Integer literal, we don't care about it.
                        // It's already reduced. So, we need to check
                        // connections.
                        else {
                            int wireIndex =
                                d_wire_find_first(sheet->graph, socket);

                            if (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
                                NodeSocket otherSide =
                                    sheet->graph.wires[wireIndex].socketTo;

                                // If this socket reduced?
                                SocketMeta otherMeta =
                                    d_get_socket_meta(sheet->graph, otherSide);
                                if (IS_TYPE_REDUCED(otherMeta.type)) {
                                    // Great! We can set this socket's type to
                                    // be discrete!
                                    sheet->graph.nodes[nodeIndex]
                                        .reducedTypes[socketIndex] =
                                        otherMeta.type;

                                    if (otherMeta.type == TYPE_FLOAT) {
                                        hasFloatInput = true;
                                    }
                                } else {
                                    reducedAllInputs = false;
                                }
                            }
                        }
                    } else {
                        outputSocket = socket;
                    }
                }
            }

            // If we know our output needs to be a Float, set it now. (Except if
            // it is a divide.)
            if (coreFunc != CORE_DIVIDE) {
                sheet->graph.nodes[outputSocket.nodeIndex]
                    .reducedTypes[outputSocket.socketIndex] =
                    (hasFloatInput) ? TYPE_FLOAT : TYPE_INT;
            }

            // If we've gotten all of our inputs reduced, we can say we are
            // fully reduced now.
            if (reducedAllInputs) {
                nodeReduced[nodeIndex] = true;
            }

            break;

        // Print and Set have a single TYPE_VAR_ANY input. With Set, we need to
        // check that the type we reduced down to is the same as the type of the
        // variable we're setting.
        case CORE_PRINT:
        case CORE_SET:;
            DType reducedTo       = TYPE_NONE;
            const char *inputName = NULL;
            for (size_t socketIndex = 0; socketIndex < numSockets;
                 socketIndex++) {
                socket.socketIndex = socketIndex;
                SocketMeta meta    = d_get_socket_meta(sheet->graph, socket);

                if (d_is_input_socket(sheet->graph, socket)) {
                    if (meta.type == TYPE_NAME) {
                        inputName = meta.defaultValue.stringValue;
                    }

                    else if (meta.type != TYPE_EXECUTION &&
                             meta.type != TYPE_NAME) {
                        reducedTo = meta.type;
                    }

                    // This is our input of TYPE_VAR_ANY. That will change
                    // soon... muhaha. Unless we're printing a literal. In which
                    // case... muhaha?
                    if (meta.type == TYPE_VAR_ANY) {
                        // Ok... we'll try the connection.
                        int wireIndex = d_wire_find_first(sheet->graph, socket);

                        if (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
                            NodeSocket otherSide =
                                sheet->graph.wires[wireIndex].socketTo;

                            // Is the socket reduced?
                            SocketMeta otherMeta =
                                d_get_socket_meta(sheet->graph, otherSide);

                            if (IS_TYPE_REDUCED(otherMeta.type)) {
                                // Great! We can set this socket's type to be
                                // discrete!
                                sheet->graph.nodes[nodeIndex]
                                    .reducedTypes[socketIndex] = otherMeta.type;

                                reducedTo = otherMeta.type;
                            }
                        }
                    }
                }
            }

            if (reducedTo != TYPE_NONE) {
                nodeReduced[nodeIndex] = true;

                if (coreFunc == CORE_SET && inputName != NULL) {
                    Node node = sheet->graph.nodes[nodeIndex];

                    // We need to check the variable type matches!
                    SheetVariable *var =
                        node.nameDefinition.definition.variable;

                    // Does the reduced type not match the variable type?
                    if (reducedTo != var->variableMeta.type) {
                        ERROR_COMPILER(sheet->filePath, node.lineNum, true,
                                       "Input type (%s) does not match "
                                       "variable's type (%s has type %s)",
                                       d_type_name(reducedTo),
                                       var->variableMeta.name,
                                       d_type_name(var->variableMeta.type));
                    }
                }
            }

            break;

        // For bitwise operators, the inputs and outputs can either be all
        // integers or all booleans, but they CANNOT mix.
        case CORE_AND:
        case CORE_NOT:
        case CORE_OR:
        case CORE_XOR:;
            DType finalType =
                TYPE_NONE; // Represents all of the socket's types.
            bool allSame = true;
            for (size_t socketIndex = 0; socketIndex < numSockets;
                 socketIndex++) {
                socket.socketIndex = socketIndex;

                if (d_is_input_socket(sheet->graph, socket)) {
                    SocketMeta meta = d_get_socket_meta(sheet->graph, socket);

                    // If we know it's an Integer or Boolean anyway (from a
                    // literal argument)...
                    if (meta.type == TYPE_INT || meta.type == TYPE_BOOL) {

                        // Before we replace finalType, we need to make sure, if
                        // finalType has a type, if this type is the same.
                        if (finalType != TYPE_NONE && meta.type != finalType) {
                            Node node = sheet->graph.nodes[nodeIndex];
                            d_error_compiler_push("All inputs in bitwise "
                                                  "operators must be of the "
                                                  "same type",
                                                  sheet->filePath, node.lineNum,
                                                  true);
                            allSame = false;
                        }

                        finalType = meta.type;
                    }
                    // So we need to check the connection.
                    else {
                        int wireIndex = d_wire_find_first(sheet->graph, socket);

                        if (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
                            NodeSocket otherSide =
                                sheet->graph.wires[wireIndex].socketTo;

                            SocketMeta otherMeta =
                                d_get_socket_meta(sheet->graph, otherSide);

                            // Is this socket reduced?
                            if (IS_TYPE_REDUCED(otherMeta.type)) {
                                // Great! We can set this socket's type to be
                                // discrete!
                                sheet->graph.nodes[nodeIndex]
                                    .reducedTypes[socketIndex] = otherMeta.type;

                                // But wait... is it the same as the rest of the
                                // types???
                                if (finalType != TYPE_NONE &&
                                    otherMeta.type != finalType) {
                                    Node node = sheet->graph.nodes[nodeIndex];
                                    d_error_compiler_push(
                                        "All inputs in bitwise operators must "
                                        "be of the same type",
                                        sheet->filePath, node.lineNum, true);
                                    allSame = false;
                                }

                                finalType = otherMeta.type;
                            } else {
                                reducedAllInputs = false;
                            }
                        }
                    }
                } else if (finalType != TYPE_NONE) {
                    sheet->graph.nodes[nodeIndex].reducedTypes[socketIndex] =
                        finalType;
                }
            }

            // If all of them are not the same, we can't keep reducing and hope
            // that they do become the same. It's a lost cause :(
            if (reducedAllInputs || !allSame) {
                nodeReduced[nodeIndex] = true;
            }
            break;

        // For comparison operators, we want the inputs to be the same. The
        // output is always a boolean. The user is allowed to mix integer and
        // float inputs, but if one is a string, the other has to be too.
        case CORE_EQUAL:
        case CORE_LESS_THAN:
        case CORE_LESS_THAN_OR_EQUAL:
        case CORE_MORE_THAN:
        case CORE_MORE_THAN_OR_EQUAL:
        case CORE_NOT_EQUAL:;
            bool hasNumberInput = false;
            bool hasStringInput = false;
            bool hasBoolInput   = false;

            for (size_t socketIndex = 0; socketIndex < numSockets;
                 socketIndex++) {
                socket.socketIndex = socketIndex;
                SocketMeta meta    = d_get_socket_meta(sheet->graph, socket);

                if (meta.type != TYPE_EXECUTION) {
                    if (d_is_input_socket(sheet->graph, socket)) {
                        // If we know it's a Number anyway (from a literal
                        // argument)...
                        if ((meta.type & TYPE_NUMBER) != 0) {
                            hasNumberInput = true;
                        }
                        // Or if we know it's a String anyway...
                        else if (meta.type == TYPE_STRING) {
                            hasStringInput = true;
                        }
                        // Or if we know it's a Boolean anyway...
                        else if (meta.type == TYPE_BOOL) {
                            hasBoolInput = true;
                        }
                        // If it's an Integer literal, we don't care about it.
                        // It's already reduced. So, we need to check
                        // connections.
                        else {
                            int wireIndex =
                                d_wire_find_first(sheet->graph, socket);

                            if (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
                                NodeSocket otherSide =
                                    sheet->graph.wires[wireIndex].socketTo;

                                SocketMeta otherMeta =
                                    d_get_socket_meta(sheet->graph, otherSide);

                                // Is this socket reduced?
                                if (IS_TYPE_REDUCED(otherMeta.type)) {
                                    // Great! We can set this socket's type to
                                    // be discrete!
                                    sheet->graph.nodes[nodeIndex]
                                        .reducedTypes[socketIndex] =
                                        otherMeta.type;

                                    if ((otherMeta.type & TYPE_NUMBER) != 0) {
                                        hasNumberInput = true;
                                    } else if (otherMeta.type == TYPE_STRING) {
                                        hasStringInput = true;
                                    } else if (otherMeta.type == TYPE_BOOL) {
                                        hasBoolInput = true;
                                    }
                                } else {
                                    reducedAllInputs = false;
                                }
                            }
                        }
                    } else {
                        outputSocket = socket;
                    }
                }
            }

            // If they've mixed data types... remind them of their dirty deeds.
            if (hasNumberInput && hasStringInput) {
                Node node = sheet->graph.nodes[nodeIndex];
                d_error_compiler_push("Comparison operators cannot compare "
                                      "between numbers and strings",
                                      sheet->filePath, node.lineNum, true);
            }

            if (hasNumberInput && hasBoolInput) {
                Node node = sheet->graph.nodes[nodeIndex];
                d_error_compiler_push("Comparison operators cannot compare "
                                      "between numbers and booleans",
                                      sheet->filePath, node.lineNum, true);
            }

            if (hasStringInput && hasBoolInput) {
                Node node = sheet->graph.nodes[nodeIndex];
                d_error_compiler_push("Comparison operators cannot compare "
                                      "between strings and booleans",
                                      sheet->filePath, node.lineNum, true);
            }

            // If we've gotten all of our inputs reduced, we can
            // say we are fully reduced now.
            if (reducedAllInputs) {
                nodeReduced[nodeIndex] = true;
            }

            break;

        // For the Ternary operator, the two value inputs must
        // be the same, and the output must match the type of
        // the inputs.
        case CORE_TERNARY:;
            DType inputType     = TYPE_NONE;
            bool inputsSameType = true;

            for (size_t socketIndex = 1; socketIndex < numSockets;
                 socketIndex++) {
                socket.socketIndex = socketIndex;
                SocketMeta meta    = d_get_socket_meta(sheet->graph, socket);

                if (d_is_input_socket(sheet->graph, socket)) {

                    // If we already know the type (since) the input could be a
                    // literal...
                    if (IS_TYPE_REDUCED(meta.type)) {

                        // Before we replace inputType, if it has already been
                        // set, then we need to check our type is the type that
                        // is already there.
                        if (inputType != TYPE_NONE && meta.type != inputType) {
                            Node node = sheet->graph.nodes[nodeIndex];
                            d_error_compiler_push("Value inputs in a Ternary "
                                                  "operator must be of the "
                                                  "same type",
                                                  sheet->filePath, node.lineNum,
                                                  true);
                            inputsSameType = false;
                        }

                        inputType = meta.type;
                    }

                    // So we need to check the connection.
                    else {
                        int wireIndex = d_wire_find_first(sheet->graph, socket);

                        if (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
                            NodeSocket otherSide =
                                sheet->graph.wires[wireIndex].socketTo;

                            SocketMeta otherMeta =
                                d_get_socket_meta(sheet->graph, otherSide);

                            // Is this socket reduced?
                            if (IS_TYPE_REDUCED(otherMeta.type)) {

                                // Great! We can set this socket's type to be
                                // discrete!
                                sheet->graph.nodes[nodeIndex]
                                    .reducedTypes[socketIndex] = otherMeta.type;

                                // But wait... is it the same as the rest of the
                                // types???
                                if (inputType != TYPE_NONE &&
                                    otherMeta.type != inputType) {
                                    Node node = sheet->graph.nodes[nodeIndex];
                                    d_error_compiler_push(
                                        "Value inputs in a Ternary operator "
                                        "must be of the same type",
                                        sheet->filePath, node.lineNum, true);
                                    inputsSameType = false;
                                }
                            } else {
                                reducedAllInputs = false;
                            }
                        }
                    }

                } else if (inputType != TYPE_NONE) {
                    sheet->graph.nodes[nodeIndex].reducedTypes[socketIndex] =
                        inputType;
                }
            }

            // If we've gotten all of our inputs reduced, we can say we are
            // fully reduced now, or if the inputs are not the same, we can't
            // make them the same, so we have to stop.
            if (reducedAllInputs || !inputsSameType) {
                nodeReduced[nodeIndex] = true;
            }
            break;

        default:
            nodeReduced[nodeIndex] = true;
            break;
    }
}

/**
 * \fn void d_semantic_reduce_types(Sheet *sheet)
 * \brief Take the connections of a sheet which may have "vague" connections,
 * and reduce them to unique types with the information we have.
 *
 * e.g. If `Multiply` has at least one `Float` input, the output must be a
 * `Float`.
 *
 * \param sheet The sheet to reduce the types on.
 */
void d_semantic_reduce_types(Sheet *sheet) {
    // An array of booleans to say whether we have reduced the node with the
    // same index.
    bool *nodeReduced = (bool *)d_malloc(sheet->graph.numNodes * sizeof(bool));
    for (size_t i = 0; i < sheet->graph.numNodes; i++) {
        nodeReduced[i] = false;
    }

    bool allReduced = false;

    while (!allReduced) {
        VERBOSE(5, "Beginning a pass of reducing nodes...\n");

        bool neededToReduce = false;

        for (size_t nodeIndex = 0; nodeIndex < sheet->graph.numNodes;
             nodeIndex++) {
            bool isReduced = nodeReduced[nodeIndex];

            // We haven't reduced this node yet.
            if (!isReduced) {
                neededToReduce = true;
                const char *name =
                    d_get_node_definition(sheet->graph, nodeIndex)->name;

                VERBOSE(5, "Reducing node #%zu (%s)... ", nodeIndex, name)

                size_t numInputs  = d_node_num_inputs(sheet->graph, nodeIndex);
                size_t numOutputs = d_node_num_outputs(sheet->graph, nodeIndex);
                size_t numSockets = numInputs + numOutputs;

                // Is it a core function?
                const CoreFunction coreFunc = d_core_find_name(name);

                if ((int)coreFunc > -1) {
                    reduce_core_node(sheet, coreFunc, nodeIndex, numSockets,
                                     nodeReduced);
                } else {
                    // TODO: Reduce functions from other sheets.
                    nodeReduced[nodeIndex] = true;
                }

                if (VERBOSE_LEVEL >= 5) {
                    if (nodeReduced[nodeIndex])
                        printf("done.\n");
                    else
                        printf("not yet able to reduce.\n");
                }
            }
        }

        if (!neededToReduce) {
            allReduced = true;
        }
    }

    VERBOSE(5, "Done reducing nodes.\n");

    free(nodeReduced);
}

/*
    void check_loop(Sheet* sheet, SheetNode* start, SheetNode** pathArray,
   size_t pathLength) A recursive function to check if a sheet has a loop.

    Returns: If a loop was found.

    Sheet* sheet: In case we error, say where we errored from.
    SheetNode* start: The recursive argument, where to start
    checking in the sheet.
    SheetNode** pathArray: The array to write our current path
    to. Used to see if we visited the same node twice.
    size_t pathLength: The length of the current path. Increases
    by 1 each recursion.
*/
/**
 * \fn static void check_loop(Sheet *sheet, size_t start, size_t *pathArray,
 *                            size_t pathLength)
 * \brief A recursive function to check if a sheet has a loop.
 *
 * \param sheet In case we error, say where we errored from.
 * \param start The index of the node to start from.
 * \param pathArray The array to write our current path to. Used to see if we
 * visit the same node twice.
 * \param pathLength The length of the current path.
 */
static void check_loop(Sheet *sheet, size_t start, size_t *pathArray,
                       size_t pathLength) {
    // Firstly, check if we're already in the path.
    for (size_t i = 0; i < pathLength - 1; i++) {
        if (start == pathArray[i]) {
            VERBOSE(5, "FOUND LOOP\n")

            Node node = sheet->graph.nodes[start];

            ERROR_COMPILER(sheet->filePath, node.lineNum, true,
                           "Detected loop entering node %s",
                           node.definition->name);
            return;
        }
    }

    // Set ourselves into the path array.
    pathArray[pathLength - 1] = start;

    // Now, go through all of our output connections,
    // and call this function on them.
    size_t numInputs  = d_node_num_inputs(sheet->graph, start);
    size_t numOutputs = d_node_num_outputs(sheet->graph, start);

    // Start from the first output socket.
    for (size_t i = numInputs; i < numInputs + numOutputs; i++) {
        NodeSocket socket;
        socket.nodeIndex   = start;
        socket.socketIndex = i;

        int wireIndex = d_wire_find_first(sheet->graph, socket);

        while (IS_WIRE_FROM(sheet->graph, wireIndex, socket)) {
            size_t nextNodeIndex =
                sheet->graph.wires[wireIndex].socketTo.nodeIndex;
            Node nextNode = sheet->graph.nodes[nextNodeIndex];

            VERBOSE(5, "ENTER %s LINE %zu\n", nextNode.definition->name,
                    nextNode.lineNum)

            check_loop(sheet, nextNodeIndex, pathArray, pathLength + 1);

            VERBOSE(5, "EXIT %s LINE %zu\n", nextNode.definition->name,
                    nextNode.lineNum)

            wireIndex++;
        }
    }
}

/**
 * \fn void d_semantic_detect_loops(Sheet *sheet)
 * \brief After a sheet has been connected in `d_semantic_scan_nodes`, go
 * through the sheet and see if there are any loops.
 *
 * "Loops are bad, and should be given coal at christmas."
 *
 * In retrospect, this was a bad quote. Instead, give it something that won't
 * cause climate change, like a really cheap sticker.
 *
 * While we are here, we also check to see if there are any redundant nodes.
 *
 * \param sheet The connected sheet to check for loops.
 */
void d_semantic_detect_loops(Sheet *sheet) {
    // We need to find all of the nodes in the sheet that start
    // a path, i.e. nodes with no arguments.
    // We then try every single possible path from that node.
    // If we end up visiting a node we've already visited on our
    // journey, error.

    if (sheet->graph.nodes != NULL && sheet->graph.numNodes > 0) {
        // We can't go on a journey that is bigger than the number
        // of nodes (without looping)
        size_t *path =
            (size_t *)d_malloc(sheet->graph.numNodes * sizeof(size_t));

        // Find nodes with no inputs (except name sockets).
        for (size_t i = 0; i < sheet->graph.numNodes; i++) {
            const NodeDefinition *nodeDef =
                d_get_node_definition(sheet->graph, i);
            bool hasInputs = false;

            for (size_t j = 0; j < d_definition_num_inputs(nodeDef); j++) {
                if (nodeDef->sockets[j].type != TYPE_NAME) {
                    hasInputs = true;
                    break;
                }
            }

            // If we have no inputs, find all paths from here.
            if (!hasInputs) {
                VERBOSE(5, "- Checking paths from node #%zu (%s)...\n", i,
                        nodeDef->name);

                check_loop(sheet, i, path, 1);
            }
        }

        free(path);
    }
}

/**
 * \fn void d_semantic_scan(Sheet *sheet, SyntaxNode *root)
 * \brief Perform Semantic Analysis on a syntax tree.
 *
 * \param sheet The sheet to put everything into.
 * \param root The *valid* syntax tree to scan everything from.
 */
void d_semantic_scan(Sheet *sheet, SyntaxNode *root) {
    VERBOSE(1, "-- Scanning properties...\n")
    d_semantic_scan_properties(sheet, root);

    VERBOSE(1, "-- Scanning nodes...\n")
    d_semantic_scan_nodes(sheet, root);

    VERBOSE(1, "-- Reducing data types...\n")
    d_semantic_reduce_types(sheet);

    // TODO: Optimise to see if we can get away with setting the types or
    // literals array in the sheet nodes to NULL if they are identical to their
    // definitions.

    VERBOSE(1, "-- Detecting loops...\n")
    d_semantic_detect_loops(sheet);
}
