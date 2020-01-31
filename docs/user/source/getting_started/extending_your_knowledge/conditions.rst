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

Conditions
==========

Conditions are non-execution nodes that check two items and return a
``Boolean`` (which is either ``true`` or ``false``).

You can check to see whether two numbers or strings are:

Equal
-----

.. code-block:: decision

   Start~#1
   Equal(5, 5)~#2
   Print(#1, #2)~#3

   Equal(5, 6)~#4
   Print(#3, #4)~#5

   Equal("Hello", "Hello")~#6
   Print(#5, #6)

.. code-block::

   $ decision equal.dc

   true
   false
   true

Not Equal
---------

.. code-block:: decision

   Start~#1
   NotEqual(5, 5)~#2
   Print(#1, #2)~#3

   NotEqual(5, 6)~#4
   Print(#3, #4)~#5

   NotEqual("Hello", "Hello")~#6
   Print(#5, #6)

.. code-block::

   $ decision equal.dc

   false
   true
   false

Less Than
---------

.. code-block:: decision

   Start~#1
   LessThan(5.6, 12.9)~#2
   Print(#1, #2)~#3

   LessThan(100, 100)~#4
   Print(#3, #4)~#5

   LessThan("hello", "help")~#6
   Print(#5, #6)

.. code-block::

   $ decision less_than.dc

   true
   false
   true

Less Than or Equal
------------------

.. code-block:: decision

   Start~#1
   LessThanOrEqual(5.6, 12.9)~#2
   Print(#1, #2)~#3

   LessThanOrEqual(100, 100)~#4
   Print(#3, #4)~#5

   LessThanOrEqual("hello", "help")~#6
   Print(#5, #6)

.. code-block::

   $ decision less_than_or_equal.dc

   true
   true
   true

More Than
---------

.. code-block:: decision

   Start~#1
   MoreThan(100, 10)~#2
   Print(#1, #2)~#3

   MoreThan(6.7, 6.7)~#4
   Print(#3, #4)~#5

   MoreThan("world", "worlds")~#6
   Print(#5, #6)

.. code-block::

   $ decision more_than.dc

   true
   false
   false

More Than or Equal
------------------

.. code-block:: decision

   Start~#1
   MoreThanOrEqual(100, 10)~#2
   Print(#1, #2)~#3

   MoreThanOrEqual(6.7, 6.7)~#4
   Print(#3, #4)~#5

   MoreThanOrEqual("world", "worlds")~#6
   Print(#5, #6)

.. code-block::

   $ decision more_than_or_equal.dc

   true
   true
   false
