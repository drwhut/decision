..
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

.. _running-c-from-decision:

#######################
Running C from Decision
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

C Function Prototype
====================

In order to start calling C functions from Decision, the functions you want to
call need to look like this:

.. doxygentypedef:: DecisionCFunction
   :no-link:

Essentially, the functions that are called by Decision should not return
anything, and they take a Decision Virtual Machine pointer as input.

But wait, what if I want my C function to return values? The answer to that
question is that inputs and outputs are taken from the Virtual Machine's
general stack, with the same functions described in :ref:`decision-functions`.

For example, lets say you want to create a ``Sin`` function that you want to
call from a Decision sheet. A way you could implement the C function would be
like this:

.. code-block:: c

   #include <dcfunc.h>
   #include <dvm.h>

   #include <math.h>

   void mySin(DVM *vm) {
       // Pop the argument from the stack.
       dfloat input = d_vm_pop_stack_float(vm);

       dfloat output = sin(input) // sin defined in math.h

       // Push the return value back onto the stack.
       d_vm_push_stack_float(vm);
   }

Notice that the ``mySin`` function doesn't return anything, and it takes a
``DVM *`` input, so it fits the prototype explained above. It then pops a float
value from the stack, takes the ``sin`` of that value, and pushes the result
onto the stack as the return value.

.. note::

   As per the calling convention, the VM before calling will push arguments in
   reverse order, and after calling will pop return values in the correct
   order.
   
   This is why, when defining C functions to be called from Decision, you
   should **pop arguments in the correct order** and **push return values in
   reverse order**.

So now you know how to create C functions so you can call them from Decision,
so how do you call them from Decision?

C Functions as Decision Functions
=================================

To create a Decision function to call a C function, you can use this function
defined in ``dcfunc.h``:

.. doxygenfunction:: d_create_c_function
   :no-link:

You should then free the list of created C functions at the end of your program
with:

.. doxygenfunction:: d_free_c_functions
   :no-link:

Here is an example that uses the ``mySin`` function that we defined earlier:

.. code-block:: c

   #include <dcfunc.h>
   #include <decision.h>
   #include <dtype.h>
   #include <dvm.h>

   #include <math.h>

   void mySin(DVM *vm) {
       // Pop the argument from the stack.
       dfloat input = d_vm_pop_stack_float(vm);

       dfloat output = sin(input) // sin defined in math.h

       // Push the return value back onto the stack.
       d_vm_push_stack_float(vm);
   }

   int main() {
       // Sin has 1 Float input.
       DType sinInputs[] = {TYPE_FLOAT, TYPE_NONE};

       // Sin has 1 Float output.
       DType sinOutputs[] = {TYPE_FLOAT, TYPE_NONE};

       // Create the C function.
       d_create_c_function("Sin", &mySin, sinInputs, sinOutputs);

       // Run source code that calls the function.
       d_run_string("Start~#1; Sin(1.0)~#2; Print(#1, #2);", "source");

       // Free the function we defined earlier.
       d_free_c_functions();

       return 0;
   }

C Functions as Decision Subroutines
===================================

To create a Decision subroutine to call a C function, you can use this function
defined in ``dcfunc.h``:

.. doxygenfunction:: d_create_c_subroutine
   :no-link:

.. note::

   This function automatically prepends the standard input and output
   Execution sockets for you.

Like before, you should then free the list of created C functions at the end of
your program with:

.. doxygenfunction:: d_free_c_functions
   :no-link:

Here is an example of a subroutine that reads the contents of a given file:

.. code-block:: c

   #include <dcfunc.h>
   #include <decision.h>
   #include <dtype.h>
   #include <dvm.h>

   #include <stdio.h>

   // A buffer for the contents of the file.
   // The reason it is a global is because it will last the lifetime of the
   // program - if we put it in the readFile function, it would go out of scope
   // when the function ended, so the pointer pushed to the VM would be invalid.
   char buffer[256];

   void myReadFile(DVM *vm) {
       // Pop the file name argument from the stack.
       char *fileName = (char *)d_vm_pop_stack_ptr(vm);

       FILE *fp = fopen(fileName, "r");
    
       if (fp != NULL) {
           size_t len = fread(buffer, 1, 256, fp);

           if (ferror(fp) != 0) {
               printf("Error reading file!\n");
           } else {
               buffer[len + 1] = 0;
           }

           fclose(fp);
       }

       // Push the buffer pointer return value to the stack.
       d_vm_push_stack_ptr(vm, buffer);
   }

   int main() {
       // ReadFile has 1 String input.
       DType readFileInputs[] = {TYPE_STRING, TYPE_NONE};

       // ReadFile has 1 String output.
       DType readFileOutputs[] = {TYPE_STRING, TYPE_NONE};

       // Create the C subroutine.
       d_create_c_subroutine("ReadFile", &myReadFile, readFileInputs, readFileOutputs);

       // Run source code that calls the subrotune.
       d_run_string("Start~#1; ReadFile(#1, 'hello.txt')~#2, #3; Print(#2, #3);", "source");

       // Free the subroutine we defined earlier.
       d_free_c_functions();

       return 0;
   }
