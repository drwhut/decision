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

Changing Flow with Conditions
=============================

You can then use conditions to change the direction in which the execution of
the script flows.

If Then
-------

This execution node takes in a ``Boolean``, and activates the next node along
if the condition is ``true``.

It also has an extra output for *after* the condition has been activated.

.. code-block:: decision

   Start~#1
   MoreThan(6, 5)~#2
   IfThen(#1, #2)~#3, #10
   Print(#3, "6 is more than 5!")~#4

   LessThan(50, 25)~#5
   IfThen(#4, #5)~#6
   Print(#6, "50 is less than 25!")

   Print(#10, "Done with everything.")

.. code-block::

   $ decision if_then.dc

   6 is more than 5!
   Done with everything.

.. note::

   If the two ``IfThen`` nodes were swapped, then neither of the ``Print``
   nodes would activate, as the first condition would be ``false``, then none
   of the nodes after would be activated.

If Then Else
------------

This execution node takes in a ``Boolean``, and activates the first node if
the condition is ``true``, and activates the second if the condition is
``false``.

Like with ``IfThen``, there is also a third output for *after* the condition
has been evaluated.

.. code-block:: decision

   Start~#1
   MoreThan(6, 5)~#2
   IfThenElse(#1, #2)~#3, #4
   Print(#3, "6 is more than 5!")~#5
   Print(#4, "6 it not more than 5!")~#5

   LessThan(50, 25)~#6
   IfThenElse(#5, #6)~#7, #8
   Print(#7, "50 is less than 25!")
   Print(#8, "50 is not less than 25!")

.. code-block::

   $ decision if_then_else.dc

   6 is more than 5!
   50 is not less than 25!

.. note::

   Notice how two ``Print`` statements have the same wire identifier as output.
   This is telling Decision "these two sockets connect to the same place",
   in this case the input of the second ``IfThenElse`` node.

   You can do this for ``Execution`` wires only. If you tried to do this for
   any other wire, e.g. an ``Integer`` wire, you would get an error because
   the compiler wouldn't know *which* input to take.

Ternary
-------

Unlike the previous nodes, ``Ternary`` is a non-execution node. This comes in
handy when you want to write functions that only use non-execution nodes later
on.

This means that, like all other non-execution nodes, it will evaluate the
condition whenever it needs to, rather than when the node is executed.

This node will check the condition, and it will return either one input or
another depending on the condition.

.. note::

   The second and third inputs can be of any type, as long as they are the
   *same* type.

.. code-block:: decision

   Start~#1

   MoreThan(6, 5)~#2
   Ternary(#2, "6 is more than 5!", "6 is not more than 5!")~#3
   Print(#1, #3)~#4

   LessThan(50, 25)~#5
   Ternary(#5, "50 is less than 25!", "50 is not less than 25!")~#6
   Print(#4, #6)

.. code-block::

   $ decision ternary.dc

   6 is more than 5!
   50 is not less than 25!
