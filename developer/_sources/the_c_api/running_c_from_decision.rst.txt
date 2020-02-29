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
stack, with the same functions described in :ref:`decision-functions`.

For example, lets say you want to create a ``Sin`` function that you want to
call from a Decision sheet. A way you could implement the C function would be
like this:

.. code-block:: c

   #include <dcfunc.h>
   #include <dvm.h>

   #include <math.h>

   void mySin(DVM *vm) {
       // Get the "first" argument from the stack.
       dfloat input = d_vm_get_float(vm, 1);

       dfloat output = sin(input) // sin defined in math.h

       // Push the return value back onto the stack.
       d_vm_push_float(vm, output);
   }

Notice that the ``mySin`` function doesn't return anything, and it takes a
``DVM *`` input, so it fits the prototype explained above. It then gets a float
value from the stack, takes the ``sin`` of that value, and pushes the result
onto the stack as the return value.

.. note::

   As per the calling convention, the VM before calling will push arguments in
   the correct order, during the call the VM's frame pointer will be set, and
   after the call the VM will get the return values assuming the first return
   value is at the top of the stack.
   
   This is why, when defining C functions to be called from Decision, in order
   to get the nth argument, you should use ``d_vm_get(vm, n)``, and when you
   push the return values, you should push them **in reverse order.**

So now you know how to create C functions so you can call them from Decision,
so how do you call them from Decision?

C Functions as Decision Functions
=================================

To create a Decision function to call a C function, you can use this function
defined in ``dcfunc.h``:

.. doxygenfunction:: d_create_c_function
   :no-link:

You can then add the C function to a sheet with:

.. doxygenfunction:: d_sheet_add_c_function
   :no-link:

Here is an example that uses the ``mySin`` function that we defined earlier:

.. code-block:: c

   #include <dcfunc.h>
   #include <decision.h>
   #include <dtype.h>
   #include <dvm.h>

   #include <math.h>

   void mySin(DVM *vm) {
       // Get the "first" argument from the stack.
       dfloat input = d_vm_get_float(vm, 1);

       dfloat output = sin(input); // sin defined in math.h

       // Push the return value back onto the stack.
       d_vm_push_float(vm, output);
   }

   int main() {
       // The Sin function sockets.
       SocketMeta sinSockets[] = {
           {"theta", "The number to take the sin of, in radians.", TYPE_FLOAT, {0}},
           {"sin", "The sin of theta.", TYPE_FLOAT, {0}}
       };

       // If a socket's default value you want to set isn't an integer, you will
       // need to set it outside the array, like this:
       sinSockets[0].defaultValue.floatValue = 1.0;

       // The Sin function has 1 input and 1 output.
       CFunction sinFunction = d_create_c_function(&mySin, "Sin", "Get the sin of a number.",
           sinSockets, 1, 1);
         
       // Create a "library" sheet to store the C function in.
       Sheet *library = d_sheet_create("MathStuff");

       // Make sure other sheets do not free this library when they are freed.
       library->allowFree = false;
      
       // Add the C function to the sheet.
       d_sheet_add_c_function(library, sinFunction);

       // Create a list of sheets with the sheet that has the C function in.
       // It needs to end with a NULL entry!
       Sheet *includeList[] = {NULL, NULL};
       includeList[0]       = library;

       // Set the compile options so the sheet that we want to run, so the sheet
       // can find the definition of "Sin" during compile time.
       CompileOptions options = DEFAULT_COMPILE_OPTIONS;
       options.includes       = includeList;

       // Run a Decision string which uses Sin!
       d_run_string("Start~#1; Sin(1.57)~#2; Print(#1, #2);", NULL, &options);

       // Free the library sheet.
       d_sheet_free(library);

       return 0;
   }

C Functions as Decision Subroutines
===================================

To create a Decision subroutine to call a C function, you can use this function
defined in ``dcfunc.h``:

.. doxygenfunction:: d_create_c_subroutine
   :no-link:

.. note::

   This function automatically prepends a **before** input execution socket and
   appends an **after** output execution socket for you.

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
       // Get the file name argument from the stack.
       char *fileName = (char *)d_vm_get_ptr(vm, 1);

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
       d_vm_push_ptr(vm, buffer);
   }

   int main() {
       // The ReadFile subroutine non-execution sockets.
       SocketMeta readFileSockets[] = {
           {"file", "The name of the file to open.", TYPE_STRING, {0}},
           {"content", "The content of the file.", TYPE_STRING, {0}}
       }

       // Again, if you want to set a non-integer socket's default value...
       readFileSockets[0].defaultValue.stringValue = "file.txt";

       // The ReadFile subroutine has 1 input and 1 output.
       CFunction readFileSubroutine = d_create_c_subroutine(&myReadFile, "ReadFile",
           "Read the contents of a file.", readFileSockets, 1, 1);
      
       // Create a "library" sheet to store the C function in.
       Sheet *library = d_sheet_create("FileThings");

       // Make sure other sheets do not free this library when they are freed.
       library->allowFree = false;
      
       // Add the C function to the sheet.
       d_sheet_add_c_function(library, readFileSubroutine);

       // Create a list of sheets with the sheet that has the C function in.
       // It needs to end with a NULL entry!
       Sheet *includeList[] = {NULL, NULL};
       includeList[0]       = library;

       // Set the compile options so the sheet that we want to run, so the sheet
       // can find the definition of "ReadFile" during compile time.
       CompileOptions options = DEFAULT_COMPILE_OPTIONS;
       options.includes       = includeList;

       // Run source code that calls the ReadFile subroutine!
       d_run_string("Start~#1; ReadFile(#1, 'hello.txt')~#2, #3; Print(#3, #2);",
           NULL, &options);

       // Free the library sheet.
       d_sheet_free(library);

       return 0;
   }
