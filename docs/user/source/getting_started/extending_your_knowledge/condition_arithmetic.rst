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

Condition Arithmetic
====================

Since conditions are just booleans, you can perform some simple bitwise
operations on conditions:

And
---

Outputs ``true`` if and only if the two inputs are both ``true``.

.. code-block:: decision

   Start~#1

   And(false, false)~#2
   Print(#1, #2)~#3

   And(false, true)~#4
   Print(#3, #4)~#5

   And(true, false)~#6
   Print(#5, #6)~#7

   And(true, true)~#8
   Print(#7, #8)

.. code-block::

   $ decision and.dc

   false
   false
   false
   true

Or
--

Outputs ``true`` if and only if either of the two inputs are ``true``.

.. code-block:: decision

   Start~#1

   Or(false, false)~#2
   Print(#1, #2)~#3

   Or(false, true)~#4
   Print(#3, #4)~#5

   Or(true, false)~#6
   Print(#5, #6)~#7

   Or(true, true)~#8
   Print(#7, #8)

.. code-block::

   $ decision or.dc

   false
   true
   true
   true

Not
---

Outputs ``true`` if the input is ``false``, and outputs ``false`` if the input
is ``true``.

.. code-block:: decision

   Start~#1

   Not(false)~#2
   Print(#1, #2)~#3

   Not(true)~#4
   Print(#3, #4)

.. code-block::

   $ decision not.dc

   true
   false

Xor
---

Outputs ``true`` if and only if only one if the inputs is ``true``.

.. code-block:: decision

   Start~#1

   Xor(false, false)~#2
   Print(#1, #2)~#3

   Xor(false, true)~#4
   Print(#3, #4)~#5

   Xor(true, false)~#6
   Print(#5, #6)~#7

   Xor(true, true)~#8
   Print(#7, #8)

.. code-block::

   $ decision xor.dc

   false
   true
   true
   false
