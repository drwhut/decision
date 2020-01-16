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

Variables
=========

Unlike in the wires, variables are named values kept seperate from the wires,
and have their own nodes to access and manipulate their values.

Variables can then be accessed from anywhere in the sheet, and as we'll see
later, from anywhere outside the sheet as well!

Defining variables
------------------

You can define variables by defining a ``Variable`` property in the sheet:

.. code-block:: decision

   [Variable(integerVariableName, Integer, 420)]
   [Variable(floatVariableName, Float, 3.14)]
   [Variable(stringVariableName, String, "Hello, world!")]
   [Variable(booleanVariableName, Boolean, false)]

.. note::

   The third argument is the **starting value** of the variable, and it is
   optional. If you don't give it, Decision will warn you, but it will use
   ``0``, ``0.0``, ``NULL``, and ``false`` as the default values respectively.

.. note::

   The names of variables (as well as other names) can be made up of letters,
   numbers, and underscores (``_``). But they have to start with either a
   letter or an underscore.

Getting variable values
-----------------------

You can get the values stored in variables from nodes with the same name as
the variable:

.. code-block:: decision

   [Variable(myNum, Integer, 420)]

   Start~#1
   myNum~#2
   Print(#1, #2)

.. code-block::

   $ decision get_var.dc

   420

Setting variable values
-----------------------

You can set the values of variables by using the ``Set`` node, in which you
give the name of the variable, the input execution socket (so *when* to set
the variable), and the value you want to set.

.. code-block:: decision

   [Variable(myStr, String, "Hello!")]

   Start~#1
   myStr~#2

   Print(#1, #2)~#3

   Set(myStr, #3, "Hello, world!")~#4
   Print(#4, #2)

.. code-block::

   $ decision set_var.dc

   Hello!
   Hello, world!

.. note::

   Even though the value of ``myStr`` changed inbetween the ``Print`` nodes,
   using the same ``myStr`` node gave the updated value.
