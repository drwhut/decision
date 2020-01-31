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

.. _the-virtual-machine:

*******************
The Virtual Machine
*******************

This chapter describes the specification of the Decision Virtual Machine.
It is defined in the files ``dvm.c`` and ``dvm.h``.

Its job is to run the bytecode generated from :ref:`code-generation`.

#############
The Structure
#############

.. note::

   ``dint`` == ``int32_t`` if the macro ``DECISION_32`` is defined.
   Otherwise, ``dint`` == ``int64_t``.

The Decision VM, ``DVM``, is defined in ``dvm.h``. It has the following
properties:

* A program counter ``pc``, which is a ``char*``.

.. note::

   The program counter ``pc`` increments after **every** instruction, even
   when calling / returning from functions.

* 256 general-purpose registers, which are indexed by 1 byte, split up into
  2 sections:

  * The first 128 registers are integer (``dint``) registers, capable of
    holding integers, string pointers, and booleans.

  * The last 128 registers are float (``dfloat``) registers.

* 4 argument registers (``dint``), which are used to specify arguments when
  when making VM system calls.

* 2 stacks for when we want to make calls to other functions:

  * A general stack (``dint``), which is used to pass arguments and return
    values from calls to other functions.
  
  * A call stack (``char*``), which is used to store where we left off when
    calling other functions, so that once we are done in those functions,
    we can carry on from where we left off.

* 2 flags:

  * A ``halted`` flag, which states if the VM has stopped executing.
  
  * A ``runtimeError`` flag, which states if there was an error during
    execution.

##################
Instruction Format
##################

All instructions vary in size. You can find out the size of an instruction by
using the function:

.. doxygenfunction:: d_vm_ins_size
   :no-link:

Opcodes
=======

Opcodes are stored as a single byte, in the most significant byte of the
instruction.

Registers
=========

All general-purpose regisers can be represented with a single byte, since
there are 256 of them. However, which registers you provide should depend
on whether the instruction is floating-point or not. Some opcodes have a
floating-point variation, e.g. ``ADD`` has a floating-point variation called
``ADDF``. Floating point registers start at the index
``VM_REG_FLOAT_START = 128``.

Immediates
==========

To optimise the number of instructions in the machine code, there are some
instructions that allow you to take shortcuts in the form of **immediates**.
Instead of loading a constant value into a register and then using it to
operate on another register, you can operate on the one register in one
instruction, with the constant represented as an immediate.

Immediates are always *half* the size of ``dint``, and they always start from
the least significant byte, regardless of the instruction. This means if you
want to represent a ``dint`` without overflow, you need to store it in a
register by using a ``LOADUI`` followed by an ``ORI``, with the most
significant half and least significant half respectively.

.. note::

   Floating-point constants cannot be represented as immediates.

Diagrams
========

Here are some example diagrams with the instructions represented in big-endian
format.

* ``ADD // $(r1) = $(r1) + $(r2)``:

::

   2     1    0
   +-----+----+----+
   | 0x1 | r2 | r1 |
   +-----+----+----+

* ``CEQ // $(r1) = ($(r2) == $(r3)) ? 1 : 0``:

::

   3     2    1    0
   +-----+----+----+----+
   | 0x8 | r3 | r2 | r1 |
   +-----+----+----+----+

* ``JRCON // $(pc) = ($(r1)) ? $(pc) + W_IMMEDIATE(4/2) : $(pc)`` in 32-bit:

::

   3      2    1     0
   +------+----+-----+-----+
   | 0x1F | r1 | IMMEDIATE |
   +------+----+-----+-----+

* ``JRCON // $(pc) = ($(r1)) ? $(pc) + W_IMMEDIATE(4/2) : $(pc)`` in 64-bit:

::

   5      4    3    2    1    0
   +------+----+----+----+----+----+
   | 0x1F | r1 |     IMMEDIATE     |
   +------+----+----+----+----+----+

Action Syntax
=============

Next to each opcode in ``dvm.h`` is a description of what the opcode does,
and how to write the instruction. The examples above have their descriptions
as they appear in the comments. This section describes what the syntax of
those descriptions are.

* ``rn`` means an integer regiser in the nth byte of the instruction, e.g.
  ``r3`` would mean any value inbetween 0 and 127 in the 3rd byte after the
  immediate, if any. If there is no n, then you only need 1 register for the
  instruction to work, in the 1st byte.

* ``fn`` means a float register in the nth byte of the instruction, e.g.
  ``f1`` would mean any value inbetween 128 and 255 in the 1st byte after
  the immediate, if any. If there is no n, then you only need 1 register for
  the instruction to work, in the 1st byte.

* ``$(register)`` means the value inside the register ``register``.

* ``IMMEDIATE(4/2)`` means the immediate value in the instruction. The 4/2
  means the immediate is 4 bytes long if in 64-bit, and 2 bytes long if in
  32-bit. Note that the value is taken as if it was the size of ``dint``!
  For example, -1 would not stay as -1, as when it gets resizes, the missing
  bytes are all ``0x0``. This allows instructions like ``ORI`` to work as
  expected, where it will not OR the most-significant half.

* ``W_IMMEDIATE(4/2)`` means the same as ``IMMEDIATE(4/2)``, except it is
  taken as if it was an integer of that size, so -1 would stay as -1 as the
  rest of the bytes would be filled by ``0xff``. This allows instructions like
  ``JR`` to work as expected, where you are allowed to jump a negative amount
  (backwards).

############
System Calls
############

With the ``SYSCALL`` opcode, you can make system calls for extra
functionality. All of the types of system calls you can make are defined in
an enumerator called ``DSyscall`` in ``dvm.h``. Next to each system call in
``dvm.h`` is a specification of how the value of each argument register will
affect the action.

When generating bytecode, it will always set the argument registers before
making the system call.

##############
Implementation
##############

To create a virtual machine, simply define one:

.. code-block:: c

   DVM vm;

To initialize the VM to a ready state, run:

.. doxygenfunction:: d_vm_reset
   :no-link:

If at any point a runtime error should be produced from the VM, use this
function:

.. doxygenfunction:: d_vm_runtime_error
   :no-link:

It will halt the VM and set the ``runtimeError`` flag to ``true``. Similar to
in ``derror.h``, you can instead call a macro verion:

.. doxygendefine:: ERROR_RUNTIME

Where the variable arguments are treated as if they were put into a ``printf``
call, allowing for more detailed error messages.

Arguably one of the more important functions for the VM is:

.. doxygenfunction:: d_vm_parse_ins_at_pc
   :no-link:

Which take the instruction at ``vm->pc`` and executes it.

However, the most important function of them all, the big daddy himself, the
one ring to rule them all, is:

.. doxygenfunction:: d_vm_run
   :no-link:

Which initializes the program counter to ``start``, and enters a loop of
``d_vm_parse_ins_at_pc`` until the VM's ``halted`` flag is set to ``true``.
It returns ``true`` if it halted without any errors, and ``false`` otherwise.
