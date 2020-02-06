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

############
Syntax Rules
############

Comments
========

Comments are defined as everything after and including the ``>`` symbol on the
same line, as long as the ``>`` is not in a string. Comments should be ignored
by the compiler. They can be freely used by the programmer, but their main
purpose is for GUI editors to store graphical information in them, like the
positions of nodes for example.

.. code-block:: decision

   > This is a comment!
   > This is still a comment! > Still the same comment...

Statements
==========

Each statement represents a node in the sheet. Statements cannot span more
than one line.

The most atomic syntax for a node is the name of the node on its own:

.. code-block:: decision

   Start

This Start node has no connected inputs or outputs.

.. note::

   Names are defined as any combination of alphanumeric and underscore (``_``)
   characters, but only if the name starts with a letter or an underscore.

Wire Identifiers and Node Outputs
---------------------------------

Outputs of nodes are given as **wire identifiers**, which let inputs to other
nodes know which output to get their input from. They are given as a hashtag
(``#``), followed by a positive integer.

Wire identifiers can then be defined as the outputs of nodes, in a
comma-seperated list after the ``~`` symbol, like so:

.. code-block:: decision

   > #1 is an execution wire, since it is coming out of the Start node socket.
   Start~#1

Arguments and Node Inputs
-------------------------

To provide inputs into nodes, you provide a comma-seperated list of either
*literals* or *wire identifiers* surrounded by ``(`` and ``)`` symbols
straight after the name of the node, like so:

.. code-block:: decision

   NodeName(666, 3.14, "Hello, world!", true, #4, ...)

However, if there are no arguments you wish to give, then the brackets are not
nessesary.

.. note::

   In some nodes, you need to provide *names* as arguments.

.. code-block:: decision

   > The node Set sets the value of a variable.
   > integerVariableName is the name of the variable to set.
   > #5 is the input execution wire.
   > 99 is the value to set.
   > #6 is the execution wire to activate after we've set the value.

   Set(integerVariableName, #5, 99)~#6

General Statement Syntax
------------------------

Bringing the outputs and inputs together, this is what a statement will
look like:

.. code-block:: decision

   NodeName(arg1, arg2, ...) ~ #1, #2, ...

In order to seperate statements, they will need to either have ``;`` symbols
or newlines ``\n`` between them:

.. code-block:: decision

   Start~#1; Print(#1, "Hello, world!")~#2
   Print(#2, "Hi!")

.. note::

   Whitespace should never matter.

.. code-block:: decision

   Print ( # 9 , "Hello, world!" ) ~ # 56
   > Is equivalent to...
   Print(#9,"Hello, world!")~#56

Property Statements
===================

Each property is represented by the name of the property and a list of
comma-seperated arguments surrounded by ``(`` and ``)``. Both of these items
are surrounded by ``[`` and ``]``:

.. code-block:: decision

   [PropertyName(arg1, arg2, ...)]

Property arguments however cannot include wire identifiers. They can be names,
literals, or *keyword representations of data types*:

.. code-block:: decision

   <
       This creates a global variable called integerVariableName, which is an
       Integer, with starting value 200.
   >
   [Variable(integerVariableName, Integer, 200)]
