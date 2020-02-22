**NOTE:** This update changes some of the fundamental definitions of items in
Decision, and thus will break most source files, and all object files, from
previous Decision versions!

* Changed the Decision VM to be stack-based, rather than register-based.
* Removed multi-line comments.
* Added single-line comments: Anything after and including the `>` symbol on
the same line will be ignored by the compiler.
* All nodes, sockets, variables, functions and subroutines now have both names
and descriptions which are accessible via the C API.
  * The `Variable` property now has a description argument, e.g.
  `Variable(count, Integer, 0, "Counts how many pies you've eaten today.")`
  * The `Function` and `Subroutine` properties now have a description argument,
  e.g. `Function(Double, "Doubles the number.")`
  * The `FunctionInput` and `FunctionOutput` argument specification has
  changed:
    * `FunctionInput(functionName, inputName, inputType [,inputDefaultValue [, inputDescription]])`
    * `FunctionOutput(functionName, outputName, outputType [, outputDescription])`
* Nodes now reference their definitions, which reduces data redundancy.
* Changed how sheets store wires in memory.
* Reimplemented `Length` to use the C function `strlen`.
* Split large source files into smaller ones to improve code readability.
  * The graph components of sheets (i.e. the nodes and wires) have their own
  set of files, `dgraph.c` and `dgraph.h`.
  * Finding the definitions of names has been moved to `dname.c` and `dname.h`.
  * Decision object files now have their own set of source files, `dobj.c` and
  `dobj.h`, and the code has been revamped to be less error-prone.

# Decision v0.2.0 - 16th January 2020

## Strings

* Added the `Length` core function, which returns the number of characters in
  a given string.
* Added tests for the `Length` function.

## Compiler

* The compiler now automatically converts integer literals to float literals
if the socket is a float-only socket.
* Moved the linking stage to the last step in loading a string instead of the
first step in running a string.
* Optimised by adding a stage in linking dedicated to pre-calculating the
memory locations of external variables and pointers.
* Fixed a bug where compiled sheets maintained their external pointers.

## The C API

* Added `d_run_function` to `decision.h`, which allows for running Decision
functions and subroutines from C.
* Added `dcfunc.c` and `dcfunc.h`, which allow for running C functions from
Decision.
* Added new opcode `OP_CALLC` to the VM.
* Added a `.c` section to compiled object files to store the specifications of
C functions the sheet uses.
* The compiler, as part of finding the definition of a name, will look at
defined C functions in `dcfunc.c` to find one.
* Added `d_vm_pop_stack`, `d_vm_pop_stack_float` and `d_vm_pop_stack_ptr` to
`dvm.h`.
* Added `d_vm_push_stack`, `d_vm_push_stack_float` and `d_vm_push_stack_ptr` to
`dvm.h`.
* Added tests for the C API to the CMake project.

## Documentation

* Added "String Manipulation" section to the user manual.
* Added "The C API" chapter to the developer manual.
* Added note in the user manual about semi-colons being valid end of statement
symbols.

# Decision v0.1.1 - 25th December 2019

* Changed the `bool` definition to the one in `stdbool.h`, which makes it
consistent to the C++ definition of the `bool` type.

# Decision v0.1.0 - 19th December 2019

This is the first release of Decision!

Feel free to download it and give it a try - if you want to start contributing
to the project, have a look at `CONTRIBUTING.md`, and have fun!
