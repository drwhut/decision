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

   [Variable(integerVariableName, Integer, 420, "Because i'm sensible...")]
   [Variable(floatVariableName, Float, 3.14, "Yum...")]
   [Variable(stringVariableName, String, "Hello, world!", "... Hi?")]
   [Variable(booleanVariableName, Boolean, false, "You either like it or ya don't.")]

Here's what each of the arguments mean:

1. The first argument is the name of the variable. You'll use this name to get
   and set the variable's value.
2. The second argument is the data type of the variable.
3. The third argument is the starting value of the variable.
4. (Optional) The fourth argument is the description of the variable.

.. note::

   The names of variables (as well as other names) can be made up of letters,
   numbers, and underscores (``_``). But they have to start with either a
   letter or an underscore.

Getting variable values
-----------------------

You can get the values stored in variables from nodes with the same name as
the variable:

.. code-block:: decision

   [Variable(myNum, Integer, 420, "I am REALLY sensible.")]

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
