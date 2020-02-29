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

.. _semantic-analysis:

#################
Semantic Analysis
#################

Semantic Analysis is performed in ``dsemantic.c`` and ``dsemantic.h``.

This stage checks to see whether what the user has written is *correct*,
in terms of the nodes, their connections, and their definitions. During this
stage, we build up a representation of the sheet in memory (``Sheet``, defined
in ``dsheet.h``), and check for errors using that structure.

Property Scanning
=================

Firstly, we scan the syntax tree we made in :ref:`syntax-analysis`, and
extract the property statements that were found. We use the property
statements to manipulate a sheet's data.

.. doxygenfunction:: d_semantic_scan_properties
   :no-link:

Takes a reference to a ``Sheet``, and depending on what property statements
we find in the syntax tree ``root``, we manipulate ``sheet``. We first check
to see if its a property name we expect to find. Then, the necessary
information is passed onto a function called ``add_property_PropertyName``
in ``dsemantic.c``, where ``PropertyName`` is the name of the property, e.g.
``add_property_Variable``.

Node Scanning
=============

Secondly, we scan the syntax tree to find the general statements, and we
find out:

* What the name of the node is, and if it is a defined name.
* What the inputs and outputs are. If they are literals, are they of the
  correct data type? If they are names, are they defined?

.. note::

   When we say **literals** we mean data explicitly inserted into the source
   code, e.g. In ``Print(#1, "Hello, world!")``, ``"Hello, world!"`` is the
   literal.

.. note::

   If an integer literal was given to a socket that is exclusively for floats,
   then the compiler *should* automatically convert the integer literal to a
   float literal for convenience.

Afterwards, we take the input and output sockets and connect them. While
connecting them, we check to see if the data types of both sockets match,
and we check that an output execution socket doesn't have more than one
connection, etc.

.. doxygenfunction:: d_semantic_scan_nodes
   :no-link:

Takes ``sheet`` and creates a list of ``Node`` in the sheet's graph, as well as
a list of ``Wire`` to connect the sockets of the nodes.

When we want to find out where the name of something is defined, we can use:

.. doxygenfunction:: d_get_name_definitions
   :no-link:

This gets all the places a name is defined. Ideally, there should only be one
definition of a name. If there is more than one, then we error saying that we
can't pick a definition, and if there is none, then we error saying the name
is not defined.

If we know there is only one definition of a name, then we can go straight
to the name's definition with this function:

.. doxygenfunction:: d_get_definition
   :no-link:

.. note::

   Nodes store a ``NameDefinition`` as a cache for :ref:`code-generation`,
   but what they store depends on the node itself. If the node is the getter
   or setter of a variable, the definition stored references **the variable.**
   If the node is a ``Define`` or ``Return`` node, the definition stores a
   reference to **the function being defined or returned**. Otherwise, the
   definition stores a reference to **the node's functionality itself.**

Remember that the sheets can include other sheets, so we need to check
included sheets recursively to see if there are names that are defined that
we can use.

You can free an ``AllNameDefinitions`` struct with:

.. doxygenfunction:: d_free_name_definitions
   :no-link:

The act of checking the data type matching of connections is done when we add
a connection to a socket with the function:

.. doxygenfunction:: d_graph_add_wire
   :no-link:

which is defined in ``dgraph.h``.

Reducing Data Types
===================

Wait, *reducing* types? What does that mean???

I've been hiding a dark truth from you this whole time... data types are
sometimes not what they seem. Sometimes... **a socket can have more than one
data type!** *(insert shocked Pikachu face)*

So this is a thing that exists purely so that we don't have multiple functions
for different data types. For example, we can use the ``Add`` node without
having to worry about whether to call ``AddInt`` or ``AddFloat``. We say
sockets have **vague** data types if they can allow more than one data type.

.. note::

   Only core functions defined in ``dcore.c`` have vague data types.

The reason we need to reduce down vague data types to single data types is
for when we come to :ref:`code-generation`, the compiler needs to know
*exactly* what data types it is dealing with. As a wise woman once said:

    "Ain't nobody got time for ambiguity!"

.. doxygenfunction:: d_semantic_reduce_types
   :no-link:

Takes the connections of a sheet and reduces down vague connections.

It does this by completing *passes* until all of the sockets are reduced.
During each pass, it will use the information it has to reduce down a
connection. Here is an example:

.. code-block:: decision

   [Variable(i, Integer, 10)]

   i~#1
   Multiply(#1, 2)~#2
   Multiply(#2, 3.14)~#3

   Start~#4
   Print(#4, #3)

* Since ``i`` is an Integer, line ``#1`` begins as an Integer as well.
* When the first ``Multiply`` is reached, it checks it's input data types,
  which are both integers. Thus, it's output is set from Integer | Float
  to just an Integer.
* When the second ``Multiply`` is reached, like the first, it checks it's
  input data types. The first is an integer since we've just reduced it, but
  the second is a Float literal. Thus, the output is set from Integer | Float
  to just a Float.
* When the ``Print`` is reached, it checks its non-execution connection, and
  we've just reduced it down to a Float. Thus, the socket is set from the
  union of all data types (since ``Print`` should be able to print anything
  except for Execution types) to just a Float.

We are lucky in this example that everything got fully reduced in one pass
since the order of the nodes was optimal. But if we change the order in which
the nodes are reduced, we will need to do multiple passes before every node
is reduced.

What's that, I hear you ask? How do you represent combinations of multiple
data types, and how do we make sure that two vague data types are compatible?
Well, I'm glad you asked!

Data types are represented as separate bits in an integer, for example, in
the integer 00000110 represents Integer | Float, since both the Integer bit
(``TYPE_INT = 2``) and the Float bit (``TYPE_FLOAT = 4``) are active.
Therefore, if two data types are compatible, then at least one base data type
has to be present in both data types - so we can **bitwise AND** both data
types, and hope to god we don't get 0, i.e.

.. code-block:: c

   (type1 & type2) != 0

Here is what the ``DType`` enum looks like:

.. doxygenenum:: _dType
   :no-link:

Detecting Loops
===============

If you abstract sheets down far enough, they will just turn into **directed
graphs**. Get your graph theory textbook of choice at the ready, and you'll
see why this matters!

Consider this example:

.. code-block:: decision

   i~#1
   Add(#1, #3)~#3

This is clearly erroneous since we can never get a value for ``#3``. In fact,
**all loops are erroneous**, since we can't get input values from nodes that
depend on the input value indirectly.

So fourthly, we need to check that the directed graph we have is a **tree**,
i.e. a **connected** graph with **no cycles**. The connected bit we've already
dealt with, since unconnected nodes can't be accessed anyway, but the no
cycles bit is tricky...

We also want to take the time to let the user know if there are any nodes
that have no connections, i.e. *redundant* nodes.

.. doxygenfunction:: d_semantic_detect_loops
   :no-link:

Takes ``sheet`` and detects if there are any loops in it. We also warn the
user of any redundant nodes while we're checking for loops as well.

This function goes through all of the nodes and finds nodes with no input
sockets (except for names). These nodes are the start of a path of execution.
We each of these nodes, we enter a function with the node with no input sockets
``start``, we get the outputs of the node and recursively call the function
again, and again. We are essentially checking every possible path through the
graph from the starting node using depth-first search. As we are checking each
path, we are adding the latest node into ``pathArray``, which acts as a stack,
and represents the current path we've taken. Thus, if the node we've just added
is already in ``pathArray``, we have a cycle!

Checking Subroutine Returns
===========================

Last but not least, once we have verified that there are no loops in the graph,
we need to check that every subroutine in the sheet has a Return node at the
end of every possible execution path.

The reason we need to check this is because execution paths can *branch*, e.g.
with an IfThenElse node. If an execution path doesn't end with a Return, then
we don't know what return values the subroutine should return!

Functions don't have this problem, as they have one and only one Return node.

This check is done with the function:

.. doxygenfunction:: d_semantic_check_subroutine_returns
   :no-link:

It calls a recursive function that checks every possible execution path with
depth-first search, similar to how ``d_semantic_detect_loops`` works, but this
time it only traverses execution wires.

Conclusion
==========

So we have a bunch of things that we need to check the sheet for - surely a
good samaritan would have put all of the above and wrapped it up in a nice,
cosy function for everyone to use?

.. doxygenfunction:: d_semantic_scan
   :no-link:
