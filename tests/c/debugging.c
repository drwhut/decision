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

#include "assert.h"

#include <ddebug.h>
#include <decision.h>
#include <dsheet.h>

/* Debugging session callback functions. */

void onWireValue(Sheet *sheet, Wire wire, DType type, LexData value) {
    printf("W %s: (%zu, %zu) -> ", sheet->filePath, wire.socketFrom.nodeIndex,
           wire.socketFrom.socketIndex);

    switch (type) {
        case TYPE_INT:
            printf("%" DINT_PRINTF_d, value.integerValue);
            break;
        case TYPE_FLOAT:
            printf("%g", value.floatValue);
            break;
        case TYPE_STRING:
            printf("%s", value.stringValue);
            break;
        case TYPE_BOOL:
            printf("%s", (value.booleanValue) ? "true" : "false");
            break;
        default:
            printf("?");
            break;
    }

    printf(" -> (%zu, %zu)\n", wire.socketTo.nodeIndex,
           wire.socketTo.socketIndex);
}

void onExecutionWire(Sheet *sheet, Wire wire) {
    printf("E %s: (%zu, %zu) -> (%zu, %zu)\n", sheet->filePath,
           wire.socketFrom.nodeIndex, wire.socketFrom.socketIndex,
           wire.socketTo.nodeIndex, wire.socketTo.socketIndex);
}

void onNodeActivated(Sheet *sheet, size_t nodeIndex) {
    printf("N %s: %zu\n", sheet->filePath, nodeIndex);
}

void onCall(Sheet *sheet, const NodeDefinition *funcDef, bool isC) {
    if (isC) {
        printf("C");
    } else {
        printf("D");
    }

    printf("C %s: %s\n", sheet->filePath, funcDef->name);
}

void onReturn() {
    printf("R\n");
}

void onNodeBreakpoint(Sheet *sheet, size_t nodeIndex) {
    printf("NB %s: %zu\n", sheet->filePath, nodeIndex);
}

void onWireBreakpoint(Sheet *sheet, Wire wire) {
    printf("WB %s: (%zu, %zu) -> (%zu, %zu)\n", sheet->filePath,
           wire.socketFrom.nodeIndex, wire.socketFrom.socketIndex,
           wire.socketTo.nodeIndex, wire.socketTo.socketIndex);
}

Sheet *main_sheet() {
    const char *nonDebugIncludeSrc = "[Function(Double)]\n"
                                     "[FunctionInput(Double, n, Float, 1.0)]\n"
                                     "[FunctionOutput(Double, n2, Float)]\n"
                                     "Define(Double)~#1\n"
                                     "Multiply(#1, 2)~#2\n"
                                     "Return(Double, #2)\n";

    const char *debugIncludeSrc = "[Function(Quadruple)]\n"
                                  "[FunctionInput(Quadruple, n, Float, 1.0)]\n"
                                  "[FunctionOutput(Quadruple, n4, Float)]\n"
                                  "Define(Quadruple)~#1\n"
                                  "Double(#1)~#2\n"
                                  "Double(#2)~#3\n"
                                  "Return(Quadruple, #3)\n";

    const char *mainSrc = "Start~#1\n"
                          "Print(#1, 'I hope you\\'re ready...')~#2\n"
                          "For(#2, 1.0, 2.5, 0.5)~#3, #4, #5\n"
                          "Quadruple(#4)~#6\n"
                          "Print(#3, #6)\n"
                          "Print(#5, 'Done!')\n";

    // Test including non-debuggable sheets.
    Sheet *nonDebugInclude = d_load_string(nonDebugIncludeSrc, "ndi", NULL);

    Sheet *include1[] = {nonDebugInclude, NULL};

    CompileOptions options1 = DEFAULT_COMPILE_OPTIONS;
    options1.includes       = include1;
    options1.debug          = true;

    // Test including debuggable sheets.
    Sheet *debugInclude = d_load_string(debugIncludeSrc, "di", &options1);

    Sheet *include2[] = {debugInclude, NULL};

    CompileOptions options2 = DEFAULT_COMPILE_OPTIONS;
    options2.includes       = include2;
    options2.debug          = true;

    return d_load_string(mainSrc, "main", &options2);
}

// clang-format off

char *answer =
"E main: (0, 0) -> (1, 0)\n"
"N main: 1\n"
"I hope you're ready...\n"
"E main: (1, 2) -> (2, 0)\n"
"N main: 2\n"
"E main: (2, 4) -> (4, 0)\n"
"W main: (2, 5) -> 1 -> (3, 0)\n"
"WB main: (2, 5) -> (3, 0)\n"
"N main: 3\n"
"DC di: Quadruple\n"
"BP\n"
"W di: (0, 1) -> 1 -> (1, 0)\n"
"N di: 1\n"
"DC ndi: Double\n"
"R\n"
"W di: (1, 1) -> 2 -> (2, 0)\n"
"N di: 2\n"
"DC ndi: Double\n"
"R\n"
"W di: (2, 1) -> 4 -> (3, 1)\n"
"N di: 3\n"
"R\n"
"W main: (3, 1) -> 4 -> (4, 1)\n"
"N main: 4\n"
"4\n"
"N main: 2\n"
"E main: (2, 4) -> (4, 0)\n"
"W main: (2, 5) -> 1.5 -> (3, 0)\n"
"WB main: (2, 5) -> (3, 0)\n"
"N main: 3\n"
"DC di: Quadruple\n"
"BP\n"
"W di: (0, 1) -> 1.5 -> (1, 0)\n"
"N di: 1\n"
"DC ndi: Double\n"
"R\n"
"W di: (1, 1) -> 3 -> (2, 0)\n"
"N di: 2\n"
"DC ndi: Double\n"
"R\n"
"W di: (2, 1) -> 6 -> (3, 1)\n"
"N di: 3\n"
"R\n"
"W main: (3, 1) -> 6 -> (4, 1)\n"
"N main: 4\n"
"6\n"
"N main: 2\n"
"E main: (2, 4) -> (4, 0)\n"
"W main: (2, 5) -> 2 -> (3, 0)\n"
"WB main: (2, 5) -> (3, 0)\n"
"N main: 3\n"
"DC di: Quadruple\n"
"BP\n"
"W di: (0, 1) -> 2 -> (1, 0)\n"
"N di: 1\n"
"DC ndi: Double\n"
"R\n"
"W di: (1, 1) -> 4 -> (2, 0)\n"
"N di: 2\n"
"DC ndi: Double\n"
"R\n"
"W di: (2, 1) -> 8 -> (3, 1)\n"
"N di: 3\n"
"R\n"
"W main: (3, 1) -> 8 -> (4, 1)\n"
"N main: 4\n"
"8\n"
"N main: 2\n"
"E main: (2, 4) -> (4, 0)\n"
"W main: (2, 5) -> 2.5 -> (3, 0)\n"
"WB main: (2, 5) -> (3, 0)\n"
"N main: 3\n"
"DC di: Quadruple\n"
"BP\n"
"W di: (0, 1) -> 2.5 -> (1, 0)\n"
"N di: 1\n"
"DC ndi: Double\n"
"R\n"
"W di: (1, 1) -> 5 -> (2, 0)\n"
"N di: 2\n"
"DC ndi: Double\n"
"R\n"
"W di: (2, 1) -> 10 -> (3, 1)\n"
"N di: 3\n"
"R\n"
"W main: (3, 1) -> 10 -> (4, 1)\n"
"N main: 4\n"
"10\n"
"N main: 2\n"
"E main: (2, 6) -> (5, 0)\n"
"N main: 5\n"
"NB main: 5\n"
"BP\n"
"Done!\n"
"R\n";

// clang-format on

int main() {
    Sheet *sheet = main_sheet();

    DebugNodeBreakpoint nodeBreaks[] = {{sheet, 5}, // Last Print.
                                        {NULL, 0}};

    DebugWireBreakpoint wireBreaks[] = {
        {sheet, {{2, 5}, {3, 0}}}, // Index value from For to Quadruple.
        {NULL, {{0, 0}, {0, 0}}}};

    DebugAgenda agenda      = NO_AGENDA;
    agenda.onWireValue      = &onWireValue;
    agenda.onExecutionWire  = &onExecutionWire;
    agenda.onNodedActivated = &onNodeActivated;
    agenda.onCall           = &onCall;
    agenda.onReturn         = &onReturn;
    agenda.onNodeBreakpoint = &onNodeBreakpoint;
    agenda.onWireBreakpoint = &onWireBreakpoint;
    agenda.nodeBreakpoints  = nodeBreaks;
    agenda.wireBreakpoints  = wireBreaks;

    DebugSession session = d_debug_create_session(sheet, agenda);

    START_CAPTURE_STDOUT()
    while (d_debug_continue_session(&session)) {
        printf("BP\n");
    }
    STOP_CAPTURE_STDOUT()
    ASSERT_CAPTURED_STDOUT(answer)

    d_debug_stop_session(&session);

    d_sheet_free(sheet);
    return 0;
}