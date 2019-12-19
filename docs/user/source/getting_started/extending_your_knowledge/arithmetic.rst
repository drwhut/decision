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

Arithmetic
==========

Decision includes some basic arithmetic operators by default.

Addition
--------

You can add at least two numbers together:

.. code-block:: decision

   Start~#1
   Add(5, 6)~#2
   Print(#1, #2)~#3

   Add(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)~#4
   Print(#3, #4)

.. code-block::

   $ decision addition.dc

   11
   55

Subtraction
-----------

You can subtract two numbers:

.. code-block:: decision

   Start~#1
   Subtract(12.63, 5.82)~#2
   Print(#1, #2)

.. code-block::

   $ decision subtract.dc

   6.81

Multiplication
--------------

You can multiply at least two numbers together:

.. code-block:: decision

   Start~#1
   Multiply(5.5, 10)~#2
   Print(#1, #2)~#3

   Multiply(1, 2, 3, 4, 5)~#4
   Print(#3, #4)

.. code-block::

   $ decision multiply.dc

   55
   120

Division
--------

You can divide two numbers:

.. code-block:: decision

   Start~#1
   Divide(4, 2)~#2
   Print(#1, #2)~#3

   Divide(10, 3)~#4
   Print(#3, #4)

.. code-block::

   $ decision division.dc

   2
   3.33333

Integer Division
----------------

You can divide two numbers and *truncate* the result to get an integer:

.. code-block:: decision

   Start~#1
   Div(4, 2)~#2
   Print(#1, #2)~#3

   Div(10, 3)~#4
   Print(#3, #4)

.. code-block::

   $ decision div.dc

   2
   3

Modulo
------

You can take the remainder after division of two integers:

.. code-block:: decision

   Start~#1
   Mod(4, 2)~#2
   Print(#1, #2)~#3
   
   Mod(10, 3)~#4
   Print(#3, #4)

.. code-block::

   $ decision mod.dc

   0
   1
