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
:ref:`the-virtual-machine`.

Lastly, it will check to see if the last output execution socket has a
connection. If it does, then there is something more to do after this node
is finished. We then recursively generate the bytecode for the next execution
node and add it on to the bytecode we've already generated. Otherwise, we may
want to put a return instruction at the end of the sequence to say that we
are done. In some cases, however, this is not needed, like inside ``For``
loops.

When getting output from an execution node, the generator will copy the output
onto the top of the stack in order to preserve the original value.

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
getting the input. This ensures that if values change over time, then the
calculations will produce different results over time as well. Non-execution
nodes are purely meant to just manipulate
the inputs to execution nodes.

.. _calling-convention:

Calling Convention
==================

If we see a node with a name that isn't a core function, it needs to be
*called*. How we call functions is important and should be as efficient as
possible since there could be lots of calls during execution, especially
if the call is inside a loop.

Arguments and return values are easy - arguments are pushed onto the stack in
order before the call, and return values are pushed onto the stack in reverse
order before returning.

.. note::

   The calling and returning instructions in :ref:`the-virtual-machine` have a
   byte immediate operand for how many arguments or return values there are,
   meaning you can only have at most up to 256 arguments and 256 returns.

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
    :ref:`the-virtual-machine` stack is as small as possible throughout the
    execution of the program. This is done by keeping track, at any one time,
    of what the index of the top of the stack is.

Sockets
=======

One important thing to note is how a node knows which input corresponds to
which element in the stack.

Inside the ``SheetSocket`` structure defined in ``dsheet.h``, there is a
property called ``_stackIndex``. Only Code Generation uses this property.
If the socket is connected, it will be set to the index of the socket's value
in the stack at that time. If there is no connection, then an index may or may
not be assigned for the literal value, depending on if we want to use the
literal value as an immediate or not.

.. note::

   See :ref:`the-virtual-machine` to see what I mean by an *immediate* value.

This way, indexes are passed along from socket to socket, so that the next node
knows in what index the socket's value is in.

.. note::

   Most of the bytecode generation functions guarantee that the output is on
   the top of the stack, so usually we take advantage of this and don't bother
   to read the socket's stack index.

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

.. doxygenfunction:: d_push_literal
   :no-link:

This function takes a literal value and pushes it onto the top of the stack.

.. doxygenfunction:: d_push_variable
   :no-link:

This function takes a variable's value and pushes it onto the top of the stack.

.. doxygenfunction:: d_push_input
   :no-link:

This function takes an input socket and pushes it's value onto the top of the
stack. However, that is not as simple as it sounds, as it may generate the
entire bytecode nessesary to get the value.

.. doxygenfunction:: d_push_node_inputs
   :no-link:

This function takes the variable inputs of a node, and pushes them onto the top
of the stack in an order of your choosing.

.. doxygenfunction:: d_generate_operator
   :no-link:

This function generates the bytecode for a generic operator (e.g. ``Add``).

.. doxygenfunction:: d_generate_comparator
   :no-link:

This function generates the bytecode for a generic comparator (e.g. ``Equal``).

.. doxygenfunction:: d_generate_call
   :no-link:

This function generates the bytecode to call a function or subroutine.
It abides to the calling convention and pushes the arguments in the correct
order.

.. doxygenfunction:: d_push_argument
   :no-link:

If you are in a function or subroutine, and you want an argument on the top of
the stack, this is the function for that.

.. doxygenfunction:: d_generate_return
   :no-link:

This function generates the bytecode to return from the function or subroutine.
It abides to the calling convention and pushes the return values in reverse
order.

.. doxygenfunction:: d_generate_nonexecution_node
   :no-link:

This function generates the bytecode for a generic non-execution node.

.. doxygenfunction:: d_generate_execution_node
   :no-link:

This function generates the bytecode for a generic execution node.

.. doxygenfunction:: d_generate_start
   :no-link:

This function generates the bytecode for the main Start function.

.. doxygenfunction:: d_generate_function
   :no-link:

This function generates the bytecode for a function or subroutine defined in
the sheet.

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
