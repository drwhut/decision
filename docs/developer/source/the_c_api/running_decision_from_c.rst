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
Running Decision from C
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

Decision Strings
================

Loading Decision Strings
------------------------

To load a string with Decision source code in into a ``Sheet``, use:

.. doxygenfunction:: d_load_string
   :no-link:

You can then run any compiled ``Sheet`` with:

.. doxygenfunction:: d_run_sheet
   :no-link:

.. code-block:: c

   #include <decision.h>
   #include <dsheet.h>

   int main() {
       // Load the string.
       Sheet *sheet = d_load_string("Start~#1 ; Print(#1, 'Hello, world!')", NULL);

       // Run the sheet.
       d_run_sheet(sheet);

       // Free the sheet from memory.
       d_sheet_free(sheet);

       return 0;
   }

Running Decision Strings
------------------------

To run a Decision source code string directly, use:

.. doxygenfunction:: d_run_string
   :no-link:

.. code-block:: c

   #include <decision.h>

   int main() {
       // Run the string.
       d_run_string("Start~#1 ; Print(#1, 'Hello, world!')", NULL);

       return 0;
   }

Decision Files
==============

Loading Decision Files
----------------------

To load a Decision file from the disk into a ``Sheet``, use:

.. doxygenfunction:: d_load_file
   :no-link:

.. code-block:: c

   #include <decision.h>
   #include <dsheet.h>

   int main() {
       // Load the sheet into memory.
       Sheet *sheet = d_load_file("hello_world.dc");

       // Run the sheet.
       d_run_sheet(sheet);

       // Free the sheet from memory.
       d_sheet_free(sheet);

       return 0;
   }

Running Decision Files
----------------------

To run a Decision file directly, use:

.. doxygenfunction:: d_run_file
   :no-link:

.. code-block:: c

   #include <decision.h>

   int main() {
       // Run the file.
       d_run_file("hello_world.dc");

       return 0;
   }

.. _decision-functions:

Decision Functions
==================

In order to run specific functions and subroutines, you first need to know how
to pass arguments and get return values from them.

Passing Arguments
-----------------

To pass arguments to a function or subroutine, you need to push the values
onto the Decision Virtual Machine's stack **in the correct order**.
This can be done with the following functions:

.. doxygenfunction:: d_vm_push
   :no-link:

.. doxygenfunction:: d_vm_push_float
   :no-link:

.. doxygenfunction:: d_vm_push_ptr
   :no-link:

Getting Return Values
---------------------

To get return values from a function or subroutine, you need to pop the values
from the Decision Virtual Machine's stack **in reverse order**.
This can be done with the following functions:

.. doxygenfunction:: d_vm_pop
   :no-link:

.. doxygenfunction:: d_vm_pop_float
   :no-link:

.. doxygenfunction:: d_vm_pop_ptr
   :no-link:

Calling Decision Functions
--------------------------

Finally, you can call the function or subroutine itself using:

.. doxygenfunction:: d_run_function
   :no-link:

.. code-block:: decision

   > decision.dc

   [Function(IsEven)]
   [FunctionInput(IsEven, Integer)]
   [FunctionOutput(IsEven, Boolean)]

   Define(IsEven)~#1
   Mod(#1, 2)~#2
   Equal(#2, 0)~#3
   Return(IsEven, #3)

   [Subroutine(SayHi)]

   Define(SayHi)~#10
   Print(#10, "Hi! :3")

.. code-block:: c

   // main.c
   #include <dcfg.h>
   #include <decision.h>
   #include <dsheet.h>
   #include <dvm.h>

   #include <stdio.h>

   int main() {
       // Load the sheet into memory.
       Sheet *sheet = d_load_file("decision.dc");

       // Create a Decision Virtual Machine.
       DVM vm = d_vm_create();

       // Calling a function/subroutine with no inputs or outputs is easy:
       d_run_function(&vm, sheet, "SayHi");

       // It's good practice to reset the VM once you've run something.
       d_vm_reset(&vm);

       dint value = 531780;

       // To call the IsEven function, we first need to push the argument onto
       // the stack.
       d_vm_push(&vm, value);

       // REMEMBER: If you have more than one argument, push the arguments IN
       // THE CORRECT ORDER.

       // Then call the function:
       d_run_function(&vm, sheet, "IsEven");

       // Then pop the return values out IN REVERSE ORDER.
       dint isEven = d_vm_pop(&vm);

       // dcfg.h has some handy macros for when you want to print dint or
       // dfloat types:
       if (isEven) {
           printf("%" DINT_PRINTF_d " is even!\n", value);
       } else {
           printf("%" DINT_PRINTF_d " is odd!\n", value);
       }

       // Free the VM.
       d_vm_free(&vm);

       d_sheet_free(sheet);
       return 0;
   }
