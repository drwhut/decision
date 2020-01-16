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

#include "dsemantic.h"

#include "dcfunc.h"
#include "decision.h"
#include "derror.h"
#include "dmalloc.h"
#include "dsheet.h"
#include "dsyntax.h"

#include <stdlib.h>
#include <string.h>

/*
    Fair warning, this file is A MESS.
    TODO: Introduce macros, split up code into more functions?
*/

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
    void add_name_definition(AllNameDefinitions* all, NameDefinition definition)
    Add a name definition to the array of all name definitions.
    Similar to the LIST_PUSH macro in dsheet.c

    AllNameDefinitions* all: The list of name definitions to add onto.
    NameDefinition definition: The name definition to add on.
*/
static void add_name_definition(AllNameDefinitions *all,
                                NameDefinition definition) {
    size_t newNumDefinitions = all->numDefinitions + 1;
    if (newNumDefinitions == 1)
        all->definitions = (NameDefinition *)d_malloc(sizeof(NameDefinition));
    else
        all->definitions = (NameDefinition *)d_realloc(
            all->definitions, newNumDefinitions * sizeof(NameDefinition));
    all->definitions[all->numDefinitions++] = definition;
}

/*
    void get_name_definitions_recursive(struct _sheet* sheet, const char* name,
   AllNameDefinitions* state) Recursively find a definition of a name.

    Sheet* sheet: The sheet to start looking from.
    const char* name: The name to query.
    AllNameDefinitions* state: The state of the recursion. This is added onto
   when we find a name definition.
*/
static void get_name_definitions_recursive(Sheet *sheet, const char *name,
                                           AllNameDefinitions *state) {
    VERBOSE(5, "Checking sheet %s for name %s...\n", sheet->filePath, name)

    // First, check to see if it is a core function.
    CoreFunction coreFunc = d_core_find_name(name);

    if ((int)coreFunc > -1) {
        NameDefinition definition;
        definition.sheet     = sheet;
        definition.type      = NAME_CORE;
        definition.coreFunc  = coreFunc;
        definition.variable  = NULL;
        definition.function  = NULL;
        definition.cFunction = NULL;

        add_name_definition(state, definition);

        VERBOSE(5, "Core function: YES\n")
    } else {
        VERBOSE(5, "Core function: NO\n")
    }

    // Maybe it's a variable name?
    if (sheet->variables != NULL && sheet->numVariables > 0) {
        size_t hits = 0;
        for (size_t i = 0; i < sheet->numVariables; i++) {
            if (strcmp(name, sheet->variables[i].name) == 0) {
                hits++;

                NameDefinition definition;
                definition.sheet     = sheet;
                definition.type      = NAME_VARIABLE;
                definition.coreFunc  = -1;
                definition.variable  = &(sheet->variables[i]);
                definition.function  = NULL;
                definition.cFunction = NULL;

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "Variable: %zu\n", hits)
    } else {
        VERBOSE(
            5,
            "Not checking if a variable, no variables defined in this sheet.\n")
    }

    // Maybe, just maybe... it's a function name?
    if (sheet->functions != NULL && sheet->numFunctions > 0) {
        size_t hits = 0;
        for (size_t i = 0; i < sheet->numFunctions; i++) {
            if (strcmp(name, sheet->functions[i].name) == 0) {
                hits++;

                NameDefinition definition;
                definition.sheet     = sheet;
                definition.type      = NAME_FUNCTION;
                definition.coreFunc  = -1;
                definition.variable  = NULL;
                definition.function  = &(sheet->functions[i]);
                definition.cFunction = NULL;

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "Function: %zu\n", hits)
    } else {
        VERBOSE(
            5,
            "Not checking if a function, no functions defined in this sheet.\n")
    }

    // It could also be a C function.
    size_t numCFunctions = d_get_num_c_functions();

    if (numCFunctions > 0) {
        const CFunction *cFunctionList = d_get_c_functions();

        size_t hits = 0;
        for (size_t i = 0; i < numCFunctions; i++) {
            if (strcmp(cFunctionList[i].name, name) == 0) {
                hits++;

                NameDefinition definition;
                definition.sheet     = sheet;
                definition.type      = NAME_CFUNCTION;
                definition.coreFunc  = -1;
                definition.variable  = NULL;
                definition.function  = NULL;
                definition.cFunction = (CFunction *)cFunctionList + i;

                add_name_definition(state, definition);
            }
        }

        VERBOSE(5, "C Function: %zu\n", hits);
    } else {
        VERBOSE(5,
                "Not checking if a C function, no C function are defined.\n");
    }

    // Please sir... may I have some more names? (If it isn't a core function)
    if (sheet->includes != NULL && sheet->numIncludes > 0 &&
        (int)coreFunc == -1) {
        for (size_t i = 0; i < sheet->numIncludes; i++) {
            get_name_definitions_recursive(sheet->includes[i], name, state);
        }
    }
}

/**
 * \fn AllNameDefinitions d_semantic_get_name_definitions(Sheet *sheet,
 *                                                        const char *name)
 * \brief Get all of the places where a name is defined, and what the name's
 * type is.
 *
 * We will also check recursively up the includes of sheets.
 *
 * \return An array of NameDefinition.
 *
 * \param sheet The sheet to start looking from.
 * \param name The name to query.
 */
AllNameDefinitions d_semantic_get_name_definitions(struct _sheet *sheet,
                                                   const char *name) {
    VERBOSE(5, "Finding definitions for name %s...\n", name)

    AllNameDefinitions allDefinitions = (AllNameDefinitions){NULL, 0};
    get_name_definitions_recursive(sheet, name, &allDefinitions);

    VERBOSE(5, "Found %zu results for name %s.\n",
            allDefinitions.numDefinitions, name)

    return allDefinitions;
}

/**
 * \fn bool d_semantic_select_name_definition(const char *name,
 *                                            AllNameDefinitions allDefinitions,
 *                                            NameDefinition *selection)
 * \brief Given a set of definitions, select the one the user intended, give
 * the name of the node.
 *
 * \return If a definition was selected.
 *
 * \param name The name that decides the definition to use.
 * \param allDefinitions The set of definitions to choose from.
 * \param selection A pointer whose value is set to the definition if it was
 * selected, i.e. if we return `true`. If `false` is returned, do not trust
 * this value.
 */
bool d_semantic_select_name_definition(const char *name,
                                       AllNameDefinitions allDefinitions,
                                       NameDefinition *selection) {
    // TODO: Once you get around to linking other sheets, use name components
    // to decide properly. For now, we're just picking the first one from the
    // list.

    if (allDefinitions.numDefinitions > 0) {
        NameDefinition definition = allDefinitions.definitions[0];

        *selection = definition;

        return true;
    }

    return false;
}

/**
 * \fn void d_semantic_free_name_definitions(AllNameDefinitions definitions)
 * \brief Free an `AllNameDefinitions` struct. It should not be used after it
 * has been freed.
 *
 * \param definitions The structure to free.
 */
void d_semantic_free_name_definitions(AllNameDefinitions definitions) {
    free(definitions.definitions);
}

/**
 * \fn NodeTrueProperties d_semantic_get_node_properties(
 *     Sheet *sheet,
 *     const char *name,
 *     size_t lineNum,
 *     const char *funcName,
 *     NameDefinition *definition)
 * \brief Get a node's true properties (e.g. input and output types) by it's
 * name.
 *
 * \return A NodeTrueProperties object with malloc'd elements.
 *
 * \param sheet The sheet the node is a part of.
 * \param name The name of the node.
 * \param lineNum In case we error, say where we errored from.
 * \param funcName If the name is Return, we need the function name so we can
 * get the correct return values.
 * \param definition A convenience reference that is set to the name definition
 * of the name or funcName given. This can be used to set the definition of a
 * node. Should not be used if the NodeTrueProperties struct has isDefined set
 * to false.
 */
NodeTrueProperties d_semantic_get_node_properties(struct _sheet *sheet,
                                                  const char *name,
                                                  size_t lineNum,
                                                  const char *funcName,
                                                  NameDefinition *definition) {
    VERBOSE(5, "Getting node properties of node %s on line %zu in %s...\n",
            name, lineNum, sheet->filePath)

    NodeTrueProperties out =
        (NodeTrueProperties){false, false, true, NULL, 0, NULL, 0};

    // Most cases use this to build up the output types.
    DType *outputTypes = NULL;

    // First, let's check if it's a special function like Start.
    // We do this now so we don't need to search all of our includes
    // for this function.
    if (strcmp(name, "Start") == 0) {
        out.isDefined  = true;
        out.numOutputs = 1;

        outputTypes    = (DType *)d_malloc(sizeof(DType));
        outputTypes[0] = TYPE_EXECUTION;

        out.outputTypes = (const DType *)outputTypes;
    } else if (strcmp(name, "Return") == 0) {
        // For return, we need to make sure funcName is the name
        // of a function that is defined in the same sheet!
        if (funcName != NULL) {
            AllNameDefinitions funcDefinitions =
                d_semantic_get_name_definitions(sheet, funcName);
            if (d_semantic_select_name_definition(funcName, funcDefinitions,
                                                  definition)) {
                out.isDefined = true;

                if (definition->sheet == sheet) {
                    SheetFunction *func = definition->function;

                    bool addExec = func->isSubroutine;

                    out.numInputs = (long)(1 + addExec + func->numReturns);

                    DType *inputTypes = NULL;

                    if (out.numInputs > 0)
                        inputTypes =
                            (DType *)d_malloc(out.numInputs * sizeof(DType));

                    if (inputTypes != NULL)
                        inputTypes[0] = TYPE_NAME; // The name of the function.

                    if (addExec && inputTypes != NULL)
                        inputTypes[1] = TYPE_EXECUTION;

                    if (inputTypes != NULL)
                        for (size_t j = 0; j < func->numReturns; j++) {
                            inputTypes[j + 1 + addExec] =
                                func->returns[j].dataType;
                        }

                    out.inputTypes = (const DType *)inputTypes;
                } else {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Return node for function %s that is not "
                                   "defined on the same sheet",
                                   funcName);
                }
            } else {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Return call for undefined function %s",
                               funcName);
            }

            d_semantic_free_name_definitions(funcDefinitions);
        } else {
            d_error_compiler_push("Return call but function name not found",
                                  sheet->filePath, lineNum, true);
        }
    } else if (strcmp(name, "Define") == 0) {
        // For define, we need to make sure funcName is the name
        // of a function that is defined in the same sheet!
        if (funcName != NULL) {
            AllNameDefinitions funcDefinitions =
                d_semantic_get_name_definitions(sheet, funcName);
            if (d_semantic_select_name_definition(funcName, funcDefinitions,
                                                  definition)) {
                out.isDefined = true;

                if (definition->sheet == sheet) {
                    SheetFunction *func = definition->function;

                    bool addExec = func->isSubroutine;

                    out.numInputs  = 1;
                    out.numOutputs = (long)(addExec + func->numArguments);

                    DType *inputTypes = (DType *)d_malloc(sizeof(DType));

                    if (out.numOutputs > 0)
                        outputTypes =
                            (DType *)d_malloc(out.numOutputs * sizeof(DType));

                    if (inputTypes != NULL)
                        inputTypes[0] = TYPE_NAME; // The name of the function.

                    if (addExec && outputTypes != NULL)
                        outputTypes[0] = TYPE_EXECUTION;

                    if (outputTypes != NULL)
                        for (size_t j = 0; j < func->numArguments; j++) {
                            outputTypes[j + addExec] =
                                func->arguments[j].dataType;
                        }

                    out.inputTypes  = (const DType *)inputTypes;
                    out.outputTypes = (const DType *)outputTypes;
                } else {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Define node for function %s that is not "
                                   "defined on the same sheet",
                                   funcName);
                }
            } else {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Define call for undefined function %s",
                               funcName);
            }

            d_semantic_free_name_definitions(funcDefinitions);
        } else {
            d_error_compiler_push("Define call but function name not found",
                                  sheet->filePath, lineNum, true);
        }
    } else {
        // Let's find out if the name we're looking for exists, first.
        AllNameDefinitions nameDefinitions =
            d_semantic_get_name_definitions(sheet, name);
        if (d_semantic_select_name_definition(name, nameDefinitions,
                                              definition)) {
            out.isDefined = true;

            switch (definition->type) {
                case NAME_CORE:;

                    // This bit's simple, all of the info we need is in dcore.h
                    out._mallocd = false;
                    out.numInputs =
                        d_core_num_input_sockets(definition->coreFunc);
                    out.numOutputs =
                        d_core_num_output_sockets(definition->coreFunc);

                    out.inputTypes =
                        d_core_get_input_types(definition->coreFunc);
                    out.outputTypes =
                        d_core_get_output_types(definition->coreFunc);

                    break;

                // Specifically, the GETTER of a variable.
                case NAME_VARIABLE:;

                    // We just need to look up the variable's data type.
                    SheetVariable *var = definition->variable;

                    out.numOutputs = 1;

                    outputTypes    = (DType *)d_malloc(sizeof(DType));
                    outputTypes[0] = var->dataType;

                    out.outputTypes = (const DType *)outputTypes;

                    break;

                // Specifically, the calling of a function.
                case NAME_FUNCTION:;

                    SheetFunction *func = definition->function;
                    bool addExec        = func->isSubroutine;

                    out.numInputs  = (long)(addExec + func->numArguments);
                    out.numOutputs = (long)(addExec + func->numReturns);

                    outputTypes       = NULL;
                    DType *inputTypes = NULL;

                    if (out.numInputs > 0)
                        inputTypes =
                            (DType *)d_malloc(out.numInputs * sizeof(DType));

                    if (out.numOutputs > 0)
                        outputTypes =
                            (DType *)d_malloc(out.numOutputs * sizeof(DType));

                    if (addExec) {
                        if (inputTypes != NULL)
                            inputTypes[0] = TYPE_EXECUTION;

                        if (outputTypes != NULL)
                            outputTypes[0] = TYPE_EXECUTION;
                    }

                    if (inputTypes != NULL)
                        for (size_t j = 0; j < func->numArguments; j++) {
                            inputTypes[j + addExec] =
                                func->arguments[j].dataType;
                        }

                    if (outputTypes != NULL)
                        for (size_t j = 0; j < func->numReturns; j++) {
                            outputTypes[j + addExec] =
                                func->returns[j].dataType;
                        }

                    out.inputTypes  = (const DType *)inputTypes;
                    out.outputTypes = (const DType *)outputTypes;

                    break;

                // Specifically, the calling of a C function.
                case NAME_CFUNCTION:;

                    const CFunction *cFunction = definition->cFunction;

                    // This one is simple, all the info we need is in the
                    // CFunction struct.
                    out._mallocd = false;

                    out.inputTypes = cFunction->inputs;
                    out.numInputs  = (long)cFunction->numInputs;

                    out.outputTypes = cFunction->outputs;
                    out.numOutputs  = (long)cFunction->numOutputs;

                    break;

                default:
                    d_error_compiler_push("Name definition is not that of a "
                                          "core function, variable, function "
                                          "or C function",
                                          sheet->filePath, lineNum, true);
                    break;
            }
        }

        d_semantic_free_name_definitions(nameDefinitions);
    }

    return out;
}

/**
 * \fn bool d_semantic_is_execution_node(NodeTrueProperties trueProperties)
 * \brief Given the properties of a node, decide if it's an execution node or
 * not.
 *
 * \return If this theoretical node is an execution node.
 *
 * \param trueProperties The properties to query.
 */
bool d_semantic_is_execution_node(NodeTrueProperties trueProperties) {
    // The way we determine if it's an execution node is if it has at least one
    // execution socket.

    if (trueProperties.numInputs == -1) {
        if (trueProperties.inputTypes[0] == TYPE_EXECUTION)
            return true;
    } else {
        for (long i = 0; i < trueProperties.numInputs; i++)
            if (trueProperties.inputTypes[i] == TYPE_EXECUTION)
                return true;
    }

    if (trueProperties.numOutputs == -1) {
        if (trueProperties.outputTypes[0] == TYPE_EXECUTION)
            return true;
    } else {
        for (long i = 0; i < trueProperties.numOutputs; i++)
            if (trueProperties.outputTypes[i] == TYPE_EXECUTION)
                return true;
    }

    return false;
}

/**
 * \fn void d_semantic_free_true_properties(NodeTrueProperties trueProperties)
 * \brief Free malloc'd elements of a NodeTrueProperties object.
 *
 * \param trueProperties The object to free elements of.
 */
void d_semantic_free_true_properties(NodeTrueProperties trueProperties) {
    if (trueProperties._mallocd) {
        if (trueProperties.inputTypes != NULL && trueProperties.numInputs > 0) {
            free((void *)trueProperties.inputTypes);
        }

        if (trueProperties.outputTypes != NULL &&
            trueProperties.numOutputs > 0) {
            free((void *)trueProperties.outputTypes);
        }
    }

    trueProperties.inputTypes  = NULL;
    trueProperties.outputTypes = NULL;

    trueProperties.numInputs  = 0;
    trueProperties.numOutputs = 0;
}

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

    if (argList.numArgs < 2) {
        d_error_compiler_push("Variable property needs at least 2 arguments.",
                              sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 3) {
        d_error_compiler_push("Variable property needs at most 3 arguments.",
                              sheet->filePath, lineNum, true);
    } else {
        if (argList.numArgs == 2) {
            d_error_compiler_push(
                "No default value specified in Variable property",
                sheet->filePath, lineNum, false);
            defaultValue = false;
        }

        PropertyArgument variableNameArg = argList.args[0];
        PropertyArgument dataTypeArg     = argList.args[1];
        PropertyArgument defaultValueArg;
        defaultValueArg.type      = 0;
        defaultValueArg.data.name = NULL;

        if (defaultValue)
            defaultValueArg = argList.args[2];

        SheetVariable newVariable;
        newVariable.name     = NULL;
        newVariable.dataType = TYPE_NONE;

        if (PROPERTY_ARGUMENT_NAME_DEFINED(variableNameArg)) {
            const char *varName = variableNameArg.data.name;

            // We need to check if the variable name is already defined.
            bool alreadyDefined = false;

            for (size_t i = 0; i < sheet->numVariables; i++) {
                if (strcmp(varName, sheet->variables[i].name) == 0) {
                    alreadyDefined = true;
                    break;
                }
            }

            if (!alreadyDefined)
                newVariable.name = varName;
            else
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "Variable %s already defined", varName);
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
                    newVariable.dataType = finalType;
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

                switch (newVariable.dataType) {
                    case TYPE_INT:
                        if (defaultValueToken->type == TK_INTEGERLITERAL) {
                            newVariable.defaultValue.integerValue =
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
                            newVariable.defaultValue.floatValue =
                                defaultValueToken->data.floatValue;
                        } else if (defaultValueToken->type ==
                                   TK_INTEGERLITERAL) {
                            // If we've been given an integer, that's fine,
                            // we can easily convert it.
                            newVariable.defaultValue.floatValue =
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
                            newVariable.defaultValue.stringValue =
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
                            newVariable.defaultValue.booleanValue =
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
            switch (newVariable.dataType) {
                case TYPE_INT:
                    newVariable.defaultValue.integerValue = 0;
                    break;

                case TYPE_FLOAT:
                    newVariable.defaultValue.floatValue = 0.0;
                    break;

                case TYPE_STRING:
                    newVariable.defaultValue.stringValue = "";
                    break;

                case TYPE_BOOL:
                    newVariable.defaultValue.booleanValue = false;
                    break;

                default:
                    break;
            }
        }

        // We've gotten the variable, now we add it to the sheet if everything
        // went ok.
        if (newVariable.name != NULL && newVariable.dataType != TYPE_NONE) {
            d_sheet_add_variable(sheet, newVariable);
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

static void add_property_Function(Sheet *sheet, size_t lineNum,
                                  PropertyArgumentList argList) {
    if (argList.numArgs == 1) {
        PropertyArgument arg = argList.args[0];

        if (PROPERTY_ARGUMENT_NAME_DEFINED(arg)) {
            const char *name = arg.data.name;

            // Does a function with this name already exist?
            for (size_t i = 0; i < sheet->numFunctions; i++) {
                SheetFunction func = sheet->functions[i];

                if (strcmp(func.name, name) == 0) {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Function %s is already defined.", name);
                    return;
                }
            }

            d_sheet_create_function(sheet, name, false);
        } else {
            d_error_compiler_push("Function property argument is not a name",
                                  sheet->filePath, lineNum, true);
        }
    } else {
        d_error_compiler_push("1 argument needed for Function property",
                              sheet->filePath, lineNum, true);
    }
}

static void add_property_Subroutine(Sheet *sheet, size_t lineNum,
                                    PropertyArgumentList argList) {
    if (argList.numArgs == 1) {
        PropertyArgument arg = argList.args[0];

        if (PROPERTY_ARGUMENT_NAME_DEFINED(arg)) {
            const char *name = arg.data.name;

            // Does a function with this name already exist?
            for (size_t i = 0; i < sheet->numFunctions; i++) {
                SheetFunction func = sheet->functions[i];

                if (strcmp(func.name, name) == 0) {
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Function %s is already defined.", name);
                    return;
                }
            }

            d_sheet_create_function(sheet, name, true);
        } else {
            d_error_compiler_push("Subroutine property argument is not a name",
                                  sheet->filePath, lineNum, true);
        }
    } else {
        d_error_compiler_push("1 argument needed for Subroutine property",
                              sheet->filePath, lineNum, true);
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

    const char *argName = NULL;
    LexData defaultValue;
    defaultValue.integerValue = 0;

    if (argList.numArgs < 2) {
        d_error_compiler_push(
            "FunctionInput property needs at least 2 arguments",
            sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 4) {
        d_error_compiler_push(
            "FunctionInput property needs at most 4 arguments", sheet->filePath,
            lineNum, true);
    } else {
        funcArg = argList.args[0];

        if (argList.numArgs == 2) {
            typeArg = argList.args[1];
        } else {
            nameArg = argList.args[1];
            typeArg = argList.args[2];

            if (argList.numArgs == 4) {
                defaultArg = argList.args[3];
            }
        }

        if (PROPERTY_ARGUMENT_NAME_DEFINED(funcArg)) {
            const char *funcName = funcArg.data.name;

            if (PROPERTY_ARGUMENT_TYPE_DEFINED(typeArg) &&
                PROPERTY_ARGUMENT_TYPE_IS_VAR(typeArg)) {
                DType argType = typeArg.data.dataType;

                // TODO: Add support for vague data types in functions.
                if (d_type_is_vague(argType)) {
                    d_error_compiler_push("Vague data types in functions is "
                                          "not currently supported",
                                          sheet->filePath, lineNum, true);
                }

                if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
                    argName = nameArg.data.name;
                } else if (argList.numArgs >= 3) {
                    ERROR_COMPILER(sheet->filePath, lineNum, false,
                                   "FunctionInput name argument (argument "
                                   "2/%zu) is not a name, ignoring",
                                   argList.numArgs);
                }

                if (defaultArg.type == STX_literal) {
                    LexToken *literal   = defaultArg.data.literal;
                    DType typeOfLiteral = TYPE_FROM_LEX_LITERAL(literal->type);

                    if ((typeOfLiteral & argType) != TYPE_NONE) {
                        // The data types match, we can set the default value.
                        defaultValue = literal->data;
                    } else if (typeOfLiteral == TYPE_INT &&
                               argType == TYPE_FLOAT) {
                        // We can easily convert an integer into a float.
                        literal->data.floatValue =
                            (dfloat)literal->data.integerValue;
                        defaultValue = literal->data;
                    } else {
                        ERROR_COMPILER(
                            sheet->filePath, lineNum, false,
                            "FunctionInput default value argument data type %s "
                            "does not match input data type %s",
                            d_type_name(typeOfLiteral), d_type_name(argType));
                    }
                } else if (argList.numArgs == 4) {
                    d_error_compiler_push(
                        "FunctionInput default value argument (argument 4/4) "
                        "is not a literal, ignoring",
                        sheet->filePath, lineNum, false);
                }

                // Now we've organised the arguments, we can add the argument!
                d_sheet_function_add_argument(sheet, funcName, argName, argType,
                                              defaultValue);
            } else {
                d_error_compiler_push(
                    "FunctionInput data type argument is invalid",
                    sheet->filePath, lineNum, true);
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "FunctionInput data type argument (argument "
                               "%d/%zu) is invalid",
                               (argList.numArgs == 2) ? 2 : 3, argList.numArgs);
            }

            // Free this instance of the function name, since it *should* have
            // already been malloc'd elsewhere, and that version will go into
            // the SheetFunction struct.
            free((char *)funcName);
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionInput function argument (argument 1/%zu) "
                           "is not a name",
                           argList.numArgs);
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

    const char *retName = NULL;

    if (argList.numArgs < 2) {
        d_error_compiler_push(
            "FunctionOutput property needs at least 2 arguments",
            sheet->filePath, lineNum, true);
    } else if (argList.numArgs > 3) {
        d_error_compiler_push(
            "FunctionOutput property needs at most 3 arguments",
            sheet->filePath, lineNum, true);
    } else {
        funcArg = argList.args[0];

        if (argList.numArgs == 2) {
            typeArg = argList.args[1];
        } else {
            nameArg = argList.args[1];
            typeArg = argList.args[2];
        }

        if (PROPERTY_ARGUMENT_NAME_DEFINED(funcArg)) {
            const char *funcName = funcArg.data.name;

            if (PROPERTY_ARGUMENT_TYPE_DEFINED(typeArg) &&
                PROPERTY_ARGUMENT_TYPE_IS_VAR(typeArg)) {
                DType retType = typeArg.data.dataType;

                // TODO: Add support for vague data types in functions.
                if (d_type_is_vague(retType)) {
                    d_error_compiler_push("Vague data types in functions is "
                                          "not currently supported",
                                          sheet->filePath, lineNum, true);
                }

                if (PROPERTY_ARGUMENT_NAME_DEFINED(nameArg)) {
                    retName = nameArg.data.name;
                } else if (argList.numArgs == 3) {
                    d_error_compiler_push(
                        "FunctionOutput name argument (argument 2/3) is not a "
                        "name, ignoring",
                        sheet->filePath, lineNum, false);
                }

                // Now we've organised the arguments, we can add the return
                // value!
                d_sheet_function_add_return(sheet, funcName, retName, retType);
            } else {
                ERROR_COMPILER(sheet->filePath, lineNum, true,
                               "FunctionOutput data type argument (argument "
                               "%d/%zu) is invalid",
                               (argList.numArgs == 2) ? 2 : 3, argList.numArgs);
            }

            // Free this instance of the function name, since it *should* have
            // already been malloc'd elsewhere, and that version will go into
            // the SheetFunction struct.
            free((char *)funcName);
        } else {
            ERROR_COMPILER(sheet->filePath, lineNum, true,
                           "FunctionOutput function argument (argument 1/%zu) "
                           "is not a name",
                           argList.numArgs);
        }
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

                            // Now we have the property name, we can get the
                            // arguments.
                            PropertyArgumentList argList =
                                (PropertyArgumentList){NULL, 0};
                            bool argListMallocd = false;

                            if (node->sibling != NULL) {
                                // STX_propertyCall
                                node = node->sibling;

                                if (node->child != NULL) {
                                    // STX_listOfPropertyArguments
                                    node = node->child;

                                    // STX_propertyArgument
                                    SyntaxNode *argParentNode = node->child;

                                    const size_t numArgs =
                                        d_syntax_get_num_children(node);

                                    if (numArgs > 0) {
                                        argList.numArgs = numArgs;
                                        argList.args =
                                            (PropertyArgument *)d_malloc(
                                                numArgs *
                                                sizeof(PropertyArgument));
                                        argListMallocd = true;

                                        for (size_t i = 0; i < numArgs; i++) {
                                            SyntaxNode *argNode =
                                                argParentNode->child;

                                            SyntaxDefinition definition =
                                                argNode->definition;

                                            PropertyArgument arg;
                                            arg.type      = definition;
                                            arg.data.name = NULL;

                                            switch (definition) {
                                                case STX_TOKEN:;
                                                    if (argNode->info->type ==
                                                        TK_NAME)
                                                        arg.data.name =
                                                            argNode->info->data
                                                                .stringValue;
                                                    break;
                                                case STX_literal:;
                                                    // STX_TOKEN
                                                    SyntaxNode *literalNode =
                                                        argNode->child;
                                                    arg.data.literal =
                                                        literalNode->info;
                                                    break;
                                                case STX_dataType:;
                                                    // STX_dataType
                                                    SyntaxNode *dataTypeNode =
                                                        argNode->child;
                                                    arg.data.dataType =
                                                        TYPE_FROM_LEX(
                                                            dataTypeNode->info
                                                                ->type);
                                                    break;
                                                default:
                                                    d_error_compiler_push(
                                                        "Property arguments "
                                                        "must be either a "
                                                        "name, literal or data "
                                                        "type.",
                                                        sheet->filePath,
                                                        lineNum, true);
                                                    break;
                                            }

                                            argList.args[i] = arg;

                                            argParentNode =
                                                argParentNode->sibling;
                                        }
                                    }
                                }
                            }

                            // What is the name of the property?
                            IF_PROPERTY(Variable)
                            else IF_PROPERTY(Include) else IF_PROPERTY(Function) else IF_PROPERTY(
                                Subroutine) else IF_PROPERTY(FunctionInput) else IF_PROPERTY(FunctionOutput) else {
                                ERROR_COMPILER(sheet->filePath, lineNum, true,
                                               "Unknown property name %s",
                                               propertyName);
                            }

                            if (argListMallocd) {
                                free(argList.args);
                            }

                            // Once we've scanned the property, there's no need
                            // to store the name of the property anymore.
                            free((char *)propertyName);
                        }
                    }
                }
            }
        }
    }

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
 * \fn void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root)
 * \param Sets the nodes of the sheet, given the syntax tree.
 *
 * **NOTE:** This function also sets the connections between the nodes.
 *
 * \param sheet A pointer to the sheet where we want to set the properties.
 * \param root The root node of the syntax tree.
 */
void d_semantic_scan_nodes(Sheet *sheet, SyntaxNode *root) {
    LexData defaultLexData;
    defaultLexData.integerValue = 0;

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
                NameDefinition nameDefinition =
                    (NameDefinition){NULL, -1, -1, NULL, NULL};

                // Get the node's true properties!
                NodeTrueProperties trueProperties =
                    d_semantic_get_node_properties(sheet, nodeName, lineNum,
                                                   funcName, &nameDefinition);

                bool isExecutionNode =
                    d_semantic_is_execution_node(trueProperties);

                if (trueProperties.isDefined) {
                    // Create a SheetNode, and put it into the sheet.
                    SheetNode *sheetNode =
                        d_node_create(nodeName, lineNum, isExecutionNode);
                    d_sheet_add_node(sheet, sheetNode);

                    // Now we've created the SheetNode struct, we should set its
                    // definition from when we checked for the node's true
                    // properties.
                    sheetNode->definition = nameDefinition;

                    // Is it the "Start" node? There's a special pointer in the
                    // sheet for this node.
                    if (strcmp(nodeName, "Start") == 0) {
                        sheet->startNode = sheetNode;

                        // We want to make sure the sheet doesn't have more than
                        // one Start function, so we keep track.
                        sheet->numStarts++;
                    }

                    // Or is it a "Define" node? Functions have a pointer to
                    // the node that defines their functionality.
                    else if (strcmp(nodeName, "Define") == 0) {
                        if (nameDefinition.function != NULL) {
                            nameDefinition.function->defineNode = sheetNode;

                            // There can only be 1 define node for each
                            // function.
                            nameDefinition.function->numDefineNodes++;
                        }
                    }

                    // Or is it a "Return" node? Functions (unlike subroutines)
                    // must have 1 return node.
                    else if (strcmp(nodeName, "Return") == 0) {
                        if (nameDefinition.function != NULL) {
                            nameDefinition.function->lastReturnNode = sheetNode;

                            nameDefinition.function->numReturnNodes++;
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
                        inputArgs = d_syntax_get_all_nodes_with(
                            node, STX_argument, false);

                        for (size_t inputIndex = 0;
                             inputIndex < inputArgs.numOccurances;
                             inputIndex++) {
                            size_t typeIndex = inputIndex;

                            // If there can be an infinite amount of inputs, we
                            // still need to get the data type.
                            if (trueProperties.numInputs == -1) {
                                for (size_t j = 0; j < inputIndex; j++) {
                                    if (trueProperties.inputTypes[j] == 0)
                                        break;

                                    typeIndex = j;
                                }
                            }

                            // Create the socket, and add it to the node.
                            SheetSocket *inputSocket = d_socket_create(
                                trueProperties.inputTypes[typeIndex],
                                defaultLexData, true);
                            d_node_add_socket(sheetNode, inputSocket);

                            SyntaxNode *inputSyntaxNode =
                                inputArgs.occurances[inputIndex];

                            // STX_literal, or STX_lineIdentifier.
                            // STX_TOKEN could be a variable name for a function
                            // like Set. STX_keyword shouldn't be here.
                            inputSyntaxNode = inputSyntaxNode->child;

                            switch (inputSyntaxNode->definition) {
                                case STX_literal:;
                                    // STX_TOKEN
                                    inputSyntaxNode = inputSyntaxNode->child;

                                    if (inputSyntaxNode != NULL) {
                                        LexToken *literalToken =
                                            inputSyntaxNode->info;

                                        // Make sure the literal data type is
                                        // compatable!
                                        DType typeOfLiteral =
                                            TYPE_FROM_LEX_LITERAL(
                                                literalToken->type);

                                        if ((typeOfLiteral &
                                             inputSocket->type) != TYPE_NONE) {
                                            inputSocket->type = typeOfLiteral;

                                            // Just set the LexData instead of
                                            // testing for each data type.
                                            inputSocket->defaultValue =
                                                literalToken->data;
                                        } else if (typeOfLiteral == TYPE_INT &&
                                                   inputSocket->type ==
                                                       TYPE_FLOAT) {
                                            // If the compiler expected a float,
                                            // but got an integer, that's fine.
                                            // We can easily convert it.
                                            literalToken->data.floatValue =
                                                (dfloat)literalToken->data
                                                    .integerValue;

                                            inputSocket->defaultValue =
                                                literalToken->data;
                                        } else {
                                            ERROR_COMPILER(
                                                sheet->filePath, lineNum, true,
                                                "Literal argument type (%s) "
                                                "does not match type of socket "
                                                "(%s)",
                                                d_type_name(typeOfLiteral),
                                                d_type_name(inputSocket->type));
                                        }
                                    }

                                    break;

                                case STX_lineIdentifier:;
                                    // STX_TOKEN (TK_INTEGERLITERAL)
                                    inputSyntaxNode = inputSyntaxNode->child;

                                    if (inputSyntaxNode != NULL) {
                                        LexToken *identifierToken =
                                            inputSyntaxNode->info;

                                        if (identifierToken != NULL) {
                                            dint identifier =
                                                identifierToken->data
                                                    .integerValue;

                                            // If we need to up the capacity of
                                            // the list, do so.
                                            if (numUnknownLineDefinitions + 1 >
                                                unknownLineDefinitionsCapacity) {
                                                unknownLineDefinitions =
                                                    (LineSocketPair *)d_realloc(
                                                        unknownLineDefinitions,
                                                        (++unknownLineDefinitionsCapacity) *
                                                            sizeof(
                                                                LineSocketPair));
                                            }

                                            LineSocketPair unknownLine;
                                            unknownLine.identifier = identifier;
                                            unknownLine.socket = inputSocket;
                                            unknownLineDefinitions
                                                [numUnknownLineDefinitions++] =
                                                    unknownLine;
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
                                        const char *name =
                                            token->data.stringValue;

                                        // Check to see if this input socket is
                                        // actually expecting a name.
                                        if (inputSocket->type == TYPE_NAME) {

                                            // Check to see if the name is
                                            // defined.
                                            AllNameDefinitions nameDefinitions =
                                                d_semantic_get_name_definitions(
                                                    sheet, name);

                                            if (nameDefinitions.numDefinitions >
                                                0) {
                                                // Set the input socket's string
                                                // value.
                                                inputSocket->defaultValue =
                                                    token->data;

                                                // NOTE: For Set nodes, the
                                                // definition of the node points
                                                // to the variable, not the Set
                                                // core function.
                                                if (nameDefinition.coreFunc ==
                                                    CORE_SET) {
                                                    NameDefinition
                                                        varDefinition;

                                                    if (d_semantic_select_name_definition(
                                                            name,
                                                            nameDefinitions,
                                                            &varDefinition)) {
                                                        sheetNode->definition =
                                                            varDefinition;
                                                    }
                                                }
                                            } else {
                                                ERROR_COMPILER(
                                                    sheet->filePath, lineNum,
                                                    true,
                                                    "Name %s is not defined",
                                                    name);
                                            }

                                            d_semantic_free_name_definitions(
                                                nameDefinitions);
                                        } else {
                                            d_error_compiler_push(
                                                "Name argument given when "
                                                "socket input isn't a name",
                                                sheet->filePath, lineNum, true);
                                        }
                                    } else {
                                        d_error_compiler_push(
                                            "Token argument is not a name",
                                            sheet->filePath, lineNum, true);
                                    }

                                    break;

                                default:
                                    d_error_compiler_push(
                                        "Invalid node input argument",
                                        sheet->filePath, lineNum, true);
                                    break;
                            }
                        }

                        d_syntax_free_results(inputArgs);
                    }

                    // Check to see if the number of inputs is what we expected.
                    if (trueProperties.numInputs > -1) {
                        if ((long)inputArgs.numOccurances !=
                            trueProperties.numInputs) {
                            ERROR_COMPILER(
                                sheet->filePath, lineNum, true,
                                "Expected %lu inputs in node %s, got %zu",
                                trueProperties.numInputs, nodeName,
                                inputArgs.numOccurances);
                        }
                    }

                    // Now we get the outputs.
                    // This will be easier since all outputs are line
                    // identifiers.
                    size_t numOutputs = 0;

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
                            size_t typeIndex = numOutputs;

                            if (trueProperties.numOutputs > -1) {
                                if (numOutputs >=
                                    (size_t)trueProperties.numOutputs) {
                                    // We've got more arguments than we
                                    // expected.
                                    ERROR_COMPILER(
                                        sheet->filePath, lineNum, false,
                                        "Got more outputs than expected "
                                        "(expected at most %zu), ignoring "
                                        "redundant outputs",
                                        (size_t)trueProperties.numOutputs);
                                    break;
                                }
                            }
                            // If there can be an infinite amount of inputs, we
                            // still need to get the data type.
                            else {
                                for (size_t j = 0; j < numOutputs; j++) {
                                    if (trueProperties.outputTypes[j] == 0)
                                        break;

                                    typeIndex = j;
                                }
                            }

                            // Create the socket, and add it to the node.
                            SheetSocket *outputSocket = d_socket_create(
                                trueProperties.outputTypes[typeIndex],
                                defaultLexData, false);
                            d_node_add_socket(sheetNode, outputSocket);

                            numOutputs++;

                            // STX_TOKEN
                            leaf = node->child;

                            if (leaf != NULL) {
                                leafToken = leaf->info;

                                if (leafToken != NULL) {
                                    dint identifier =
                                        leafToken->data.integerValue;

                                    // If we need to up the capacity of the
                                    // list, do so.
                                    if (numKnownLineDefinitions + 1 >
                                        knownLineDefinitionsCapacity) {
                                        knownLineDefinitions =
                                            (LineSocketPair *)d_realloc(
                                                knownLineDefinitions,
                                                (++knownLineDefinitionsCapacity) *
                                                    sizeof(LineSocketPair));
                                    }

                                    LineSocketPair knownLine;
                                    knownLine.identifier = identifier;
                                    knownLine.socket     = outputSocket;
                                    knownLineDefinitions
                                        [numKnownLineDefinitions++] = knownLine;
                                }
                            }

                            node = node->sibling;
                        }
                    }

                    // "We don't care about missing outputs, probably means the
                    // user doesn't care about those outputs."
                    //     - drwhut, when this section was being created.
                    //
                    // Me in the future now knows that this was a mistake.
                    // During code generation, it relies on the fact that all
                    // of the sockets are present. So, let's make them present!

                    if (trueProperties.numOutputs > -1) {
                        for (size_t socketIndex = numOutputs;
                             socketIndex < (size_t)trueProperties.numOutputs;
                             socketIndex++) {
                            SheetSocket *outputSocket = d_socket_create(
                                trueProperties.outputTypes[socketIndex],
                                defaultLexData, false);
                            d_node_add_socket(sheetNode, outputSocket);
                        }
                    }
                } else {
                    // We can't recognise the node name!
                    ERROR_COMPILER(sheet->filePath, lineNum, true,
                                   "Undefined node %s", nodeName);
                }

                d_semantic_free_true_properties(trueProperties);
            }
        }
    }

    // Now we've gone through all of the nodes, we need to do some checks.
    if (sheet->numStarts > 1) {
        ERROR_COMPILER(sheet->filePath, sheet->startNode->lineNum, true,
                       "Found %zu Start functions, only 1 is allowed",
                       sheet->numStarts);
    }

    for (size_t i = 0; i < sheet->numFunctions; i++) {
        SheetFunction func = sheet->functions[i];

        // All functions and subroutines must at most 1 Define node, and
        // subroutines must have 1 and only 1 Define node.
        if (func.numDefineNodes == 0 && func.isSubroutine) {
            ERROR_COMPILER(sheet->filePath, 0, true,
                           "Function %s has no Define "
                           "node defined",
                           func.name);
        } else if (func.numDefineNodes > 1) {
            ERROR_COMPILER(sheet->filePath, func.defineNode->lineNum, true,
                           "Function %s has %zu Define nodes defined when only "
                           "1 is allowed",
                           func.name, func.numDefineNodes);
        }

        // All functions (not subroutines) must have 1 and only 1 Return node.
        if (!func.isSubroutine) {
            if (func.numReturnNodes == 0) {
                ERROR_COMPILER(sheet->filePath, 0, true,
                               "Function %s has no Return "
                               "node defined",
                               func.name);
            } else if (func.numReturnNodes > 1) {
                ERROR_COMPILER(sheet->filePath, func.lastReturnNode->lineNum,
                               true,
                               "Function %s has %zu Return nodes defined when "
                               "only 1 is allowed",
                               func.name, func.numReturnNodes)
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
                d_socket_connect(knownLine.socket, unknownLine.socket, sheet,
                                 unknownLine.socket->node);
            }
        }

        if (!foundMatch) {
            ERROR_COMPILER(sheet->filePath, unknownLine.socket->node->lineNum,
                           true, "Undefined line identifier %" DINT_PRINTF_u,
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
    // Copy the array of node pointers, and when we've reduced a node,
    // set the node pointer in the new array to NULL.
    SheetNode **reducedNodes =
        (SheetNode **)d_malloc(sheet->numNodes * sizeof(SheetNode *));

    // Copy the array over.
    memcpy(reducedNodes, sheet->nodes, sheet->numNodes * sizeof(SheetNode *));

    bool allReduced = false;

    while (!allReduced) {
        VERBOSE(5, "Beginning a pass of reducing nodes...\n");

        bool neededToReduce = false;

        for (size_t nodeIndex = 0; nodeIndex < sheet->numNodes; nodeIndex++) {
            SheetNode *node = reducedNodes[nodeIndex];

            // We haven't reduced this node yet.
            if (node != NULL) {
                neededToReduce        = true;
                bool reducedAllInputs = true;
                const char *name      = node->name;

                VERBOSE(5, "Reducing node #%zu (%s)... ", nodeIndex, name)

                // Is it a core function?
                const CoreFunction coreFunc = d_core_find_name(name);

                if ((int)coreFunc > -1) {
                    switch (coreFunc) {
                        // Arithmetic core operations return a Float if any one
                        // of their inputs is a Float, otherwise an Integer,
                        // except for CORE_DIVIDE, which always returns a Float.
                        // This also applies to For, where if any of the
                        // arguments are Floats, the output index will also be a
                        // Float. We just need to check that the output we're
                        // setting isn't an Execution socket.
                        case CORE_ADD:
                        case CORE_SUBTRACT:
                        case CORE_MULTIPLY:
                        case CORE_DIVIDE:
                        case CORE_FOR:;
                            bool hasFloatInput = false;
                            SheetSocket *outputSocket =
                                NULL; // We want to set the output socket later
                                      // based on the input sockets.

                            for (size_t socketIndex = 0;
                                 socketIndex < node->numSockets;
                                 socketIndex++) {
                                SheetSocket *socket =
                                    node->sockets[socketIndex];

                                if (socket->type != TYPE_EXECUTION) {
                                    if (socket->isInput) {
                                        // If we know it's a Float anyway (from
                                        // a literal argument)...
                                        if (socket->type == TYPE_FLOAT) {
                                            hasFloatInput = true;
                                        }
                                        // If it's an Integer literal, we don't
                                        // care about it. It's already reduced.
                                        // So, we need to check connections.
                                        else if (socket->numConnections == 1) {
                                            SheetSocket *otherSide =
                                                socket->connections[0];

                                            // Is this socket reduced?
                                            if (IS_TYPE_REDUCED(
                                                    otherSide->type)) {
                                                // Great! We can set this
                                                // socket's type to be discrete!
                                                socket->type = otherSide->type;

                                                if (socket->type == TYPE_FLOAT)
                                                    hasFloatInput = true;
                                            } else
                                                reducedAllInputs = false;
                                        }
                                    } else {
                                        outputSocket = socket;
                                    }
                                }
                            }

                            // If we know out output needs to be a float, set it
                            // now. (Except if it's a divide)
                            if (coreFunc != CORE_DIVIDE &&
                                outputSocket != NULL) {
                                outputSocket->type =
                                    (hasFloatInput) ? TYPE_FLOAT : TYPE_INT;
                            }

                            // If we've gotten all of our inputs reduced, we can
                            // say we are fully reduced now.
                            if (reducedAllInputs) {
                                reducedNodes[nodeIndex] = NULL;
                            }

                            break;

                        // Print and Set have a single TYPE_VAR_ANY input.
                        // With Set, we need to check that the type we reduced
                        // down to is the same as the type of the variable we're
                        // setting.
                        case CORE_PRINT:
                        case CORE_SET:;
                            bool reduced          = true;
                            DType reducedTo       = TYPE_NONE;
                            const char *inputName = NULL;
                            for (size_t socketIndex = 0;
                                 socketIndex < node->numSockets;
                                 socketIndex++) {
                                SheetSocket *socket =
                                    node->sockets[socketIndex];

                                if (socket != NULL) {
                                    if (socket->isInput) {
                                        if (socket->type == TYPE_NAME)
                                            inputName = socket->defaultValue
                                                            .stringValue;

                                        else if (socket->type !=
                                                     TYPE_EXECUTION &&
                                                 socket->type != TYPE_NAME)
                                            reducedTo = socket->type;

                                        // This is our input of TYPE_VAR_ANY.
                                        // That will change soon... muhaha.
                                        // Unless we're printing a literal. In
                                        // which case... muhaha?
                                        if (socket->type == TYPE_VAR_ANY) {
                                            // Ok... we'll try the connection.
                                            if (socket->numConnections == 1) {
                                                SheetSocket *otherSide =
                                                    socket->connections[0];

                                                // Is this socket reduced?
                                                if (IS_TYPE_REDUCED(
                                                        otherSide->type)) {
                                                    // Great! We can set this
                                                    // socket's type to be
                                                    // discrete!
                                                    socket->type =
                                                        otherSide->type;

                                                    reducedTo = socket->type;
                                                } else
                                                    reduced = false;
                                            }
                                        }
                                    }
                                }
                            }

                            if (reduced) {
                                reducedNodes[nodeIndex] = NULL;

                                if (coreFunc == CORE_SET && inputName != NULL) {
                                    // We need to check the variable type
                                    // matches!
                                    SheetVariable *var =
                                        node->definition.variable;

                                    // Does the reduced type not match the
                                    // variable type?
                                    if (reducedTo != var->dataType) {
                                        ERROR_COMPILER(
                                            sheet->filePath, node->lineNum,
                                            true,
                                            "Input type (%s) does not match "
                                            "variable's type (%s has type %s)",
                                            d_type_name(reducedTo), var->name,
                                            d_type_name(var->dataType));
                                    }
                                }
                            }

                            break;

                        // For bitwise operators, the inputs and outputs can
                        // either be all integers or all booleans, but they
                        // CANNOT mix.
                        case CORE_AND:
                        case CORE_NOT:
                        case CORE_OR:
                        case CORE_XOR:;
                            DType finalType = TYPE_NONE; // Represents all of
                                                         // the socket's types.
                            bool allSame = true;
                            for (size_t socketIndex = 0;
                                 socketIndex < node->numSockets;
                                 socketIndex++) {
                                SheetSocket *socket =
                                    node->sockets[socketIndex];

                                if (socket != NULL) {
                                    if (socket->isInput) {
                                        // If we know it's an Integer or Boolean
                                        // anyway (from a literal argument)...
                                        if (socket->type == TYPE_INT ||
                                            socket->type == TYPE_BOOL) {
                                            // Before we replace finalType, we
                                            // need to make sure, if finalType
                                            // has a type, if this type is the
                                            // same.
                                            if (finalType != TYPE_NONE &&
                                                socket->type != finalType) {
                                                d_error_compiler_push(
                                                    "All inputs in bitwise "
                                                    "operators must be of the "
                                                    "same type",
                                                    sheet->filePath,
                                                    node->lineNum, true);
                                                allSame = false;
                                            }

                                            finalType = socket->type;

                                        }
                                        // So we need to check the connection.
                                        else if (socket->numConnections == 1) {
                                            SheetSocket *otherSide =
                                                socket->connections[0];

                                            // Is this socket reduced?
                                            if (IS_TYPE_REDUCED(
                                                    otherSide->type)) {
                                                // Great! We can set this
                                                // socket's type to be discrete!
                                                socket->type = otherSide->type;

                                                // But wait... is it the same as
                                                // the rest of the types???
                                                if (finalType != TYPE_NONE &&
                                                    socket->type != finalType) {
                                                    d_error_compiler_push(
                                                        "All inputs in bitwise "
                                                        "operators must be of "
                                                        "the same type",
                                                        sheet->filePath,
                                                        node->lineNum, true);
                                                    allSame = false;
                                                }

                                                finalType = socket->type;
                                            } else {
                                                reducedAllInputs = false;
                                            }
                                        }
                                    } else if (finalType != TYPE_NONE) {
                                        socket->type = finalType;
                                    }
                                }
                            }

                            // If all of them are not the same, we can't keep
                            // reducing and hope that they do become the same.
                            // It's a lost cause :(
                            if (reducedAllInputs || !allSame) {
                                reducedNodes[nodeIndex] = NULL;
                            }
                            break;

                        // For comparison operators, we want the inputs to be
                        // the same. The output is always a boolean. The user is
                        // allowed to mix integer and float inputs, but if one
                        // is a string, the other has to be too.
                        case CORE_EQUAL:
                        case CORE_LESS_THAN:
                        case CORE_LESS_THAN_OR_EQUAL:
                        case CORE_MORE_THAN:
                        case CORE_MORE_THAN_OR_EQUAL:
                        case CORE_NOT_EQUAL:;
                            bool hasNumberInput = false;
                            bool hasStringInput = false;

                            for (size_t socketIndex = 0;
                                 socketIndex < node->numSockets;
                                 socketIndex++) {
                                SheetSocket *socket =
                                    node->sockets[socketIndex];

                                if (socket != NULL) {
                                    if (socket->type != TYPE_EXECUTION) {
                                        if (socket->isInput) {
                                            // If we know it's a Number anyway
                                            // (from a literal argument)...
                                            if ((socket->type & TYPE_NUMBER) !=
                                                0) {
                                                hasNumberInput = true;
                                            }
                                            // Or if we know it's a String
                                            // anyway...
                                            else if (socket->type ==
                                                     TYPE_STRING) {
                                                hasStringInput = true;
                                            }
                                            // If it's an Integer literal, we
                                            // don't care about it. It's already
                                            // reduced. So, we need to check
                                            // connections.
                                            else if (socket->numConnections ==
                                                     1) {
                                                SheetSocket *otherSide =
                                                    socket->connections[0];

                                                // Is this socket reduced?
                                                if (IS_TYPE_REDUCED(
                                                        otherSide->type)) {
                                                    // Great! We can set this
                                                    // socket's type to be
                                                    // discrete!
                                                    socket->type =
                                                        otherSide->type;

                                                    if ((socket->type &
                                                         TYPE_NUMBER) != 0)
                                                        hasNumberInput = true;
                                                    else if (socket->type ==
                                                             TYPE_STRING)
                                                        hasStringInput = true;
                                                } else
                                                    reducedAllInputs = false;
                                            }
                                        } else {
                                            outputSocket = socket;
                                        }
                                    }
                                }
                            }

                            // If they've mixed numbers and strings.. remind
                            // them of their dirty deeds.
                            if (hasNumberInput && hasStringInput) {
                                d_error_compiler_push(
                                    "Comparison operators cannot compare "
                                    "between numbers and strings",
                                    sheet->filePath, node->lineNum, true);
                            }

                            // If we've gotten all of our inputs reduced, we can
                            // say we are fully reduced now.
                            if (reducedAllInputs) {
                                reducedNodes[nodeIndex] = NULL;
                            }

                            break;

                        // For the Ternary operator, the two value inputs must
                        // be the same, and the output must match the type of
                        // the inputs.
                        case CORE_TERNARY:;
                            DType inputType     = TYPE_NONE;
                            bool inputsSameType = true;

                            for (size_t socketIndex = 1;
                                 socketIndex < node->numSockets;
                                 socketIndex++) {
                                SheetSocket *socket =
                                    node->sockets[socketIndex];

                                if (socket != NULL) {
                                    if (socket->isInput) {

                                        // If we already know the type (since)
                                        // the input could be a literal...
                                        if (IS_TYPE_REDUCED(socket->type)) {

                                            // Before we replace inputType, if
                                            // it has already been set, then
                                            // we need to check our type is
                                            // the type that is already there.
                                            if (inputType != TYPE_NONE &&
                                                socket->type != inputType) {
                                                d_error_compiler_push(
                                                    "Value inputs in a Ternary "
                                                    "operator must be of the "
                                                    "same type",
                                                    sheet->filePath,
                                                    node->lineNum, true);
                                                inputsSameType = false;
                                            }

                                            inputType = socket->type;
                                        }

                                        // So we need to check the connection.
                                        else if (socket->numConnections == 1) {
                                            SheetSocket *otherSide =
                                                socket->connections[0];

                                            // Is this socket reduced?
                                            if (IS_TYPE_REDUCED(
                                                    otherSide->type)) {

                                                // Great! So we can set this
                                                // socket's type to be discrete!
                                                socket->type = otherSide->type;

                                                // But wait... is is the same
                                                // as the rest of the types???
                                                if (inputType != TYPE_NONE &&
                                                    socket->type != inputType) {
                                                    d_error_compiler_push(
                                                        "Value inputs in a "
                                                        "Ternary operator must "
                                                        "be of the same type",
                                                        sheet->filePath,
                                                        node->lineNum, true);
                                                    inputsSameType = false;
                                                }

                                                inputType = socket->type;
                                            } else {
                                                reducedAllInputs = false;
                                            }
                                        }

                                    } else if (inputType != TYPE_NONE) {
                                        socket->type = inputType;
                                    }
                                }
                            }

                            // If we've gotten all of our inputs reduced, we can
                            // say we are fully reduced now, or if the inputs
                            // are not the same, we can't make them the same,
                            // so we have to stop.
                            if (reducedAllInputs || !inputsSameType) {
                                reducedNodes[nodeIndex] = NULL;
                            }
                            break;

                        default:
                            reducedNodes[nodeIndex] = NULL;
                            break;
                    }
                } else {
                    // TODO: Reduce functions from other sheets.
                    reducedNodes[nodeIndex] = NULL;
                }

                if (VERBOSE_LEVEL >= 5) {
                    if (reducedNodes[nodeIndex] == NULL)
                        printf("done.\n");
                    else
                        printf("not yet able to reduce.\n");
                }
            }
        }

        if (!neededToReduce)
            allReduced = true;
    }

    VERBOSE(5, "Done reducing nodes.\n");

    free(reducedNodes);
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
static void check_loop(Sheet *sheet, SheetNode *start, SheetNode **pathArray,
                       size_t pathLength) {
    // Firstly, check if we're already in the path.
    for (size_t i = 0; i < pathLength - 1; i++) {
        if (start == pathArray[i]) {
            VERBOSE(5, "FOUND LOOP\n")

            ERROR_COMPILER(sheet->filePath, start->lineNum, true,
                           "Detected loop entering node %s", start->name);
            return;
        }
    }

    // Set ourselves into the path array.
    pathArray[pathLength - 1] = start;

    // Now, go through all of our output connections,
    // and call this function on them.
    if (start->sockets != NULL && start->numSockets > 0) {
        for (size_t i = 0; i < start->numSockets; i++) {
            SheetSocket *socket = start->sockets[i];

            if (!socket->isInput && socket->connections != NULL &&
                socket->numConnections > 0) {
                for (size_t j = 0; j < socket->numConnections; j++) {
                    SheetSocket *connectedTo = socket->connections[j];
                    SheetNode *next          = connectedTo->node;

                    if (next != NULL) {
                        VERBOSE(5, "ENTER %s LINE %zu\n", next->name,
                                next->lineNum)

                        check_loop(sheet, next, pathArray, pathLength + 1);

                        VERBOSE(5, "EXIT %s LINE %zu\n", next->name,
                                next->lineNum)
                    }
                }
            }
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

    if (sheet->nodes != NULL && sheet->numNodes > 0) {
        // We can't go on a journey that is bigger than the number
        // of nodes (without looping)
        SheetNode **path =
            (SheetNode **)d_malloc(sheet->numNodes * sizeof(SheetNode *));

        // Find nodes with no inputs (except name sockets).
        for (size_t i = 0; i < sheet->numNodes; i++) {
            SheetNode *node = sheet->nodes[i];
            bool hasInputs  = false;

            if (node->sockets != NULL && node->numSockets > 0) {
                for (size_t j = 0; j < node->numSockets; j++) {
                    if (node->sockets[j]->isInput &&
                        node->sockets[j]->type != TYPE_NAME) {
                        hasInputs = true;
                        break;
                    }
                }

                // If we have no inputs, find all paths from here.
                if (!hasInputs) {
                    VERBOSE(5, "- Checking paths from node #%zu (%s)...\n", i,
                            node->name);

                    check_loop(sheet, node, path, 1);
                }
            } else {
                d_error_compiler_push("Node has no sockets, is redundant",
                                      sheet->filePath, node->lineNum, false);
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

    VERBOSE(1, "-- Detecting loops...\n")
    d_semantic_detect_loops(sheet);
}
