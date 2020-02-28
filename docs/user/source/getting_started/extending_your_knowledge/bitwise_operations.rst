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

Bitwise Operations
==================

As of the 0.3.0 update, you can apply the boolean operators mentioned in the
previous page to integers, which will make them act as bitwise operators.

When we say bitwise, we mean the operator will apply the operation on each bit
(a bit being a 1 or a 0) of the two integers, one at a time, to form the new
integer.

And
---

The new bit is 1 if and only if the two input bits are both 1.

.. code-block:: decision

   Start~#1

   And(53, 29)~#2   >   110101 AND 011101
   Print(#1, #2)~#3 > = 010101

.. code-block::

   $ decision bitwise_and.dc

   21

Or
--

The new bit is 1 if and only if either of the two input bits are 1.

.. code-block:: decision

   Start~#1

   Or(53, 29)~#2    >   110101 OR 011101
   Print(#1, #2)~#3 > = 111101

.. code-block::

   $ decision bitwise_or.dc

   61

Not
---

The output bit is 1 if the input bit was 0, and the output bit is 0 if the
input bit was 1.

.. code-block:: decision

   Start~#1

   Not(0)~#2     > 0 is represented as all 0's.
   Print(#1, #2) > This will be all 1's, which will be -1 on systems which
                 > use two's complement to represent integers.

.. code-block::

   $ decision bitwise_not.dc

   -1

.. warning::

   The output of ``Not`` will depend on the architecture, for example, the
   output may differ between a 32-bit system vs a 64-bit one.

Xor
---

The output bit is 1 if and only if one of the input bits is 1.

.. code-block:: decision

   Start~#1

   Xor(53, 29)~#2   >   110101 XOR 011101
   Print(#1, #2)~#3 > = 101000

.. code-block::

   $ decision bitwise_xor.dc

   40
