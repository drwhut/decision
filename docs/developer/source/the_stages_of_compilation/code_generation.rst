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

.. _code-generation:

###############
Code Generation
###############

Code Generation is perfomed in ``dcodegen.c`` and ``dcodegen.h``.

After we've made a model of the sheet in memory, and after we've checked that
everything the user has made is syntactically and semantically correct,
we now need to generate the **bytecode** for :ref:`the-virtual-machine` to
run.

So far, the difference between *execution* and *non-execution* nodes has
only been that execution nodes have execution sockets - the main difference
will be explained in this section.

Definition Nodes
================

For definition nodes, the first thing we do is generate a return instruction
before generating anything else. This essentially acts as a "bookmark" for
the function in the bytecode, which the linker can later point to.

Subroutines
-----------

For subroutines like ``Start``, the code generator will recurse through the
execution wires and generate the bytecode for each execution node in sequence.

Functions
---------

For functions, the code generator will try and find the ``Return`` node for
the function, and recurse backwards to generate the bytecode to calculate
the return values. The reason for this is because we don't have an order of
execution as we do for execution nodes.

Execution Nodes
===============

When an execution node is reached, it will first generate the bytecode to get
the inputs. If the inputs are literals, then this is trivial. But if there
is a connection to the input's socket, then we need to follow the connection
and recursively generate the bytecode for the node that starts the connection.

Secondly, bytecode is generated depending on the node itself, using the
inputs. If the node is a core function, then we can directly insert the
bytecode in. Otherwise, we make use of the calling abilities of
:ref:`the-virtual-machine` by saving current unsafe values onto the stack,
then adding the arguments onto the stack, then calling the function itself.
When we return from the function, we then pop the returned values out from
the stack, and then restore the saved unsafe values.

Lastly, it will check to see if the last output execution socket has a
connection. If it does, then there is something more to do after this node
is finished. We then recursively generate the bytecode for the next execution
node and add it on to the bytecode we've already generated. Otherwise, we may
want to put a return instruction at the end of the sequence to say that we
are done. In some cases, however, this is not needed, like inside ``For``
loops.

When getting output from an execution node, the generator will copy the output
into another register to preserve the original value.

Non-Execution Nodes
===================

When a non-execution node is reached, it will first generate the bytecode to
get the inputs, like with execution nodes.

Next, bytecode is generated depending on the node itself, using the inputs.
This process is almost exactly the same as with execution nodes, including
if the function is not a core function.

However, unlike execution nodes, it does *not* generate any more bytecode past
the current node. This is because non-execution nodes are purely meant to be
called upon from execution nodes.

When getting output from a non-execution node, it will be used by the node
getting the input, then the register will be freed. This ensures that if
values change over time, then the calculations will produce different results
over time as well. Non-execution nodes are purely meant to just manipulate
the inputs to execution nodes.

.. _calling-convention:

Calling Convention
==================

If we see a node with a name that isn't a core function, it needs to be
*called*. How we call functions is important and should be as efficient as
possible since there could be lots of calls during execution, especially
if the call is inside a loop.

Arguments and return values are easy - the arguments are pushed onto the
general stack in reverse order just before the call and the return values are
poped out of the general stack in order just after the call.

What isn't so easy is the registers themselves. A problem that needed to be
solved was that a called functions will most likely use the same registers as
its caller. The initial solution was to save registers that needed to be
saved into the general stack before pushing the arguments, and popping the
saved values back afterwards, but I'm going to be honest: **This solution
sucked.**

So here is the current solution: We split the virtual machine registers into
2 categories:

* **Safe registers** (callee-saved): No matter what function you call, these
  registers will *always* retain the value they had before calling. This is
  guaranteed by having the callee save the safe registers it knows it will use
  into the general stack at the start of the function, and popping them back
  out at the end of the function, before pushing the return values.

* **Temporary / Unsafe registers** (caller-saved): If you call another
  function, there is a chance that these register values can change by the end
  of the call. In this case, if you want to save the value inside a temporary
  register, you'll need to manually save the register into the stack yourself.

.. note::

   There is no difference between safe and temporary registers in terms of how
   they are implemented in the virtual machine - it's how we treat them in the
   bytecode that is different, hence why it is called a **convention**.

.. note::

   A bug that came from this that took (in my opinion) too long to find was
   that some functions that saved safe registers at the start of the function
   would pop them back out into their correct registers (as intended), but
   there was a chance that in doing that it would overwrite a return value
   stored at that register. So because of this, **any return values stored in
   safe registers get transferred to a temporary register before being pushed
   onto the stack.**

Functions
---------

So we've got some *safe* registers thanks to this calling convention, but we
should use the safe registers sparingly... so how do we know if we need to use
a safe register or not? Simple. There are recursive functions that tell you
if a call is coming up:

.. doxygenfunction:: d_does_input_involve_call
   :no-link:

Checks input nodes starting from ``node`` to see if any of them involve a call.

.. doxygenfunction:: d_does_output_involve_call
   :no-link:

Checks output nodes (i.e. an execution sequence) starting from ``node`` to see
if any of them involve a call.

Both of these functions call:

.. doxygenfunction:: d_is_node_call
   :no-link:

To see if ``node`` is a function that needs to be called, i.e. it isn't a core
function.

Structures
==========

``BCode``
    Essentially a list of ``char``. Its purpose is to store bytecode and be
    a convenient way to build up the bytecode. It also stores alongside the
    code a list of instruction indexes and what they should be linked to,
    as well as a list of indexes to say where functions start pushing
    return values onto the stack.

There are 3 convenient functions for ``BCode``:

.. doxygenfunction:: d_malloc_bytecode
   :no-link:

Automatically allocates memory to store bytecode for you.

Likewise, there is a function to free the memory:

.. doxygenfunction:: d_free_bytecode
   :no-link:

Arguably the most useful function however is:

.. doxygenfunction:: d_concat_bytecode
   :no-link:

Concatenates ``after`` onto the end of ``base``, which makes building up the
bytecode simple.

There is an alternate function for attaching bytecode to one another:

.. doxygenfunction:: d_insert_bytecode
   :no-link:

Takes some bytecode ``insertCode`` and inserts it into the bytecode ``base``,
starting at the index ``insertIndex``, i.e. the bytecode at index
``insertIndex`` of ``base`` after the function is called should be the
start of the bytecode ``insertCode``.

.. note::

   ``d_insert_bytecode`` is a lot more expensive computationally than
   ``d_concat_bytecode``, and thus should be used sparingly. This is because
   it needs to fix data like links and relative jumps after the insertion.

``BuildContext``
    Contains useful information used by the code generator to help make the
    most efficient bytecode possible. Only one instance is made at the
    beginning of Bytecode Generation, and it is passed around by reference
    throughout.

    It also stores a section of memory for data like variables and string
    literals, which will be stored in the sheet, and also keeps track of
    links to those items.

    One thing we want to guarantee when creating the bytecode is that
    :ref:`the-virtual-machine` is using the least amount of registers
    possible, since there is a finite amount of them. We also don't want to
    store values that aren't going to be used anymore. The way we do this is
    by storing a list of bits, one for each register, and set the bit to 1 if
    the register is currently in use, and 0 if it is free. We also store 2
    extra variables to say what the next available integer register is, and
    what the next available float register is.

    When we generate bytecode for a non-execution node, we typically free all
    of the input registers and use a register for the output (most of the time
    we use the first input register as the output register).

    When we generate bytecode for an execution node that involves a loop, we
    don't free the output register until the loop bytecode has been generated,
    so the loop can get its value. Only after do we free the register.

    This freedom of choosing when registers are freed allows us to keep values
    in memory for as long as they are needed.

There are a few functions that control the use of registers in ``BuildContext``:

.. doxygenfunction:: d_next_general_reg
   :no-link:

Takes the index of the next available integer register, and outputs it.
It also takes the time to find the next available register by incrementing
until it finds one. You can also specify if you want a *safe* register rather
than a *temporary* one, to follow the :ref:`calling-convention`.

.. doxygenfunction:: d_next_float_reg
   :no-link:

Does the same thing as ``d_next_general_reg``, but for float registers.

.. warning::

   Bear in mind if you want a safe register, **you are not guaranteed to get
   one.** If the context has run out of safe registers, it will give you the
   next available temporary one.

Both of the above functions call a more generic one:

.. doxygenfunction:: d_next_reg
   :no-link:

Where ``nextRegInContext`` points to a ``reg_t`` variable in ``context`` that
represents which register is the next free register, and ``end`` says to the
function what the last register it can use is - if it goes over this value,
it will produce a warning.

.. doxygenfunction:: d_free_reg
   :no-link:

Frees a given register. If the given register's index is less than the current
next available register, the next available register is set to ``reg``. Again,
this is to make sure we use the least amount of registers possible.

Sockets
=======

One important thing to note is how a node knows which input corresponds to
which register.

Inside the ``SheetSocket`` structure defined in ``dsheet.h``, there is a
property called ``_reg``. Only Code Generation uses this property. If the
socket is an output socket, it is set during the generation of the bytecode
of the node to the index of the register containing that output value. If
the socket is an input socket, and if the socket has a connection, it is set
to the connected output socket's register to guarantee that the value is
transferred along the wire to the correct input socket. If there is no
connection, then a register may or may not be assigned for the literal value,
depending on if we want to use the literal value as an immediate or not.

.. note::

   See :ref:`the-virtual-machine` to see what I mean by an *immediate* value.

This way, registers are passed along from socket to socket, so that the next
node knows which register its input is in.

Linking Functions
=================

The whole process of linking is explained in :ref:`linking`, but in order for
it to work, we need to do 2 things:

1. We need to know what things we need to link to, like variables or
   functions.
2. We need to say which instructions need to point to which link.

.. doxygenfunction:: d_add_link_to_ins
   :no-link:

This function puts information into ``context`` to say which instruction in
``bcode`` (index ``insIndex``) needs to point to some link ``linkMeta``. If
``linkMeta`` is a duplicate of a previously linked ``linkMeta``, then the
instruction points to the *original version*, and the new version is ignored.

``indexInList`` is replaced with the index of ``linkMeta`` in the list of
``LinkMeta`` in ``context``, and ``wasDuplicate`` is replaced with a boolean
representing if ``linkMeta`` was already found in ``context``.

.. doxygenfunction:: d_allocate_from_data_section
   :no-link:

Allocates a chunk of memory for the data section in ``context``. ``size`` is
the size of the allocation in bytes, and ``index`` is replaced with the index
of the start of the allocation, which is useful when creating links. The
function returns a *pointer* to the start of the allocation, rather than the
*index*. This makes copying data into the allocation easy.

.. doxygenfunction:: d_allocate_string_literal_in_data
   :no-link:

Allocated memory for the data section of ``context`` in order to fit the
length of ``stringLiteral``. This function automatically links an instruction
from ``linkCode`` (index ``insIndex``) to the string literal's new location.

.. doxygenfunction:: d_allocate_variable
   :no-link:

Allocates memory for the data section of ``context``, in order to fit the
content of a variable ``variable``. You can specify a size, which is usually
``sizeof(dint)``, all you need to provide is the index of the variable's entry
in the ``LinkMeta`` list in ``context``.

Generation Functions
====================

.. doxygenfunction:: d_convert_between_number_types
   :no-link:

Takes a numbered socket (Integer or Float), and generates bytecode to convert
it to the other discrete type, how it does so depends on the opcode you provide.

If you provide ``OP_MVTF`` or ``OP_MVTI``, it will just move the raw value to
that type of register.

If you provide ``OP_CVTF`` or ``OP_CVTI``, it will convert the value into that
type so that it represents the same value (truncated).

This function automatically sets the socket's new register.

.. doxygenfunction:: d_setup_input
   :no-link:

Builds on ``d_generate_bytecode_for_literal`` and also sets up inputs that are
a part of a connection. **This function does not generate the bytecode for the
nodes that contribute to the input!** The function also automatically adds any
bytecode it needed to generate onto ``addTo``, and sets the value of
``inputReg`` to the register containing the input value. Like 
``d_generate_bytecode_for_literal``, you can specify if you want your integers
to be converted to floats with ``forceFloat``.

.. doxygenfunction:: d_setup_arguments
   :no-link:

If you are generating the bytecode for a function, then this function pops the
arguments needed from the stack, puts them in the correct registers, and
assigns those registers to their respective sockets. The bytecode to do this
is added onto ``addTo``.

.. doxygenfunction:: d_setup_returns
   :no-link:

If you are generating the bytecode for a function, then this function pushes
the return values onto the stack, and if ``retAtEnd`` is true, also places a
return instruction at the end of the bytecode. The bytecode to do this is
added onto ``addTo``.

.. doxygenfunction:: d_generate_bytecode_for_literal
   :no-link:

Generates the bytecode needed to load in a literal value. It will
automatically set the register the literal value is now in into socket.
If the literal is an integer, and you want it converted to a float, you can
specify that in ``cvtFloat``.

.. doxygenfunction:: d_generate_bytecode_for_inputs
   :no-link:

Generates the bytecode for all nodes that contribute to an input to a node.
``inLoop`` is used for optimization purposes.

.. doxygenfunction:: d_generate_bytecode_for_variable
   :no-link:

Generates the bytecode for a node that is the getter of a variable
``variable``. It essentially loads its value from the data section of the
sheet. This function also consequently adds a link to that bytecode to the
location of the variable.

.. doxygenfunction:: d_generate_bytecode_for_call
   :no-link:

Generates the bytecode to call a custom function or subroutine. The bytecode
does a number of things:

1. It saves temporary registers that haven't been freed yet by pushing them
   onto the stack.

2. It then pushes the argument values on afterwards (in reverse order, because
   stacks)

3. It then calls the function by pushing the current program counter onto a
   call stack, and jumping to the function's location.

4. When the function returns, it first pops the return values into available
   registers that aren't going to be overwritten with the previously pushed
   saved values.

5. It then pops the saved temporary values back into the register that it was
   originally in, just in case a node expects it to be in that register later
   on.

.. note::

   If you know that a call is coming, and you need to save a register for
   after the call, **put it in a safe register**. This is more efficient,
   especially if the call is being executed in a loop.

.. doxygenfunction:: d_generate_bytecode_for_operator
   :no-link:

Generates bytecode for a generic operator node. Based on the inputs, it will
automatically deal with the types for you, unless you set ``forceFloat``
to true.

.. doxygenfunction:: d_generate_bytecode_for_comparator
   :no-link:

Generates bytecode for a generic comparator node that outputs a boolean.
Since the VM does not have an opcode for "not equals", a ``notAfter``
argument is provided to invert the answer.

.. doxygenfunction:: d_generate_bytecode_for_nonexecution_node
   :no-link:

Generates the bytecode for any non-execution node, regardless of if it's a
core function or not.

.. doxygenfunction:: d_generate_bytecode_for_execution_node
   :no-link:

Generates the bytecode for any execution node, regardless of if it's a core
function or not. If ``retAtEnd`` is true, it will place a return instruction
at the end of the sequence, unless ``inLoop`` is true. ``inLoop`` should be
true if the node is being executed inside a loop like ``For`` or ``While``.
``inLoop`` will always stop a return instruction from being generated, so the
program does not return at the end of one loop.

.. doxygenfunction:: d_generate_bytecode_for_function
   :no-link:

Generates the bytecode for any custom function or subroutine.

Conclusion
==========

In order to do all of the above, all you need is the function:

.. doxygenfunction:: d_codegen_compile
   :no-link:

Which takes a sheet, generates the bytecode, and inserts it into the sheet
once it's done.

Output
------

By the end of compilation, if everything succeeds, then you should have a
``Sheet`` with the contents of an **object file**. An object file is made up
of multiple sections that contain different data.

The functionality to save, load and dump Decision object files can be found
in ``dasm.h``.

Object Sections
---------------

* ``.text``: A list of instructions for :ref:`the-virtual-machine` to execute.
* ``.main``: An integer which says which instruction in ``.text`` should be
  the one to execute first if this sheet is where we start executing. It
  essentially points to the compiled ``Start``.
* ``.data``: An allocated section of memory with items like variables and
  string literals contained inside.
* ``.lmeta``: Essentially a list of ``ListMeta``, which contains data on what
  and where something is.
* ``.link``: A table of what instructions point to which index in the
  ``.lmeta`` section.
* ``.func``: Essentially a list of ``SheetFunction``.
* ``.var``: Essentially a list of ``SheetVariable``.
* ``.incl``: A list of paths to sheets that this sheet includes.
* ``.c``: A list of specifications of C functions the sheet uses.
  See :ref:`running-c-from-decision`.
