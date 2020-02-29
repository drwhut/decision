..
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

#######################
Debugging Decision Code
#######################

.. warning::

   If you are embedding into a C++ program, it is very, very recommended that
   when you include a C header file, you encapsulate it with an ``extern``
   block, like so:

   .. code-block:: cpp

      extern "C" {
          #include <decision.h>
          #include <dsheet.h>
      }

Introduction
============

As of the 0.3.0 update, you can debug Decision code through the C API!
This means that when you compile Decision code in **debug mode** (more on this
later), you can tell the debugger to tell you when:

* A non-execution wire transfers a value from one socket to another, and what
  the value transfered was.
* An execution wire activates.
* A node activates.
* A node calls a function or subroutine.
* A function or subroutine returns.

You can also set breakpoints which, when hit, will pause the debugger.

* Node breakpoints are hit when the node is activated.
* Wire breakpoints are hit when:

  * A value is transfered if the wire is a non-execution wire.
  * The wire is activated if the wire is an execution wire.

Compiling in Debug Mode
=======================

In order to debug a sheet, you need to compile the sheet in **debug mode**.
This will make a couple of changes to the compilation process:

* When generating the bytecode, the code generator will also store debugging
  information alongside the bytecode, e.g. where a node starts in bytecode,
  which instructions call functions, etc.

* Optimisation will no longer occur. This is because we don't want to lose
  debugging information. For example, say there are two consecutive ``NOT``
  instructions in the bytecode, made from two ``Not`` nodes in the Decision
  code. Optimisation would usually remove these instructions, but when
  debugging, we still want to know when those two ``Not`` nodes activate!

To compile a sheet in debug mode, you need to set the ``debug`` property in a
``CompileOptions`` structure:

.. code-block:: c

   CompileOptions options = DEFAULT_COMPILE_OPTIONS;
   options.debug          = true;

   // This sheet will now be compiled in debug mode.
   Sheet *sheet = d_load_file("debug.dc", &options);

Debugging Agendas
=================

Agendas describe what you want to happen when certain events occur during the
debugging process. In code, they are in the form of function pointers:

.. doxygentypedef:: OnWireValue
   :no-link:

.. doxygentypedef:: OnExecutionWire
   :no-link:

.. doxygentypedef:: OnNodeActivated
   :no-link:

.. doxygentypedef:: OnCall
   :no-link:

.. doxygentypedef:: OnReturn
   :no-link:

Once you have functions in the above forms, you can then add them to a
``DebugAgenda`` structure:

.. code-block:: c

   DebugAgenda agenda      = NO_AGENDA;
   agenda.onWireValue      = &onWireValue;
   agenda.onExecutionWire  = &onExecutionWire;
   agenda.onNodedActivated = &onNodeActivated;
   agenda.onCall           = &onCall;
   agenda.onReturn         = &onReturn;

Breakpoints
-----------

If you want to set breakpoints, you can specify where you want the breakpoints
to be set:

.. code-block:: c

   DebugNodeBreakpoint nodeBreaks[] = {
       {sheet, 2}, // The 3rd node in the sheet.
       {NULL, 0}   // Array needs to end with a NULL entry!
   };

   DebugWireBreakpoint wireBreaks[] = {
       {sheet, {{2, 2}, {3, 1}}}, // The wire that goes from the 3rd socket of
                                  // the 3rd node to the 2nd socket of the 4th
                                  // node.
       {NULL, {{0, 0}, {0, 0}}}   // Array needs to end with a NULL entry!
   };

   // Don't forget to add them to the agenda!
   agenda.nodeBreakpoints = nodeBreaks;
   agenda.wireBreakpoints = wireBreaks;

Then, you can tell the debugger to let you know when the breakpoints are hit
with the given function pointers:

.. doxygentypedef:: OnNodeBreakpoint
   :no-link:

.. doxygentypedef:: OnWireBreakpoint
   :no-link:

Then you can add them to the agenda:

.. code-block:: c

   agenda.onNodeBreakpoint = &onNodeBreakpoint;
   agenda.onWireBreakpoint = &onWireBreakpoint;

Debugging Sessions
==================

To create a debugging session, use:

.. doxygenfunction:: d_debug_create_session
   :no-link:

You can then start/continue a debugging session with:

.. doxygenfunction:: d_debug_continue_session
   :no-link:

.. tip::

   You can put ``d_debug_continue_session`` as the condition of a while loop!
   This way, the session will continue until the sheet exits, and you can
   process something after each breakpoint hit!

Once you are done debugging, you should use:

.. doxygenfunction:: d_debug_stop_session
   :no-link:

Partial Example
===============

This example just prints out when nodes get activated.

.. code-block:: c

   #include <ddebug.h>
   #include <decision.h>
   #include <dsheet.h>

   #include <stdio.h>

   void onNodeActivated(Sheet *sheet, size_t nodeIndex) {
       Node node = sheet->graph.nodes[nodeIndex];
       const NodeDefinition *nodeDef = node.definition;

       printf("Debug> Node %s on line %zu in sheet %s has been activated!\n",
           nodeDef->name, node.lineNum, sheet->filePath);
   }

   int main() {
       // Load the sheet in debug mode.
       CompileOptions options = DEFAULT_COMPILE_OPTIONS;
       options.debug          = true;

       Sheet *sheet = d_load_file("debug.dc", &options);

       // Set up the debugging agenda.
       DebugAgenda agenda     = NO_AGENDA;
       agenda.onNodeActivated = &onNodeActivated;

       // Create the session.
       DebugSession session = d_debug_create_session(sheet, agenda);

       // Keep continuing the session until the sheet exits.
       while (d_debug_continue_session(&session)) {
           printf("Debug> A breakpoint was hit!\n");
       }

       // Once we're done, stop the session.
       d_debug_stop_session(&session);

       // Free the sheet.
       d_sheet_free(sheet);

       return 0;
   }

To see a full example that uses all of the capabilities of the debugger, feel
free to look at ``tests/c/debugging.c``!
