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

Iteration
=========

If you want to activate the same set of nodes multiple times, you can do so
with iteration nodes!

For
---

The ``For`` loop iterates a set of nodes, keeping track of a variable that
changes every loop.

A ``For`` node has 4 inputs and 3 outputs:

* The first input is the execution input, like all other execution nodes.

* The second input is the **start** number, i.e. what value does this variable
  hold when you start looping?

* The third input is the **end** number, i.e. what value does this variable
  need to reach to stop the loop?

* The fourth input is the **step** number, i.e. how much do we
  increment/decrement this variable every loop?

It's outputs are:

* The first output is the **loop execution**, i.e. the nodes that run every
  loop.

* The second output is the **variable value**.

* The third output is the **after execution**, i.e. it is activated when
  looping has stopped.

.. code-block:: decision

   Start~#1
   For(#1, 1, 10, 1)~#2, #3, #4
   Print(#2, #3)

   Print(#4, "Done!")

.. code-block::

   $ decision for.dc

   1
   2
   3
   4
   5
   6
   7
   8
   9
   10
   Done!

.. code-block:: decision

   [Variable(num1, Integer, 1052)]
   [Variable(num2, Integer, 516)]

   [Variable(gcf, Integer, 1)]

   > A program to calculate the greatest common factor of num1 and num2,
   > given that num1 is greater than num2.
   Start~#1

   num1~#2
   num2~#3

   For(#1, 2, #3, 1)~#4, #5, #6
   Mod(#2, #5)~#7
   Mod(#3, #5)~#8
   Equal(#7, 0)~#9
   Equal(#8, 0)~#10
   And(#9, #10)~#11 > #5 is a factor of num1 AND num2
   IfThen(#4, #11)~#12
   Set(gcf, #12, #5)

   gcf~#13
   Print(#6, #13)

.. code-block::

   $ decision gcf.dc

   4

While
-----

The ``While`` loop iterates a set of nodes while a condition is true.

It has 2 inputs:

* The first input is the execution input.

* The second input is a ``Boolean`` that, if ``true``, means it will keep
  looping.

It also has 2 outputs:

* The first output is the **loop execution**, i.e. the nodes that run every
  loop.

* The second output is the **after execution**, i.e. it is activated when
  looping has stopped.


.. code-block:: decision

   [Variable(num, Integer, 1)]

   Start~#1

   num~#2
   LessThanOrEqual(#2, 10)~#3
   
   While(#1, #3)~#4, #5
   Print(#4, #2)~#6

   Add(#2, 1)~#7
   Set(num, #6, #7)

   Print(#5, "Done!")

.. code-block::

   $ decision while.dc

   1
   2
   3
   4
   5
   6
   7
   8
   9
   10
   Done!

.. code-block:: decision

   [Variable(stop, Integer, 100)]

   [Variable(num1, Integer, 0)]
   [Variable(num2, Integer, 1)]
   [Variable(temp, Integer, 1)]

   > A program to calculate the fibonacci numbers from 0 to stop

   Start~#1

   num1~#2
   num2~#3
   Add(#2, #3)~#4

   stop~#5
   LessThanOrEqual(#4, #5)~#6

   Print(#1, #2)~#7
   Print(#7, #3)~#8

   While(#8, #6)~#9
   Print(#9, #4)~#10
   Set(temp, #10, #4)~#11  > temp = num1 + num2
   Set(num1, #11, #3)~#12  > num1 = num2
   temp~#13
   Set(num2, #12, #13)~#14 > num2 = temp

.. code-block::

   $ decision fibonacci.dc

   0
   1
   1
   2
   3
   5
   8
   13
   21
   34
   55
   89
