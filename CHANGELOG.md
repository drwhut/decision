## Compiler

* The compiler now automatically converts integer literals to float literals
if the socket is a float-only socket.
* Added `d_run_function` to `decision.h`, which allows for running Decision
functions and subroutines from C.
* Added `dcfunc.c` and `dcfunc.h`, which allow for running C functions from
Decision.
* Added new opcode `OP_CALLC` to the VM.
* The compiler, as part of finding the definition of a name, will look at
defined C functions in `dcfunc.c` to find one.
* Added `d_vm_pop_stack`, `d_vm_pop_stack_float` and `d_vm_pop_stack_ptr` to
`dvm.h`.
* Added `d_vm_push_stack`, `d_vm_push_stack_float` and `d_vm_push_stack_ptr` to
`dvm.h`.
* Moved the linking stage to the last step in loading a string instead of the
first step in running a string.
* Optimised by adding a stage in linking dedicated to pre-calculating the
memory locations of external variables and pointers.
* Fixed a bug where compiled sheets maintained their external pointers.

## Documentation

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
