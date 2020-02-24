**NOTE:** This update changes some of the fundamental definitions of items in
Decision, and thus will break most source files, and all object files, from
previous Decision versions!

## Compiler

### Comments

* Removed multi-line comments.
* Added single-line comments: Anything after and including the `>` symbol on
the same line will be ignored by the compiler.

### Metadata

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

### Core Functions

* Integers as inputs to bitwise operators is now supported.
* Booleans as inputs to `Equal` and `NotEqual` is now supported.
* Reimplemented `Length` to use the C function `strlen`.

### The VM

* Changed the Decision VM to be stack-based, rather than register-based.
  * The VM stack can grow and shrink in size, meaning the VM is not limited in
  the amount of memory it can use anymore.
  * The stack holds temporary values (previously stored in the registers),
  arguments and return values (previously stored in the general stack), saved
  return addresses (previously stored in the call stack), as well as saved
  frame pointers.

## The C API

* C functions are now stored in sheets, rather than in a global list.
* Sheets can now be loaded with an initial list of included sheets, which are
loaded before any compilation happens.
* Sheets now have an `allowFree` property that stops sheets that include it
from freeing the sheet when they are freed.

## Refactoring

* Split large source files into smaller ones to improve code readability.
  * The graph components of sheets (i.e. the nodes and wires) have their own
  set of files, `dgraph.c` and `dgraph.h`.
  * Finding the definitions of names has been moved to `dname.c` and `dname.h`.
  * Decision object files now have their own set of source files, `dobj.c` and
  `dobj.h`, and the code has been revamped to be less error-prone.
* Removed all calls to "safe" Visual C++ functions, due to considerations of
standardisation.

## Documentation

* The compiler now has a `--export-core` flag, which will output the core
reference in JSON format.
  * This is used in conjunction with an extension added to the user manual,
  which reads the JSON and displays the core reference automatically.

## Optimisations

* Changed the build system such that most of the code compiles to a singular
library (which is either static or shared, static by default, see
[README.md](README.md) for more details), and link the executable to that
library. This has reduced the compile time significantly.
* Syntax Analysis has been optimised to check the next token ahead before
dedicating itself to a syntax definition.
* Nodes now reference their definitions, which reduces data redundancy.
* Wires are stored in an ordered list, rather than as direct pointers, to
reduce the number of memory allocations and to remove the potential for
dangling pointers.

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
